/***
Streams data from  MLX 90640 through serial port to PC/Raspberry Pi.
Averages 3 successive frames and sends it to reduce noice.
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

//#define MIN_MAX_ONLY    // send only the min & max temperature of every frame, for plotting
#define NUM_FRAMES_TO_AVERAGE  3     // send the average of this many frames

#define OUT_BUFFER_SIZE   6200    // ASSUMING 768 pixels x 8 characters payload size
#define TA_SHIFT          8       // Default shift for MLX90640 in open air
#define LOWER_LIMIT       0       // to flag sensor error
#define UPPER_LIMIT       100     // to flag sensor error
#define MAX_BAD_PIXELS    10   // if number of nan pixels exceed this, reset the sensor

const byte sensor_address = 0x33; // Default 7-bit unshifted address of the MLX90640
paramsMLX90640  mlx_params;

// Comment out two of the following three options:
//int  FPS_param = 7;   // Refresh rate 64 Hz
//int  FPS_param = 3;   // Refresh rate 4 Hz
int  FPS_param = 2;     // Refresh rate 2 Hz

// Never use D1 and D2 for anything else - they are meant for I2C
int  led1 = 16;  // D0
int  sensor_enable_pin = 12;  // D6 

float min_temp = 28;
float max_temp = 32;
float pixels[768];
float averaged_pixes[768];
uint16_t  mlx_frame[834];
bool read_result = false;

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
    Wire.setClock(400000); //Increase I2C clock speed to 400kHz
    // TODO: if it freezes often, try enabling this
    
    if (!init_sensor())
        reset_sensor();   // just give it one more try

    #ifdef MIN_MAX_ONLY    
        SERIAL_PRINTLN ("# ***** Entering MIN-MAX only mode ******");
    #endif
}

void loop()
{ 
    read_result = read_sensor();   // read 3 packets and average them
    #ifndef MIN_MAX_ONLY        // MIN_MAX_ONLY mode runs in free running mode without prompt
      if (!Serial.available())  // wait for prompt from client
          return;
      while (Serial.available())
          Serial.read();        // drain the input
    #endif
    if (read_result)
        send_data();
    else
        send_error();
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
    delay(3000);  // discharge the capacitor
    digitalWrite(sensor_enable_pin, HIGH);
    delay(500);  
    ////ESP.wdtFeed();
    //// TODO: Wire.begin();  
    init_sensor();
    delay(500);    // let it stabilize
}

// internal helper method
bool read_sensor() {
   for (int j=0; j<768; j++) 
       averaged_pixes[j] = 0.0;   
   for (int i=0; i<NUM_FRAMES_TO_AVERAGE; i++) {
      if (!read_raw_pixels())  // the result is now in pixels[] array
          return false;   // abort reading
      for (int j=0; j<768; j++) 
          averaged_pixes[j] = averaged_pixes[j]+pixels[j];
   }
   for (int j=0; j<768; j++) 
       averaged_pixes[j] = averaged_pixes[j]/NUM_FRAMES_TO_AVERAGE;  
   return true;
}

bool read_raw_pixels() {
    #ifdef VERBOSE_MODE
        long start_time = millis();
    #endif
    for (int page=0 ; page < 2 ; page++)  { // read both sub pages ***
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
    // repair bad pixels
    int bad_pixels = 0;    
    for (int i=0; i< 768 ; i++)  {
        if (isnan(pixels[i]) || pixels[i] < LOWER_LIMIT || pixels[i] > UPPER_LIMIT) {
          #ifdef VERBOSE_MODE
            SERIAL_PRINT("# ----->>>> PIXEL ERROR: pixel= ");  
            SERIAL_PRINT(i);     
            SERIAL_PRINT(",  value= ");
            SERIAL_PRINTLN(pixels[i]);
          #endif
          bad_pixels++;
          if (bad_pixels > MAX_BAD_PIXELS)  {
              reset_sensor();  
              return false;  // discard everything
          }
          pixels[i] = (min_temp+max_temp)/2.0;  // TODO: replace with local average?          
        }
    }    
    find_minimum_temp();
    find_maximum_temp();
    // TODO : implement outlier removal ?
    ESP.wdtFeed(); 
    // yield();    
    return true;
}

double tmp_data;  
char tmp_str[12];     
char data[OUT_BUFFER_SIZE];  

void send_data() {    
  #ifdef MIN_MAX_ONLY
      data[0] = '\0';             // sprintf() does not support floats!
      dtostrf(min_temp,1,2,tmp_str); 
      strcat(data, tmp_str);
      strcat(data, " ");   
      dtostrf(max_temp,1,2,tmp_str);   
      strcat(data, tmp_str);
      Serial.println(data);       
  #else
      data[0] = '[';   // start a new packet
      data[1] = '\0';     
      for (int i=0 ; i< 768 ; i++)  {
          tmp_data = averaged_pixes[i];
          // dtostrf(FLOAT, WIDTH, PRECSISION, BUFFER)
          dtostrf(tmp_data,1,2,tmp_str); 
          strcat(data, tmp_str);
          strcat(data, " ");
      }    
      strcat(data, "]");
      Serial.println(data);  // the ending newline is needed for the client   
  #endif
}

void send_error() {
  SERIAL_PRINTLN("# ---- Sensor Error ----");  
}

void  find_minimum_temp() {
  min_temp = pixels[0];  // global
  for (int i=1; i<768; i++) {
      if (!isnan(pixels[i]) && pixels[i] < min_temp)
          min_temp = pixels[i];
  }
}

void  find_maximum_temp() {
  max_temp = pixels[0];  // global
  for (int i=1; i<768; i++)
      if (!isnan(pixels[i]) && pixels[i] > max_temp)
          max_temp = pixels[i]; 
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

