// AMG88_Udp.h

#ifndef AMG88_UDP_H
#define AMG88_UDP_H

#include "common.h"
#include "config.h"
#include "otaHelper.h"
#include <stdlib.h>
#include <Wire.h>
#include <WiFiUdp.h>
#include <Timer.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "Adafruit_AMG88xx.h" // https://github.com/adafruit/Adafruit_AMG88xx

#define ROWS              8
#define COLS              8
#define DATA_LENGTH       64   // 4*16
#define IN_BUFFER_SIZE    32 
#define OUT_BUFFER_SIZE   512  // ASSUMING 64*8 characters payload size
#define LOWER_LIMIT       0    // to flag sensor error
#define UPPER_LIMIT       100  // to flag sensor error

#endif 
