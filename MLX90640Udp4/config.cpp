//config.cpp
#include "config.h"
#include "flashHelper.h"

extern void safe_strncpy (char *dest, char *src, int length = MAX_STRING_LENGTH);

Config::Config(){
    SERIAL_PRINTLN ("Creating the Config object.. [Ensure it is a singleton]");
    safe_strncpy (firmware_server_url,  FW_SERVER_URL);
    safe_strncpy (version_check_url,    FW_VERSION_URL); 
}

void Config::init() {
    loadDeviceData ();
}

/* 
   Read the device id and group id from Flash and
    embed them into the Config object. This is to be done before
   initializing the wireless and http helpers.
*/
bool Config::loadDeviceData () {
    FlashHelper F;
    F.init(this);
    F.begin();
    SERIAL_PRINTLN("Trying to read EEPROM...");
    
    StorageBlock block;
    bool result = F.readFlash (&block);   
    F.end();

    if (!result) {
        SERIAL_PRINTLN("\n --------  FATAL ERROR ------------ ");
        SERIAL_PRINTLN("Could not read device data from flash.");
//        ip_high = 0;    // use the ones already specified in header file    
//        ip_low = 100;
        SERIAL_PRINT("Taking default static IP address: 192.168.");
        SERIAL_PRINT(ip_high);
        SERIAL_PRINT(".");
        SERIAL_PRINTLN(ip_low);
        return false;  
    }   
    ip_high = block.lparam1;
    ip_low = block.lparam2;
    SERIAL_PRINTLN("\Successfully retrieved device data from flash."); 
    return true; 
}

bool Config::storeDeviceData() {
    SERIAL_PRINTLN ("\nSaving the Configuration to EEPROM..");
    FlashHelper F;
    F.init(this);
    F.begin();
    bool result = F.testMemory();
    if (!result) {
        SERIAL_PRINTLN ("Basic Memory Test failed !.. aborting.");
        F.end();
        return false;
    }
    //yield();
    StorageBlock block;
    block.lparam1 = ip_high;
    block.lparam2 = ip_low;
    block.lparam3 = 0;   // don't store garbage !
    block.lparam4 = 0;
    block.lparam5 = 0;
    result = F.writeFlash(block);
    if (!result) {
        SERIAL_PRINTLN ("\nFATAL ERROR: Could not reliably write to EEPROM. Aborting.");
        F.end();
        return false;
    }    
    F.commit();
    //yield();
    SERIAL_PRINTLN ("\nReading back memory..\n");
    StorageBlock dummy;  // we only need the boolean result
    result = F.readFlash(&dummy);
    F.end();   
    if (!result) {
        SERIAL_PRINTLN ("\nFATAL ERROR: Could not read from EEPROM. Aborting.");
        return false;
    }
    // data is now stored back in 'this' object
    SERIAL_PRINTLN ("This configuration has been saved in EEPROM: ");    
    SERIAL_PRINT ("Static IP : 192.168.");
    SERIAL_PRINT (ip_high);     
    SERIAL_PRINT (".");
    SERIAL_PRINTLN (ip_low); 
    SERIAL_PRINTLN();
    return true;
}

bool Config::repairFlash (const char *config_str) {
  SERIAL_PRINT("New Flash data: ");
  SERIAL_PRINTLN(config_str);
  long high, low;  // this can come from Flash long params
  int num_ints = sscanf (config_str, "%ld %ld", &high, &low);
  SERIAL_PRINT("number of integers scanned= ");
  SERIAL_PRINTLN(num_ints);  
  if (num_ints != 2) {
      SERIAL_PRINT("ERROR: Expected 2 integers, but found ");
      SERIAL_PRINTLN(num_ints);
      return false;
  }
  SERIAL_PRINT("IP high octet= ");
  SERIAL_PRINTLN(high);  
  SERIAL_PRINT("IP low octet= ");
  SERIAL_PRINTLN(low);  
  if ((high < 0 || high > 255) || (low < 0 || low > 255)) {
        SERIAL_PRINT("Invalid IP octet");
        return false;
  }
  ip_high = high;
  ip_low = low;

  return (storeDeviceData());  // manually restart after this
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
        
    SERIAL_PRINT ("Static IP address: 192.168.");
    SERIAL_PRINT (ip_high);     
    SERIAL_PRINT (".");
    SERIAL_PRINTLN (ip_low);       
 
    SERIAL_PRINT ("Firmware server URL: ");
    SERIAL_PRINTLN (firmware_server_url);    
    SERIAL_PRINT("Firmware version URL: ");
    SERIAL_PRINTLN(version_check_url);      
      
    SERIAL_PRINTLN("-----------------------------------------\n");                     
}
 

 
