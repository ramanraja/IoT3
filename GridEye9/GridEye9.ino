/***************************************************************************
  Reads pixel data from AMG88xx GridEYE 8x8 IR camera and transmits to PC over UDP 
  as a string of floats delimited by [ and ] with CR-LF.
  If the sensor reading is NAN or is out of bounds, resets the sensor through a 2N2222.
  Test it with  Arduino serial monitor by sending any random characters as prompt. 
  Use it with Python visualizer ThermalCam30.py
  https://travis-ci.org/adafruit/Adafruit_AMG88xx
  https://github.com/adafruit/Adafruit_AMG88xx
  No need to install the library in the Arduino path; just add the cpp and h files in your
  project folder.  I2C address of Adafruit AMG88 breakout board is 0x69.   But the default address 
  of the raw device is  0x68, so replaced this value in the header:
  Edited Adafruit_AMG88xx.h under your documents/Arduino/libraries folder :
     // Modified by Rajaraman
     // #define AMG88xx_ADDRESS        (0x69)
     #define AMG88xx_ADDRESS           (0x68)  
Connect I2C Data (SDA = D2 = GPIO 4) and Clock (SCK = D1 = GPIO 5) Pins of sensor 
   with 4.7 K Pullup reistors. Connect GND and VDD with a 100nF ceramic Capacitor.     
Sometimes the sensor freezes, so you may need  a transistor to reset it.   
****************************************************************************/

#include "AMG88_Udp.h"

Timer T;
Config C;
OtaHelper O;
WiFiUDP Udp;

IPAddress remote_IP = IPAddress(192,168,0,105);   // default values, but later
unsigned int remote_port = 54321;                 // these may be overridden by requestor

boolean use_static_IP = true;
IPAddress udp_ip(192,168,0,110);          // static IP of this sensor device
IPAddress udp_gateway(192,168,0,1);
IPAddress udp_subnet(255,255,255,0);
long udp_port = 12345L;                   // local UDP port to listen for data requests

char request_packet [IN_BUFFER_SIZE];     
char data [OUT_BUFFER_SIZE];      

Adafruit_AMG88xx sensor;

// Never use D1 and D2 for anything else - they are meant for I2C
int  led1 = 16;  // D0
int  sensor_enable_pin = 12;  // D6 

void setup() {
    pinMode (led1, OUTPUT);
    pinMode(sensor_enable_pin, OUTPUT);
    digitalWrite(sensor_enable_pin, HIGH);
    blinker();  
    
    Serial.begin(115200);
    SERIAL_PRINTLN();  // go over 8266 garbage
    SERIAL_PRINTLN("# UDP thermal camera AMG88xx starting...");
    
    C.init();
   
    // start sensor with the default settings
    if (sensor.begin()) 
        SERIAL_PRINTLN("# Sensor initialized.");
    else
        SERIAL_PRINTLN ("# Could not connect to the AMG88xx sensor !");  
    delay(100); // let sensor boot up

    init_wifi();     
    O.init(&C);    
    init_udp();
    T.every(3600000L, check_for_updates);  //  every hour
}

void loop(){
    T.update();   
    if (WiFi.status() != WL_CONNECTED)
        init_wifi();
    send_data();
    //delay(200); 
}

void process_command() { // TODO: reset sensor, reboot etc
  SERIAL_PRINTLN("process_command not implemented");
}

char tmp_str[12];     
double tmp_data;
float pixels [64]; // 8x8 = 64 pixels

// data will be sent when a request is received through serial port
void send_data() {  

    sensor.readPixels(pixels);  //read all the 64 pixels
    
    int packetSize = Udp.parsePacket();
    if (packetSize==0)   // no incoming request    
        return;
    remote_IP = Udp.remoteIP();
    remote_port = Udp.remotePort();  
    int len = Udp.read(request_packet, IN_BUFFER_SIZE);   
    if (len < 1)
        return; 
    #ifdef VERBOSE_MODE 
          request_packet[len] = 0; // null terminate
          SERIAL_PRINT("packet request: ");
          SERIAL_PRINTLN(request_packet);
    #endif          
    process_command();
    
    data[0] = '[';   // start a new packet
    data[1] = '\0';  
    for(int i=0; i<64; i++) {
        tmp_data = pixels[i];
        if (isnan(tmp_data) || tmp_data < LOWER_LIMIT || tmp_data > UPPER_LIMIT) {      
            // TODO: check if many pixels are bad, then reset the sensor
            #ifdef VERBOSE_MODE
               SERIAL_PRINT("# ----->>>> PIXEL ERROR: pixel # ");  
               SERIAL_PRINT(i);                               
               SERIAL_PRINT(" has value= ");
               SERIAL_PRINTLN(tmp_data);
            #endif
            tmp_data = 28.0f;  // TODO: replace with neighbouring pixels!   
        }
        // dtostrf(FLOAT, WIDTH, PRECSISION, BUFFER)
        dtostrf(tmp_data,1,2,tmp_str); 
        strcat(data, tmp_str);
        strcat(data, " ");
    } // for
    strcat(data, "]");
    SERIAL_PRINTLN(data);   
    Udp.beginPacket(remote_IP, remote_port);
    Udp.write(data);
    Udp.endPacket();    
}

void init_udp (){
    Udp.begin(udp_port);
    SERIAL_PRINT("Listening at local IP: ");  
    SERIAL_PRINTLN(WiFi.localIP());
    SERIAL_PRINT("on UDP Port: ");
    SERIAL_PRINTLN(udp_port);
    SERIAL_PRINT("Default remote IP: ");
    SERIAL_PRINTLN (remote_IP.toString().c_str());
    SERIAL_PRINT ("Default remote port: ");
    SERIAL_PRINTLN(remote_port);    
    SERIAL_PRINT("UDP_TX_PACKET_MAX_SIZE: ");
    SERIAL_PRINTLN(UDP_TX_PACKET_MAX_SIZE);   // 8192    
}

void init_wifi() {
  SERIAL_PRINT("Connecting to ");
  SERIAL_PRINTLN(C.wifi_ssid1);
  WiFi.begin(C.wifi_ssid1, C.wifi_password1);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    SERIAL_PRINT(".");
  }
  SERIAL_PRINTLN("\nWiFi Connected.");
  if (use_static_IP) {
      WiFi.config(udp_ip, udp_gateway, udp_subnet);  // TODO: take them from Config
      SERIAL_PRINTLN("WiFi Configured.");
  }
  SERIAL_PRINTLN("IP address: "); 
  SERIAL_PRINTLN(WiFi.localIP());
}

void check_for_updates(){
  O.check_and_update();
}

// TODO: send notification to a listener
void reset_sensor() {
    SERIAL_PRINTLN("# -------------->>>> Sensor error ! resetting...");
    digitalWrite(sensor_enable_pin, LOW);
    delay(2000);
    digitalWrite(sensor_enable_pin, HIGH);
    delay(500);  
    //// TODO: Wire.begin()?
    if (sensor.begin()) 
        SERIAL_PRINTLN("# Sensor initialized.");
    else
        SERIAL_PRINTLN ("# Could not connect to the AMG88xx sensor !");  
    delay(100); // let sensor boot up        
    delay(500);    
  // TODO: send messge to remote listener    
}

void blinker() {
    for (int i=0; i<5; i++) {
        digitalWrite(led1, LOW); 
        delay(200);
        digitalWrite(led1, HIGH); 
        delay(200);    
    }
}
  
