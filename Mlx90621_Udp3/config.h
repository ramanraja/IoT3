//config.h
#ifndef CONFIG_H
#define CONFIG_H
 
#include "common.h"
#include "keys.h"

// increment this number for every version
#define  FIRMWARE_VERSION       1

#define  BAUD_RATE              115200

#define  FW_SERVER_URL          "http://52.40.239.77:5050/firmware/download3/bin"
#define  FW_VERSION_URL         "http://52.40.239.77:5050/firmware/download3/txt"
//////#define  UDP_SERVER_IP          "192.168.0.109"
#define  UDP_PORT               12345L
 
class Config {
public :
int  current_firmware_version =  FIRMWARE_VERSION;  
char *firmware_server_url = FW_SERVER_URL;
char *version_check_url = FW_VERSION_URL;
bool verison_check_enabled = true;
   
char *udp_server_ip = UDP_SERVER_IP;      // static IP of this sensor device
long udp_port = UDP_PORT;                 // local UDP port to listen for data requests
   
/* The following constants should be updated in  "keys.h" file  */
const char *wifi_ssid1        = WIFI_SSID1;
const char *wifi_password1    = WIFI_PASSWORD1;
const char *wifi_ssid2        = WIFI_SSID2;
const char *wifi_password2    = WIFI_PASSWORD2;
const char *wifi_ssid3        = WIFI_SSID3;
const char *wifi_password3    = WIFI_PASSWORD3;

Config();
void init();
void dump();
};  
#endif 
 
