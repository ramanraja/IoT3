// hardware.h

#ifndef HARDWARE_H
#define HARDWARE_H

#include "common.h"
#include "config.h"
//#include "occupancy.h"
#include <Timer.h> // https://github.com/JChristensen/Timer
#include "DHT.h"   // https://github.com/adafruit/DHT-sensor-library
// (Delete the files DHT_U.h and  DHT_U.cpp to avoid dependency problems.)

// relays are active high
#define RELAY_ON   1
#define RELAY_OFF  0
// buzzer is active low
#define BUZZER_ON  0
#define BUZZER_OFF 1

#define BEEP_DURATION  200

class Hardware {
public:
    bool pir_status;
    bool radar_status;
    int temperature, humidity, heat_index, light;    
    
    Hardware();
    void init(Config* configptr);
    void restartEsp();
    void readPM();
    void readTHL();
    void blinker(); // imposes a random startup delay
    void switchLightsOn();
    void switchLightsOff();    
    void warn(Timer& T);  // Important: it is passed by reference
    void beep(Timer& T);
    
private:
    Config *pC;
    const byte led1 = 2;        // D4
    const byte led2 = 16;       // D0
    const byte relay = 4;       // D2  CN1
    const byte radar = 14;      // D5  CN2
    const byte pir = 13;        // D7  CN4
    const byte ldr = A0;        // --  P3
    const byte buzzer = 5;      // D1
    /////const byte dht = 12;   // D6  CN3    // library quirk!
};

#endif 
