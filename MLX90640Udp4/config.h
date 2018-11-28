//config.h
#ifndef CONFIG_H
#define CONFIG_H
 
#include "common.h"
#include "keys.h"

// increment this number for every version
#define  FIRMWARE_VERSION       5

#define  BAUD_RATE              115200

// File names: thermalCam.bin and thermalCam.txt

//#define  FW_SERVER_URL        "http://192.168.1.105:8080/ota/thermal24.bin"
//#define  FW_VERSION_URL       "http://192.168.1.105:8080/ota/thermal24.txt"
//#define  FW_SERVER_URL          "http://52.40.239.77:5050/fw-download/thermal/thermalCam.bin"
//#define  FW_VERSION_URL         "http://52.40.239.77:5050/fw-download/thermal/thermalCam.txt"

#define  FW_SERVER_URL          "http://s3-us-west-2.amazonaws.com/fw-download/thermal/thermalCam.bin"
#define  FW_VERSION_URL         "http://s3-us-west-2.amazonaws.com/fw-download/thermal/thermalCam.txt"

//----------------------------------------------------------------------------------
 
class Config {
public :
int  current_firmware_version =  FIRMWARE_VERSION;  

char firmware_server_url [MAX_STRING_LENGTH];
char version_check_url [MAX_STRING_LENGTH];
bool verison_check_enabled = true;

/* The following constants should be updated in  "keys.h" file  */
const char *wifi_ssid1        = WIFI_SSID1;
const char *wifi_password1    = WIFI_PASSWORD1;
const char *wifi_ssid2        = WIFI_SSID2;
const char *wifi_password2    = WIFI_PASSWORD2;
const char *wifi_ssid3        = WIFI_SSID3;
const char *wifi_password3    = WIFI_PASSWORD3;

int ip_high = 1;      // Eg: 192.168.1.110
int ip_low = 110;     // last two octets only

Config();
void init();
void dump();
bool loadDeviceData (); 
bool storeDeviceData();
bool repairFlash (const char *config_str);
};  
#endif 
 
