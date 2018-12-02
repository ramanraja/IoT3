//config.cpp
#include "config.h"
#include "flashHelper.h"

extern void safe_strncpy (char *dest, char *src, int length = MAX_STRING_LENGTH);

Config::Config(){
    SERIAL_PRINTLN ("Creating the Config object.. [Ensure it is a singleton]");
    safe_strncpy (data_prod_url,        DATA_PROD_URL);
    safe_strncpy (data_test_url,        DATA_TEST_URL);  // this will mutate in init()
    safe_strncpy (cmd_prod_url,         CMD_PROD_URL);
    safe_strncpy (cmd_test_url,         CMD_TEST_URL);
    safe_strncpy (firmware_server_url,  FW_SERVER_URL);
    safe_strncpy (version_check_url,    FW_VERSION_URL); 
}

void Config::init() {
    loadDeviceData ();
    char custom_test_url [MAX_STRING_LENGTH];
    sprintf (custom_test_url, "%s%ld", DATA_TEST_URL, device_id);
    safe_strncpy (data_test_url, custom_test_url);    
    SERIAL_PRINTLN ("Test URL to post data: ");
    SERIAL_PRINTLN (custom_test_url);
}

void Config::targetTestUrl () {
    data_url = (char *)data_test_url;
    cmd_url = (char *) cmd_test_url;
    SERIAL_PRINTLN("Now in TEST_MODE:"); 
    SERIAL_PRINT ("Gateway data URL set to: ");
    SERIAL_PRINTLN(data_url);
    SERIAL_PRINT ("Gateway command URL set to: ");
    SERIAL_PRINTLN(cmd_url);    
}
        
void Config::targetProdUrl () {
    data_url = (char *)data_prod_url;
    data_url = (char *)cmd_prod_url;    
    SERIAL_PRINTLN("Now in PRODUCTION_MODE:"); 
    SERIAL_PRINT ("Gateway data URL set to: ");
    SERIAL_PRINTLN(data_url);
    SERIAL_PRINT ("Gateway command URL set to: ");
    SERIAL_PRINTLN(cmd_url); 
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
        SERIAL_PRINTLN("\n*** --------  FATAL ERROR ------------ ***");
        SERIAL_PRINTLN("Could not read device data from flash.");
        device_id = random(100001, 999999);
        group_id = random(100001, 999999);
        relay_enabled = 0;
        buzzer_enabled = 0;
        SERIAL_PRINT("Assuming random device id: ");
        SERIAL_PRINTLN(device_id);
        SERIAL_PRINT("Random group id: ");
        SERIAL_PRINTLN(group_id);      
        SERIAL_PRINT("Taking default static IP address: 192.168.");
        SERIAL_PRINT(ip_high);
        SERIAL_PRINT(".");
        SERIAL_PRINTLN(ip_low);
        return false;           
    } 
    SERIAL_PRINTLN("\Successfully retrieved device data from flash."); 
    device_id = block.lparam1;
    group_id = block.lparam2;
    //0=both disabled, 1=relay enabled, 2=buzzer enabled, 3=both enabled
    relay_enabled = (block.lparam3 & 0x01);
    buzzer_enabled = (block.lparam3 & 0x02);   
    ip_high = block.lparam4;
    ip_low = block.lparam5;
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
    block.lparam1 = device_id;
    block.lparam2 = group_id;
    byte temp = (byte) relay_enabled;
    if (buzzer_enabled)
        temp += 2;
    block.lparam3 = (long)temp;    
    block.lparam4 = ip_high;
    block.lparam5 = ip_low;    
    result = F.writeFlash(block);
    
    if (!result) {
        SERIAL_PRINTLN ("\nFATAL ERROR: Could not reliably write to EEPROM. Aborting.");
        F.end();
        return false;
    }    
    F.commit();
    //yield();
    SERIAL_PRINTLN ("\nReading back memory..\n");
    result = F.readFlash(&block);  // it is only a test read, so reuse the block
    F.end();   
    if (!result) {
        SERIAL_PRINTLN ("\nFATAL ERROR: Could not read from EEPROM. Aborting.");
        return false;
    }
    // the data was already in 'this' object
    SERIAL_PRINTLN ("This configuration has been saved in EEPROM: ");    
    SERIAL_PRINT ("Device ID: ");
    SERIAL_PRINTLN (device_id);     
    SERIAL_PRINT ("Group ID: ");
    SERIAL_PRINTLN (group_id);
    SERIAL_PRINT ("Relay enabled?: ");
    SERIAL_PRINTLN (relay_enabled);
    SERIAL_PRINT ("Buzzer enabled?: ");
    SERIAL_PRINTLN (buzzer_enabled);  
    SERIAL_PRINT ("Static IP : 192.168.");
    SERIAL_PRINT (ip_high);     
    SERIAL_PRINT (".");
    SERIAL_PRINTLN (ip_low);     
    SERIAL_PRINTLN();
    return true;
}

