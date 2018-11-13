// MLX90621_Udp.h

#ifndef MLX90621_UDP_H
#define MLX90621_UDP_H

#include "common.h"
#include "config.h"
#include "otaHelper.h"
#include <stdlib.h>
#include <Arduino.h>
#include "MLX90621.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Timer.h>

#define ROWS              4
#define COLS              16
#define DATA_LENGTH       64   // 4*16
#define IN_BUFFER_SIZE    32
#define OUT_BUFFER_SIZE   512  // ASSUMING 64*8 characters payload size
#define DATA_TIMEOUT      10   // x seconds*2 
#define LOWER_LIMIT       0    // to flag sensor error
#define UPPER_LIMIT       100  // to flag sensor error

#endif 
