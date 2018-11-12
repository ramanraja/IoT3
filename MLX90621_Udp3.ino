// Streams data from MLX90621 through UDP to a local python UDP receiver
// sends it as a series of individual numbers delimited by [ and ]
// If the sensor reading is NAN or is out of bounds, resets the sensor through a 2N2222.
// test it with UdpReceiver5.py or  https://packetsender.com/
// production python code: thermalCam22.py and thermalCam23.py
// done: OTA update
 
// MLX90621_Arduino_Processing code is downloaded  from
// https://github.com/robinvanemden/MLX90621_Arduino_Processing
// and modified by Rajaraman in Oct 2018 to replace i2c_t3 library with standard Wire library
// Original code and discussion at:
// http://forum.arduino.cc/index.php/topic,126244.msg949212.html#msg949212


/*
* Connection Instructions:
* Connect the Anode of a Silicon Diode to 3V Pin of Teensy. The Diode will drop ~0.7V, so the 
* Cathode will be at ~2.7V. 
* These 2.7V will be the supply voltage "VDD" for the sensor.
* Plug in the USB and measure the supply voltage with a multimeter! - it should be somewhere between 
* 2.5V and 2.75V, else it will fry the sensor !
* ...disconnect USB again...
* Connect Teensy Pin 18 to 2.7V with a 4.7kOhm Resistor (Pullup)
* Connect Teensy Pin 19 to 2.7V with a 4.7kOhm Resistor (Pullup)
* Connect I2C Data (SDA = D2 = GPIO 4) and Clock (SCK = D1 = GPIO 5) Pins of Sensor with 4.7kOhm Pullup reistors
* Connect GND and 2.7V with a 100nF ceramic Capacitor.
* Connect the VSS Pin of the Sensor to GND.
* Connect the VDD Pin of the Sensor to 2.7V (if 3V through diode)
Some times the sensor freezes, so you may need  a transistor to reset it.
 */

#include "common.h"
#include "config.h"
#include "otaHelper.h"
#include <stdlib.h>
#include <Arduino.h>
#include "MLX90621.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Timer.h>

#define ssid      "RajaCell"  // TODO: use separate Wifi class and Config settings
#define passwd    "raja1234" 

#define ROWS              4
#define COLS              16
#define DATA_LENGTH       64   // 4*16
#define IN_BUFFER_SIZE    32
#define OUT_BUFFER_SIZE   512  // ASSUMING 64*8 characters payload size
#define DATA_TIMEOUT      10   // x seconds*2 
#define LOWER_LIMIT       0    // to flag sensor error
#define UPPER_LIMIT       100  // to flag sensor error

Timer T;
WiFiClient wifi_client;
MLX90621 sensor; //  an instance of the Sensor class
WiFiUDP Udp;
                
char request_packet[IN_BUFFER_SIZE];     
char response_packet[OUT_BUFFER_SIZE];      
IPAddress remote_IP = IPAddress(192,168,0,105);   // default values, but later
unsigned int remote_port = 54321;                 // these may be overridden by requestor
boolean use_static_IP = true;

IPAddress ip(192,168,0,109);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);
      
//int  FPS_param = 16;   // thermal cam 8 frames per second
int  FPS_param = 4;   // thermal cam 2 frames per second

int  data_timer = DATA_TIMEOUT; // time out for no incoming data requests (x seconds * 2)
bool data_timed_out = true;

// NOTE: do not use pins D1 and D2, as they are meant for I2C
int  sensor_enable_pin = 12;  // D6 
int  led1 = 16;  // D0  
 
Config C;
OtaHelper O;
 
