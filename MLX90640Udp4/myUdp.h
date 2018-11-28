// myUdp.h

#ifndef MYUDP_H
#define MYUDP_H

#include "common.h"
#include "config.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h> 

#define IN_BUFFER_SIZE   32     // mostly single character commands, some times two integers 
#define OUT_BUFFER_SIZE  512    // enough to send 64 floats in ascii  *****


// parameters for MLX90640 sensor:
// these should be used on the (Python) client side also
          
#define PACKET_SIZE         512     // buffer size to pack the on-air packets and send      
#define NUM_CHUNKS          12      // 768 / 64 = 12 chunks
#define CHUNK_LENGTH        64      // every chunk has 64 floats
#define CHUNKS_PER_FRAME    12      // 768 floats are sent as 12 chunks  
#define FRAME_LENGTH        768     // a frame has 24*32 = 768 pixels

class MyUdp {
public:
    MyUdp();
    bool init (Config* configptr);
    void dump();
    void send_data (float *data_buffer, int chunk_num); 
    void send_message (const char *msg);
    const char* get_request();   // Non-blocking; returns the incoming request string
private:
    Config *pC;
    WiFiUDP Udp;
    char request_packet [IN_BUFFER_SIZE];     
    char response_packet [OUT_BUFFER_SIZE];  
    long local_port = 12345L;      // local UDP server port to listen for data requests   
     
    IPAddress remote_IP = IPAddress(192,168,0,105);   // default values only; later these 
    long remote_port = 54321L;                 // will be overridden by the requestor
    char tmp_str[12];  // scratch pad to convert one float to char
};

#endif 
