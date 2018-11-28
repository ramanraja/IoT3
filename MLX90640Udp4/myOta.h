// myOta.h
#ifndef MYOTA_H
#define MYOTA_H

#include "common.h"
#include "config.h"
#include <ESP8266HTTPClient.h>
#include <ESP8266httpUpdate.h>

class MyOta {
 public:
    MyOta();
    void init(Config *configptr);
    int check_and_update();
    bool check_version();    
    int update_firmware();  
 private:
     Config *pC;
};

#endif 
