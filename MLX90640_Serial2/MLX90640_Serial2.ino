/***
Streams data from  MLX 90640 through serial port to PC/Raspberry Pi.
Sends it as a series of individual numbers delimited by [ and ] with CR-LF.
OTA update included.
If the sensor reading is NAN or is out of bounds, resets the sensor through a 2N2222.
test it with UdpReceiver5.py or  https://packetsender.com/
production python code: thermalCamXX.py  
Original code :
https://github.com/sparkfun/SparkFun_MLX90640_Arduino_Example
The driver files were modified to accommodate 8266/Arduino based I2C interface.

Connection Instructions:
Connect I2C Data (SDA = D2 = GPIO 4) and Clock (SCK = D1 = GPIO 5) Pins of the
    Sensor with 4.7k - 10 K Pullup reistors
Connect the VDD Pin of the Sensor to 3.3V  (23 mA load)
Connect the VSS/Gnd Pin of the Sensor to GND. This pin is attached to the metal body.
Connect a 0.1 MF ceramic Capacitor between VDD and Gnd.
Sometimes the sensor freezes, so you may need  a transistor to reset it.
***/

#include "common.h"
#include <Wire.h>
#include <ESP8266WiFi.h>
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"

#define OUT_BUFFER_SIZE   6200    // ASSUMING 768 pixels x 8 characters payload size
#define TA_SHIFT          8       // Default shift for MLX90640 in open air
#define LOWER_LIMIT       0       // to flag sensor error
#define UPPER_LIMIT       100     // to flag sensor error

const byte sensor_address = 0x33; // Default 7-bit unshifted address of the MLX90640
paramsMLX90640  mlx_params;

// Comment out two of the following three options:
//int  FPS_param = 7;   // Refresh rate 64 Hz
//int  FPS_param = 3;   // Refresh rate 4 Hz
int  FPS_param = 2;     // Refresh rate 2 Hz

// Never use D1 and D2 for anything else - they are meant for I2C
int  led1 = 16;  // D0
int  sensor_enable_pin = 12;  // D6 
bool led_flag = 0;

void setup()
{
    pinMode (led1, OUTPUT);
    blinker();
    pinMode(sensor_enable_pin, OUTPUT);
    digitalWrite(sensor_enable_pin, HIGH);
    Serial.begin(115200);  
    SERIAL_PRINTLN("\n# MLX90640 thermal sensor starts..");    
    disable_wifi();
    
    Wire.begin();
    //Wire.setClock(400000); //Increase I2C clock speed to 400kHz
    // TODO: if it freezes often, try enabling this
    
    if (!init_sensor())
        reset_sensor();   // just give it one more try
}

void loop()
{
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
    delay(10); 
}

bool init_sensor() {
    if (isConnected() == false) {
        SERIAL_PRINTLN("# MLX90640 not detected at I2C address 0x33. Please check the wiring.");
        return (false);
    }
    SERIAL_PRINTLN("# Found MXL90640 at port 0x33.");
    
    //Get device parameters - We only have to do this once
    int status;
    uint16_t prom_params [832];
    status = MLX90640_DumpEE (sensor_address, prom_params);
    if (status != 0) {
        SERIAL_PRINTLN("# Failed to load system parameters.");
        return (false);
    }
    SERIAL_PRINTLN("# Loaded system parameters from device PROM.");
        
    status = MLX90640_ExtractParameters (prom_params, &mlx_params);
    if (status != 0) {
        SERIAL_PRINTLN("# Parameter extraction failed.");
        return (false);
    }
    //Once params are extracted, we can release prom_params array
    SERIAL_PRINTLN("# Successfully extracted parameters.");
    
    SERIAL_PRINTLN("# Setting refresh rate..");
    MLX90640_SetRefreshRate(sensor_address, FPS_param);   
    SERIAL_PRINTLN("# MLX90640 is ready.");    
    return (true);
}

void reset_sensor() {
  SERIAL_PRINTLN("# -------------->>>> Sensor error ! resetting...");
  digitalWrite(sensor_enable_pin, LOW);
  delay(2000);
  digitalWrite(sensor_enable_pin, HIGH);
  delay(500);  
  //// TODO: Wire.begin();  
  init_sensor();
  delay(500);    
}

// internal helper method
float pixels[768];
uint16_t  mlx_frame[834];
        
void read_sensor() {
    #ifdef VERBOSE_MODE
        long start_time = millis();
    #endif
    for (int page=0 ; page < 2 ; page++)  // read both sub pages ***
    {
        /////uint16_t  mlx_frame[834];
        int status = MLX90640_GetFrameData (sensor_address, mlx_frame);
        float vdd = MLX90640_GetVdd (mlx_frame, &mlx_params);
        float Ta = MLX90640_GetTa (mlx_frame, &mlx_params);
        float tr = Ta - TA_SHIFT; // Reflected temperature based on ambient temperature
        float emissivity = 0.95;
        MLX90640_CalculateTo (mlx_frame, &mlx_params, emissivity, tr, pixels);
    }
    #ifdef VERBOSE_MODE
        long stop_time = millis();
        SERIAL_PRINT("# Sensor read time (mSec): ");
        SERIAL_PRINTLN (stop_time - start_time);
    #endif
}
 
char tmp_str[12];     
double tmp_data; 
char data[OUT_BUFFER_SIZE];  

void send_data() {    
  
    read_sensor();  
    // TODO : implement running average and outlier removal ?
    
    data[0] = '[';   // start a new packet
    data[1] = '\0';     
    
    for (int x = 0 ; x < 768 ; x++)  {
        tmp_data = pixels[x];
        if (isnan(tmp_data) || tmp_data < LOWER_LIMIT || tmp_data > UPPER_LIMIT) {
          // TODO: check if MOST pixels are nan, then reset
          #ifdef VERBOSE_MODE
            SERIAL_PRINT("# ----->>>> PIXEL ERROR: pixel= ");  
            SERIAL_PRINT(x);     
            SERIAL_PRINT(",  value= ");
            SERIAL_PRINTLN(tmp_data);
          #endif
          tmp_data = 28.0f;  // TODO: replace with neighbouring pixels!            
        }
        // dtostrf(FLOAT, WIDTH, PRECSISION, BUFFER)
        dtostrf(tmp_data,1,2,tmp_str); 
        strcat(data, tmp_str);
        strcat(data, " ");
    }    
    strcat(data, "]");
    Serial.println(data);  // the ending newline is needed for the client    
}

//Returns true if the MLX90640 is detected on the I2C bus
boolean isConnected() {
    Wire.beginTransmission((uint8_t)sensor_address);
    if (Wire.endTransmission() != 0)
        return (false); //Sensor did not ACK
    return (true);
}

void blinker() {
    for (int i=0; i<5; i++) {
        digitalWrite(led1, LOW); 
        delay(100);
        digitalWrite(led1, HIGH); 
        delay(100);    
    }
}

