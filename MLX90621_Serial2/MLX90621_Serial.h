// MLX90621_Serial.h

#ifndef MLX90621_UDP_H
#define MLX90621_UDP_H

#include <stdlib.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include "MLX90621.h"
#include "common.h"

#define ROWS              4
#define COLS              16
#define DATA_LENGTH       64   // 4*16
#define OUT_BUFFER_SIZE   512  // ASSUMING 64*8 characters payload size
#define LOWER_LIMIT       0    // to flag sensor error
#define UPPER_LIMIT       100  // to flag sensor error

#endif 
