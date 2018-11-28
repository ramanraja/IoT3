// myUdp.cpp

#include "myUdp.h"
  
MyUdp::MyUdp(){
}
 
bool MyUdp::init (Config* configptr){
    this->pC = configptr;
    SERIAL_PRINTLN("Starting UDP server...");
    Udp.stopAll();
    bool result = Udp.begin(local_port);  
    if (!result)
        return false;
    dump();   
    return true;
}

// Non-blocking call: returns the incoming request as string
// if no request, sends a zero-length string

const char* MyUdp::get_request() {
    request_packet[0] = '@';  // precaution ? 
    int packetSize = Udp.parsePacket();
    if (packetSize==0)    // no incoming request
        return ("");    // zero length string indicates no data
 
    remote_IP = Udp.remoteIP();
    remote_port = Udp.remotePort();  
    int len = Udp.read(request_packet, IN_BUFFER_SIZE);
    request_packet[len] = '\0';   // null termination is mandatory to return the pointer!  
    if (len==0)
        return ("");
    
    #ifdef VERBOSE_MODE
      SERIAL_PRINT("packet request: ");
      SERIAL_PRINTLN(request_packet);
    #endif
    return ((const char*)request_packet);
}

void MyUdp::send_data (float *pixel_data, int chunk_num) {

    // NOTE: the receiving Python code expects exactly one hex digit in the chunk number field 
    
    response_packet[0] = '[';   // start a new packet
    response_packet[1] = chunk_num < 10 ? (chunk_num+'0') : (chunk_num-10+'A');    
    response_packet[2] = '|';   // chunk field separator
    response_packet[3] = '\0'; 
    
    int start_pixel = chunk_num * CHUNK_LENGTH;
    for(int i=0; i<CHUNK_LENGTH; i++) {
        // dtostrf(FLOAT, WIDTH, PRECSISION, BUFFER)
        dtostrf(pixel_data[start_pixel+i],1,2,tmp_str); 
        strcat(response_packet, tmp_str);
        strcat(response_packet, " ");
    }  
    strcat(response_packet, "]");
    SERIAL_PRINTLN(response_packet);   
    Udp.beginPacket(remote_IP, remote_port);
    Udp.write(response_packet);
    Udp.endPacket(); 
}

void MyUdp::send_message (const char *msg) {
    SERIAL_PRINTLN(msg);
    Udp.beginPacket(remote_IP, remote_port);
    Udp.write(msg, strlen(msg));
    Udp.endPacket();        
}

void MyUdp::dump (){
    SERIAL_PRINT("Listening at local IP: ");  
    SERIAL_PRINTLN(WiFi.localIP());
    SERIAL_PRINT("on UDP Port: ");
    SERIAL_PRINTLN(local_port);
    SERIAL_PRINT("Default remote IP: ");
    SERIAL_PRINTLN (remote_IP.toString().c_str());
    SERIAL_PRINT ("Default remote port: ");
    SERIAL_PRINTLN(remote_port);    
}


 
 
 
