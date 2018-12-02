// httpPoster.h

#ifndef HTTP_POSTER_H
#define HTTP_POSTER_H

#include "common.h"
#include "config.h"
#include "myfi.h"
#include <ESP8266WiFi.h>        
// http://arduino.esp8266.com/versions/2.4.1/package_esp8266com_index.json
#include <ESP8266HTTPClient.h>  
// https://github.com/esp8266/Arduino/tree/master/libraries/ESP8266HTTPClient
 
class HttpPoster {
public:
    HttpPoster();
    void init(Config *configptr, MyFi *myfiptr);
    int sendStatus (const char *payload);
    int sendStatus (const char *payload, const char *url);    
    int checkForCommand();
    const char *getCommand();
    int getResponseCode();
    
private:
    MyFi   *pW;
    Config *pC;  
    int reponse_code = 0;
    char command_string  [MAX_STRING_LENGTH];      
};
 
#endif 
