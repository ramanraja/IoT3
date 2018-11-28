// thermalCam.h

#ifndef THERMALCAM_H
#define THERMALCAM_H

#include "common.h"
#include "config.h"
#include "hardware.h"
#include "myfi.h"
#include "myUdp.h"
#include "flashHelper.h"
#include "myOta.h"
#include <Arduino.h>
#include <stdlib.h>
#include <Wire.h>
//#include <WiFiUdp.h>
#include <ESP8266WiFi.h>
//#include <Timer.h> // https://github.com/JChristensen/Timer 
 

// these should be used on the (Python) client end also:
 
#define NEXT_PROMPT      'N'       // send next chunk of the frame
#define ABORT_PROMPT     'A'       // discard remaining chunks, start a fresh frame
#define RESET_PROMPT     'S'       // sensor reset
#define REBOOT_PROMPT    'R'       // reboot 8266

#endif 
