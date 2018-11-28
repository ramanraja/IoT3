// myUdp.h

#ifndef HARDWARE_H
#define HARDWARE_H

#include "common.h"
#include "config.h"
#include <Wire.h>
#include "MLX90640_API.h"
#include "MLX90640_I2C_Driver.h"

/***
Original drivers and application example for MLX90640 :
https://github.com/sparkfun/SparkFun_MLX90640_Arduino_Example
The driver files were modified to accommodate 8266/Arduino based I2C interface.
Connection Instructions:
Connect I2C Data (SDA = D2 = GPIO 4) and Clock (SCK = D1 = GPIO 5) Pins of the
    Sensor with 4.7k Pullup reistors
Connect the VDD Pin of the Sensor to 3.3V  (23 mA load)
Connect the VSS/Gnd Pin of the Sensor to GND. (This pin is attached to its metal body).
Connect a 0.1 MF ceramic Capacitor and a 1 MFD capacitor between VDD and Gnd.
Sometimes the sensor freezes, so you may need  a transistor 2N2222 to reset it.
***/

#define ROWS                24
#define COLS                32
#define PIXEL_COUNT         768    // 24*32

#define IN_BUFFER_SIZE      64     // mostly single character commands, occasionally test strings
#define OUT_BUFFER_SIZE     512    // ASSUMING 64*8 characters payload size

#define MAX_BAD_PIXELS      10     // if number of nan pixels exceed this, reset the sensor
#define LOWER_LIMIT         0      // to flag sensor error
#define UPPER_LIMIT         100    // to flag sensor error

class Hardware {
public:
    Hardware();
    float cached_frame [PIXEL_COUNT];  // treat it as read-only *  
    bool init (Config* configptr);
    bool is_sensor_present();
    bool begin_sensor();
    bool read_sensor();
    void reset_sensor();
    void blinker();
    void toggle_led();
    void restart_ESP();
    void find_minimum();
    void find_maximum();
    void refresh_cached_frame();
private:
    Config *pC;
    float pixels [PIXEL_COUNT];        
    bool led_flag;
    bool read_result = false;    
    float min_temp = 28;  
    float max_temp = 35;

    uint16_t  mlx_frame[834];
    int NUM_FRAMES_TO_AVERAGE = 3;    // average 3 frames to reduce noise ?
    const byte sensor_address = 0x33;   // Default 7-bit unshifted address of the MLX90640
    paramsMLX90640  mlx_params;
    int TA_SHIFT = 8;                   // Default shift for MLX90640 in open air
    
    // Comment out two of the following three options:
    //int  FPS_param = 7;   // Refresh rate 64 Hz
    //int  FPS_param = 3;   // Refresh rate 4 Hz
    int  FPS_param = 2;     // Refresh rate 2 Hz

    // Never use D1 and D2 for anything else - they are meant for I2C
    const byte led1 = 2;                // D4
    const byte led2 = 16;               // D0
    const byte  sensor_enable_pin = 12; // D6 
    
    void read_pixels();
    bool repair_pixels();
};

#endif 
