//config.h
#ifndef CONFIG_H
#define CONFIG_H
 
#include "common.h"
#include "keys.h"

// increment this number for every version
#define  FIRMWARE_VERSION       10

#define  BROADCAST_DEVICE_ID    0
#define  BROADCAST_GROUP_ID     0
#define  BAUD_RATE              115200

//----------------------------- HTTPClient ----------------------------------------

//#define  DATA_PROD_URL        "http://192.168.1.105:8080/status"
#define  DATA_PROD_URL          "http://52.40.239.77:5800/post_occupancy_status"

#define  CMD_PROD_URL           "http://192.168.1.105:8080/command"

#define  DATA_TEST_URL          "http://dweet.io/dweet/for/vz-ind-rmz-occ"   
#define  DATA_TEST_URL2         "http://jsonplaceholder.typicode.com/users";
#define  CMD_TEST_URL           "http://jsonplaceholder.typicode.com/users/7"

//----------------------------- WifiClient ---------------------------------------

#define  DATA_PROD_HOST         "192.168.1.105"
#define  DATA_PROD_RESOURCE     "/status"
#define  DATA_PROD_PORT         8080 
#define  CMD_PROD_HOST          "192.168.1.105"
#define  CMD_PROD_RESOURCE      "/command"
#define  CMD_PROD_PORT          8080

#define  DATA_TEST_HOST         "dweet.io"
#define  DATA_TEST_RESOURCE     "/dweet/for/vz-ind-rmz-occ"
#define  DATA_TEST_PORT         80 
#define  CMD_TEST_HOST          "jsonplaceholder.typicode.com"
#define  CMD_TEST_RESOURCE      "/users/2"
#define  CMD_TEST_PORT          80
//----------------------------------------------------------------------------------
// File names: occupancy.bin  and occupancy.txt
 
//#define  FW_SERVER_URL          "http://192.168.1.105:8080/ota/occupancy.bin"
//#define  FW_VERSION_URL         "http://192.168.1.105:8080/ota/occupancy.txt"

#define  FW_SERVER_URL          "http://52.40.239.77:5050/firmware/download/occupancy/bin"
#define  FW_VERSION_URL         "http://52.40.239.77:5050/firmware/download/occupancy/txt"
//----------------------------------------------------------------------------------
 
class Config {
public :
int  current_firmware_version =  FIRMWARE_VERSION;  

char firmware_server_url [MAX_STRING_LENGTH];
char version_check_url [MAX_STRING_LENGTH];
bool verison_check_enabled = true;

// for HttpPoster
char data_prod_url [MAX_STRING_LENGTH];
char data_test_url [MAX_STRING_LENGTH];
char cmd_prod_url [MAX_STRING_LENGTH]; 
char cmd_test_url [MAX_STRING_LENGTH]; 

// working pointers
char *data_url = (char *)data_prod_url;     
char *cmd_url = (char *)cmd_prod_url;     

// for WifiPoster
bool fire_and_forget = false;

// these cannot be changed at run time (in this version):
char *data_host = DATA_TEST_HOST;
int data_port = DATA_TEST_PORT;
char *data_resource = DATA_TEST_RESOURCE;
char *cmd_host = CMD_TEST_HOST;
int cmd_port = CMD_TEST_PORT;   
char *cmd_resource = CMD_TEST_RESOURCE;
    
// the actual IDs and threshold will be read from EEPROM and plugged in here
long device_id = BROADCAST_DEVICE_ID;      
long group_id  = BROADCAST_GROUP_ID;
bool relay_enabled = false;
bool buzzer_enabled = false;

// * Timer durations SHOULD be unsigned LONG int, if they are > 16 bit! 
unsigned long sensing_interval = 55273L;   // milliSec, to read temperature,humidity etc
unsigned long data_interval    = 60000L;   // milliSec, to send data to gateway
//unsigned long command_interval = 300719L;   // milliSec, to check for commands from gateway
unsigned long sensor_staus_interval = 2000;   // mSec to send the raw status of PIR and radar
unsigned int tick_interval     = 100;      // in milliSec; 10 ticks = 1 second 
unsigned int release_ticks1    = 120*10;   // n*10 ticks = n seconds   // 60
unsigned int release_ticks2    = 480*10;   // n*10 ticks = n seconds   // 600
unsigned int buzzer_ticks1     = 110*10;   // n*10 ticks = n seconds   // 50
unsigned int buzzer_ticks2     = 470*10;   // n*10 ticks = n seconds   // 590

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
bool repairFlash(const char *config_str);
void targetProdUrl ();
void targetTestUrl ();
};  
#endif 
 