void setup(){ 
    pinMode (led1, OUTPUT);
    blinker();
    pinMode(sensor_enable_pin, OUTPUT);
    digitalWrite(sensor_enable_pin, HIGH);
    Serial.begin(115200); 
    SERIAL_PRINTLN("\nThermal camera MLX 90621 starting...");
    C.init();
    O.init(&C);
    
    sensor.initialise (FPS_param);  
    SERIAL_PRINTLN("MLX Sensor initialized.");
 
    init_wifi(); 
    Udp.begin(C.udp_port);
    
    SERIAL_PRINT("Listening at local IP: ");  
    SERIAL_PRINTLN(WiFi.localIP());
    SERIAL_PRINT("on UDP Port: ");
    SERIAL_PRINTLN(C.udp_port);
    SERIAL_PRINT("Default remote IP: ");
    SERIAL_PRINTLN (remote_IP.toString().c_str());
    SERIAL_PRINT ("Default remote port: ");
    SERIAL_PRINTLN(remote_port);
    
    T.every(2000, data_watchdog); // 2x seconds
    T.every(3600000L, check_for_updates);  //  every hour
}

void loop(){
    T.update();   
    if (WiFi.status() != WL_CONNECTED)
        init_wifi();
    send_data();
}

// data will be sent when a request is received or suo motu after a time out
void send_data() {    
    sensor.measure(); // collect pixel data from thermal sensor
    int packetSize = Udp.parsePacket();
    if (packetSize==0)   { // no incoming request
        if (!data_timed_out) 
          return;
    }  else {  // received a request on UDP
      //Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
      remote_IP = Udp.remoteIP();
      remote_port = Udp.remotePort();  
      int len = Udp.read(request_packet, IN_BUFFER_SIZE);
      #ifdef VERBOSE_MODE 
          if (len > 0) {
              request_packet[len] = 0; // null terminate
              SERIAL_PRINT("packet request: ");
              SERIAL_PRINTLN(request_packet);
            }
      #endif
    }
    // send back a reply, to the IP address and port of the requestor
    char tmp[12];     
    double data;
    response_packet[0] = '[';   // start a new packet
    response_packet[1] = '\0';    
    
    for(int y=0; y<ROWS; y++) {       // y = 4 rows
        for(int x=0; x<COLS; x++) {   // x = 16 columns
            data = sensor.getTemperature(y+x*4); // read the temperature at position (x,y)
            if (isnan(data) || data < LOWER_LIMIT || data > UPPER_LIMIT) {
                reset_sensor();
                return;
            }
            // dtostrf(FLOAT, WIDTH, PRECSISION, BUFFER)
            dtostrf(data,1,2,tmp); 
            strcat(response_packet, tmp);
            strcat(response_packet, " ");
        }
    }
    strcat(response_packet, "]");
    SERIAL_PRINTLN(response_packet);
    Udp.beginPacket(remote_IP, remote_port);
    Udp.write(response_packet);
    Udp.endPacket();
    data_timed_out = false;
    data_timer = DATA_TIMEOUT;   // keep reloading the hour glass
}

void data_watchdog() {
    data_timer = data_timer-2;  // 2 seconds timer
    if (data_timer <= 0) {
        data_timed_out = true;
        SERIAL_PRINTLN("Data watchdog timed out.");
    }
}

void reset_sensor() {
  SERIAL_PRINTLN("-------------->>>> Sensor error ! resetting...");
  digitalWrite(sensor_enable_pin, LOW);
  delay(2000);
  digitalWrite(sensor_enable_pin, HIGH);
  delay(500);  
  sensor.initialise (FPS_param);
  delay(500);    
}

void check_for_updates(){
  O.check_and_update();
}

void init_wifi() {
  SERIAL_PRINT("Connecting to ");
  SERIAL_PRINTLN(ssid);
  WiFi.begin(ssid, passwd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    SERIAL_PRINT(".");
  }
  SERIAL_PRINTLN("\nWiFi Connected.");
  if (use_static_IP) {
      WiFi.config(ip,gateway,subnet);
      SERIAL_PRINTLN("WiFi Configured.");
  }
  SERIAL_PRINTLN("IP address: "); 
  SERIAL_PRINTLN(WiFi.localIP());
}

void blinker() {
    for (int i=0; i<5; i++) {
        digitalWrite(led1, LOW); 
        delay(200);
        digitalWrite(led1, HIGH); 
        delay(200);    
    }
}
    
