// Streams data from MLX90621 through Serial port
// sends it as a string of floats delimited by [ and ] with CR-LF
// If the sensor reading is NAN or is out of bounds, resets the sensor through a 2N2222.
// test it with thermalSerial.py or  Arduino serial monitor by sending any random 
// characters as prompt. 
 
// MLX90621_Arduino_Processing code is downloaded  from
// https://github.com/robinvanemden/MLX90621_Arduino_Processing
// and modified by Rajaraman in Oct 2018 to replace i2c_t3 library with standard Wire library
// Original code and discussion at:
// http://forum.arduino.cc/index.php/topic,126244.msg949212.html#msg949212
// TODO: check if MOST pixels are nan, then reset the sensor

/*
Connect I2C Data (SDA = D2 = GPIO 4) and Clock (SCK = D1 = GPIO 5) Pins of sensor 
   with 4.7 K Pullup reistors.
Connect GND and VDD with a 100nF ceramic Capacitor.
Connect the VDD Pin of the Sensor to 2.5 - 2.7V supply (* if using 3.3V, connect through a diode *)
Sometimes the sensor freezes, so you may need  a transistor to reset it.
*/

#include "MLX90621_Serial.h"

MLX90621 sensor; //  an instance of the Sensor class
char data[OUT_BUFFER_SIZE];      
     
// Comment out any one of the following two options:      
//int  FPS_param = 16;   // thermal cam 8 frames per second
int  FPS_param = 4;   // thermal cam 2 frames per second

// Never use D1 and D2 for anything else - they are meant for I2C
int  led1 = 16;  // D0
int  sensor_enable_pin = 12;  // D6 
 
void setup(){ 
    pinMode (led1, OUTPUT);
    pinMode(sensor_enable_pin, OUTPUT);
    digitalWrite(sensor_enable_pin, HIGH);
    blinker();
    Serial.begin(115200); 
    Serial.println();
    SERIAL_PRINTLN("# Serial thermal camera MLX 90621 starting...");
    disable_wifi();
    sensor.initialise (FPS_param);  
    SERIAL_PRINTLN("# MLX Sensor initialized.");
}

void loop(){
    if (!Serial.available())
        return;
    while (Serial.available())
        Serial.read();        // drain the input
    send_data();
}

void disable_wifi() {
    SERIAL_PRINTLN ("# Switching off wifi..");
    WiFi.disconnect(); 
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    delay(100); 
}

char tmp_str[12];     
double tmp_data;
// data will be sent when a request is received through serial port
void send_data() {    
  
    sensor.measure(); // collect pixel data from thermal sensor
    // TODO : implement running average and outlier removal ?

    data[0] = '[';   // start a new packet
    data[1] = '\0';    
    
    for(int y=0; y<ROWS; y++) {       // y = 4 rows
        for(int x=0; x<COLS; x++) {   // x = 16 columns
            tmp_data = sensor.getTemperature(y+x*4); // read the temperature at position (x,y)
            if (isnan(tmp_data) || tmp_data < LOWER_LIMIT || tmp_data > UPPER_LIMIT) {
              // TODO: check if MOST pixels are nan, then reset
              #ifdef VERBOSE_MODE
                SERIAL_PRINT("# ----->>>> PIXEL ERROR: pixel= (");  
                SERIAL_PRINT(y);
                SERIAL_PRINT(",");
                SERIAL_PRINT(x);                                
                SERIAL_PRINT(")  value= ");
                SERIAL_PRINTLN(tmp_data);
              #endif
              tmp_data = 28.0f;  // TODO: replace with neighbouring pixels!            
            }
            // dtostrf(FLOAT, WIDTH, PRECSISION, BUFFER)
            dtostrf(tmp_data,1,2,tmp_str); 
            strcat(data, tmp_str);
            strcat(data, " ");
        }
    }
    strcat(data, "]");
    Serial.println(data);  // the ending newline is needed for the client
}

// TODO: send notification to a listener
void reset_sensor() {
  SERIAL_PRINTLN("# -------------->>>> Sensor error ! resetting...");
  digitalWrite(sensor_enable_pin, LOW);
  delay(2000);
  digitalWrite(sensor_enable_pin, HIGH);
  delay(500);  
  //// TODO: Wire.begin();
  sensor.initialise (FPS_param);
  delay(500);    
  // TODO: send messge to client ?
}

void blinker() {
    for (int i=0; i<5; i++) {
        digitalWrite(led1, LOW); 
        delay(200);
        digitalWrite(led1, HIGH); 
        delay(200);    
    }
}
    
