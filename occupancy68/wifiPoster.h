// wifiPoster.h

#ifndef WIFI_POSTER_H
#define WIFI_POSTER_H

#include "common.h"
#include "config.h"
#include "myfi.h"
// http://arduino.esp8266.com/versions/2.4.1/package_esp8266com_index.json
#include <ESP8266WiFi.h>        
#include <ESP8266WiFiMulti.h>
 
class WifiPoster {
public:
    WifiPoster ();
    void init(Config *configptr);
    int sendStatus (const char *payload);
    int checkForCommand();
    const char *getCommand();
    ///////int getResponseCode();  // TODO: implement this
    
private:
    void makeHeaders();
    void basic_print_response (WiFiClient wifi_client);   
    void print_response (WiFiClient wifi_client);    
    void advanced_print_response (WiFiClient wifi_client);
    
    MyFi   W;
    Config *pC;  
    
    int  HTTP_TIMEOUT = 3000;    // milliSec
    int  DIY_TIMEOUT = 50;       // in 100 mSec units
    bool PH = false;             // true: print HTTP headers; false: don't print
    
    ////////int reponse_code = 0;
    char command_string  [MAX_STRING_LENGTH];   // command cache  
    
    char data_header [2 * MAX_STRING_LENGTH];  // NOTE: twice the usual allocation is needed
    char final_header [MAX_STRING_LENGTH];    
    char cmd_header [MAX_STRING_LENGTH]; 

};
 
#endif 
