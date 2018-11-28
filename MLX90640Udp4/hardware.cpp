// hardware.cpp

#include "common.h"
#include "hardware.h"

Hardware::Hardware(){
}

bool Hardware::init (Config* configptr){
    this->pC = configptr;

    pinMode (led1, OUTPUT);
    pinMode(sensor_enable_pin, OUTPUT);
    digitalWrite(sensor_enable_pin, HIGH);
    Wire.begin();
    Wire.setClock(400000); //Increase I2C clock speed to 400kHz
    // TODO: if it freezes often, try enabling/disabling this
    blinker();
    led_flag = HIGH;
    //////return (begin_sensor());   // TODO: enable this
    return true;
}

//Returns true if the MLX90640 is detected on the I2C bus
bool Hardware::is_sensor_present() {
    Wire.beginTransmission((uint8_t)sensor_address);
    if (Wire.endTransmission() != 0)
        return (false); //Sensor did not ACK
    return (true);
}

bool Hardware::begin_sensor() {
    if (!is_sensor_present()) {
        SERIAL_PRINTLN("# MLX90640 not detected at I2C address 0x33 ! aborting..");
        return (false);
    }
    SERIAL_PRINTLN("# Found MXL90640 at I2C address 0x33.");
    
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

// make a copy of the pixels[] array. Called when a fresh frame (chunk=0) is started.
// we will send all the chunks of the current frame to the client from this cache.
void Hardware::refresh_cached_frame() {
    for (int i=0; i<PIXEL_COUNT; i++)
      cached_frame[i] = pixels[i];
}
    
// returns true if successfully read the sensor. Attempts to reset once.    
bool Hardware::read_sensor() {
    read_pixels();
    if (repair_pixels())
        return true;
        
    reset_sensor();   // try once more
    
    read_pixels();
    return (repair_pixels());
}

// internal method: stores sensor data in the array pixels[]  
void Hardware::read_pixels() {
    /***----------------------------- 
    // simulate the pixel data  
    for (int i=0; i<PIXEL_COUNT; i++)  
        pixels[i] = float(i % 100)+0.25;
    pixels[37] = -123.0;  // an error
    ------------------------------***/
    
    #ifdef VERBOSE_MODE
        long start_time = millis();
    #endif
    // read both sub pages ***
    for (int page=0 ; page < 2 ; page++)  { 
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

// internal method    
bool Hardware::repair_pixels() {
    // repair bad pixels
    int bad_pixels = 0;    
    for (int i=0; i<PIXEL_COUNT ; i++)  {
        if (isnan(pixels[i]) || pixels[i] < LOWER_LIMIT || pixels[i] > UPPER_LIMIT) {
          /***---------------------------------------------
          #ifdef VERBOSE_MODE
             SERIAL_PRINT("# PIXEL ERROR: pixel= ");  
             SERIAL_PRINT(i);     
             SERIAL_PRINT(",  value= ");
             SERIAL_PRINTLN(pixels[i]);
          #endif
          ---------------------------------------------***/
          bad_pixels++;
          if (bad_pixels > MAX_BAD_PIXELS)  {
              SERIAL_PRINTLN("# --- Too many bad pixels ---");
              return false;  // discard the frame
          }
          pixels[i] = (min_temp+max_temp)/2.0;  
          // TODO: replace with mean/median/local average          
        }
    }      
    find_minimum();
    find_maximum();
    // TODO : implement outlier removal      
    // ESP.wdtFeed(); 
    // yield();
    return true;    
}

void Hardware::find_minimum() {
  min_temp = 500;  // global variable
  for (int i=0; i<PIXEL_COUNT; i++) {
      if (!isnan(pixels[i]) && pixels[i] < min_temp)
          min_temp = pixels[i];
  }
}

void  Hardware::find_maximum() {
  max_temp = 0;  // global variable
  for (int i=0; i<PIXEL_COUNT; i++)
      if (!isnan(pixels[i]) && pixels[i] > max_temp)
          max_temp = pixels[i]; 
}
    
void Hardware:: reset_sensor(){
      SERIAL_PRINTLN("# H:Resetting sensor...");  
      digitalWrite(sensor_enable_pin, LOW);
      delay(5000);   //  let the capacitor discharge ?
      digitalWrite(sensor_enable_pin, HIGH);
      delay(500);  
      //// TODO: Wire.begin()?
      if (begin_sensor()) 
          SERIAL_PRINTLN("# Sensor reset completed.");
      else
          SERIAL_PRINTLN("# Could not rest sensor !");  
      delay(500); // let sensor boot up        
}

void Hardware::restart_ESP() {
    SERIAL_PRINTLN("\n*** ESP is about to restart ! ***");
    delay(1000);
    ESP.restart();
}

 void Hardware::toggle_led() {
    digitalWrite(led1, led_flag); 
    led_flag = !led_flag;
} 

 void Hardware::blinker() {
    for (int i=0; i<5; i++) {
        digitalWrite(led1, LOW); 
        delay(200);
        digitalWrite(led1, HIGH); 
        delay(200);    
    }
}
 
