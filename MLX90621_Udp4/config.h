//config.h
#ifndef CONFIG_H
#define CONFIG_H
 
#include "common.h"
#include "keys.h"

// increment this number for every version
#define  FIRMWARE_VERSION       1

#define  BAUD_RATE              115200

// file names: thermalCam.bin and thermalCam.txt
#define  FW_SERVER_URL          "http://52.40.239.77:5050/firmware/thermal_cam/download/bin"
#define  FW_VERSION_URL         "http://52.40.239.77:5050/firmware/thermal_cam/download/txt"
            
class Config {
public :
int  current_firmware_version =  FIRMWARE_VERSION;  
char *firmware_server_url = FW_SERVER_URL;
char *version_check_url = FW_VERSION_URL;
bool verison_check_enabled = true;
   
   
/* The following constants should be updated in  "keys.h" file  */
const char *wifi_ssid1        = WIFI_SSID1;
const char *wifi_password1    = WIFI_PASSWORD1;

Config();
void init();
void dump();
};  
#endif 
 
