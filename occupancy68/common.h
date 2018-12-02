//common.h
#ifndef COMMON_H
#define COMMON_H

#include <Arduino.h>
extern "C" {
  #include "user_interface.h"
}

// comment out this line to disable some informative messages
#define  VERBOSE_MODE 
// comment out this line to disable all serial messages
#define ENABLE_DEBUG

#ifdef ENABLE_DEBUG
  #define  SERIAL_PRINT(x)       Serial.print(x)
  #define  SERIAL_PRINTLN(x)     Serial.println(x)
  #define  SERIAL_PRINTLNF(x,y)  Serial.println(x,y)   
  #define  SERIAL_PRINTF(x,y)    Serial.printf(x,y) 
#else
  #define  SERIAL_PRINT(x)
  #define  SERIAL_PRINTLN(x)
  #define  SERIAL_PRINTLNF(x,y)
  #define  SERIAL_PRINTF(x,y)
#endif

#define  MAX_STRING_LENGTH      128

#define  STATUS_NONE            0
#define  STATUS_OCCUPIED        1
#define  STATUS_FREE            2
#define  STATUS_WARNED1         3
#define  STATUS_WARNED2         4
#define  STATUS_RESTARTED       5
#define  STATUS_SENSOR_ERROR    6
#define  STATUS_EPROM_ERROR     7
#define  STATUS_HTTP_ERROR      8
#define  STATUS_CMD_ACK         9
#define  STATUS_CMD_SUCCESS     10
#define  STATUS_CMD_FAILED      11
#define  STATUS_CHECKING_FW     12

// Tx data structure: the comments are json tags sent to the server
// Battery voltage cannot be included here, as the ADC pin is used for input
struct SensorData {  
   long device_id;              // ID
   long group_id;               // G
   int node_status;             // S
   int status_param;            // P
   int temperature;             // T
   int humidity;                // H
   int heat_index;              // I
   int light;                   // L       
};

// when command parsing fails, the command will be 0. This is sent to the
// null sink 'CMD_HELLO' where it is ignored.

#define  CMD_HELLO                0
#define  CMD_TARGET_PROD_URL      1
#define  CMD_TARGET_TEST_URL      2
#define  CMD_TARGET_OTHER_URL     3
#define  CMD_SET_FW_SERVER_URL    4
#define  CMD_SET_DEVICE_ID        5
#define  CMD_SET_GROUP_ID         6
#define  CMD_SET_EXTRAS           7   
#define  CMD_SET_INTERVAL         8  
#define  CMD_WRITE_FLASH_CONFIG   9 
#define  CMD_UPDATE_FIRMWARE      10
#define  CMD_RESTART              11

#define  CMDX_PARSING_ERROR       21
#define  CMDX_REGULAR_DATA        22
#define  CMDX_EVENT_DATA          23

// CMD_SET_EXTRAS is used for enabling/disabling the relay & buzzer. (for parking sensor, it is the threshold value)
// CMD_SET_INTERVAL is the data sending periodicity. (for parking sensor, it is the sleep duration).
// CMDX_PARSING_ERROR is generated internally by the Json parser//HTTP GET result code.
// CMDX_EVENT_DATA is set in the param when an occupancy status change or sensor event happens.
// CMDX_REGULAR_DATA is a dummy 'command' (status_param) used by the device to indicate it is 
// a routine packet, and not an event packet
  
struct CommandData {  
   long   command;           // C
   long   long_param;        // L 
   char string_param [MAX_STRING_LENGTH];// S      
};

#endif