bool Config::repairFlash(const char *config_str) {
    SERIAL_PRINTLN("New Flash data:");
    SERIAL_PRINTLN(config_str);
    long did, gid, rb, high, low;
    int num_ints = sscanf (config_str, "%ld %ld %ld %ld %d", &did,&gid,&rb,&high,&low);
    SERIAL_PRINT("number of integers scanned= ");
    SERIAL_PRINTLN(num_ints);  
    if (num_ints != 5) {
        SERIAL_PRINT("ERROR: Expected 5 integers, but found ");
        SERIAL_PRINTLN(num_ints);
        return false;
    }    
    SERIAL_PRINT("device id= ");
    SERIAL_PRINTLN(did);  
    SERIAL_PRINT("group id= ");
    SERIAL_PRINTLN(gid);  
    SERIAL_PRINT("relay enabled= ");
    SERIAL_PRINTLN(rb & 0x01);  
    SERIAL_PRINT("buzzer enabled= ");
    SERIAL_PRINTLN(rb & 0x02);  
    SERIAL_PRINT ("Static IP : 192.168.");
    SERIAL_PRINT (ip_high);     
    SERIAL_PRINT (".");
    SERIAL_PRINTLN (ip_low); 
    if ((high < 0 || high > 255) || (low < 0 || low > 255)) {
          SERIAL_PRINTLN("Invalid IP octet");
          return false;
    }
    device_id = did;
    group_id = gid;
    relay_enabled = rb & 0x01;
    buzzer_enabled = rb & 0x02;
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
    SERIAL_PRINT ("Device ID: ");
    SERIAL_PRINTLN (device_id);     
    SERIAL_PRINT ("Device Group: ");
    SERIAL_PRINTLN (group_id);       
    SERIAL_PRINT ("Static IP : 192.168.");
    SERIAL_PRINT (ip_high);     
    SERIAL_PRINT (".");
    SERIAL_PRINTLN (ip_low);     
    SERIAL_PRINT ("Relay enabled?: ");
    SERIAL_PRINTLN (relay_enabled);   
    SERIAL_PRINT ("Buzzer enabled?: ");
    SERIAL_PRINTLN (buzzer_enabled); 
    SERIAL_PRINT ("Fire and forget mode?: ");
    SERIAL_PRINTLN (fire_and_forget);
    SERIAL_PRINTLN();    
    
    SERIAL_PRINT ("Firmware server URL: ");
    SERIAL_PRINTLN (firmware_server_url);    
    SERIAL_PRINT("Firmware version URL: ");
    SERIAL_PRINTLN(version_check_url);      
      
    SERIAL_PRINT ("Production Gateway [data]: ");
    SERIAL_PRINTLN (data_prod_url);
    SERIAL_PRINT("Production gateway [command]: ");
    SERIAL_PRINTLN(cmd_prod_url); 
        
    SERIAL_PRINT ("Test Gateway [data]: ");
    SERIAL_PRINTLN (data_test_url);   
    SERIAL_PRINT("Test gateway [command]: ");
    SERIAL_PRINTLN(cmd_test_url);     
    SERIAL_PRINTLN();
    
    SERIAL_PRINTLN("Production host, port, URI [data]: ");
    SERIAL_PRINT(data_host); 
    SERIAL_PRINT(" , ");
    SERIAL_PRINT(data_port);
    SERIAL_PRINT(" , ");    
    SERIAL_PRINTLN(data_resource);
    
    SERIAL_PRINTLN("Production host, port, URI [command]: ");
    SERIAL_PRINT(cmd_host); 
    SERIAL_PRINT(" , ");
    SERIAL_PRINT(cmd_port);
    SERIAL_PRINT(" , ");
    SERIAL_PRINTLN(cmd_resource); 
    SERIAL_PRINTLN("-----------------------------------------\n");                     
}
 

 
