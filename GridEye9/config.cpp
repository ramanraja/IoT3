//config.cpp
#include "config.h"

Config::Config(){
    SERIAL_PRINTLN ("Creating the Config object.. [Ensure it is a singleton]");
}

void Config::init() {
}

void Config::dump() {
    SERIAL_PRINTLN("\n-----------------------------------------");
    SERIAL_PRINTLN("               configuration             ");
    SERIAL_PRINTLN("-----------------------------------------");    
    SERIAL_PRINT ("Firmware version: 1.0.");
    SERIAL_PRINTLN (FIRMWARE_VERSION);
    long freeheap = ESP.getFreeHeap();
    SERIAL_PRINT ("Free heap: ");
    SERIAL_PRINTLN (freeheap);    
    
    SERIAL_PRINT ("Firmware server URL: ");
    SERIAL_PRINTLN (firmware_server_url);    
    SERIAL_PRINT("Firmware version URL: ");
    SERIAL_PRINTLN(version_check_url);      
      
    SERIAL_PRINTLN("-----------------------------------------\n");                     
}
 

 
