// wifiPoster.cpp

#include "wifiPoster.h"
 
extern void safe_strncpy (char *dest, char *src, int length = MAX_STRING_LENGTH);
 
WifiPoster ::WifiPoster (){
}

void WifiPoster::init(Config *configptr){
    SERIAL_PRINTLN("WifiPoster: initializing...");
    this->pC = configptr;
    W.init(configptr);
    makeHeaders();
}

void WifiPoster::makeHeaders() {
    sprintf (data_header, "POST %s  HTTP/1.1\r\nHost: %s\r\n", pC->data_resource, pC->data_host);
    strcat (data_header, "Content-Type: application/json; charset=utf-8\r\n");
    strcat (data_header, "Connection: close\r\n"); 
    // later we will add the content length header to this
    SERIAL_PRINTLN("HTTP POST header: ");
    SERIAL_PRINT(data_header);
    SERIAL_PRINTLN("///////////////////");
    SERIAL_PRINT("length of POST header: ");
    SERIAL_PRINTLN(strlen(data_header));
    
    sprintf (cmd_header, "GET %s  HTTP/1.1\r\nHost: %s\r\n", pC->cmd_resource, pC->cmd_host);
    strcat (cmd_header, "Connection: close\r\n\r\n");  // Note the protocol empty line
    SERIAL_PRINTLN("HTTP GET header: ");
    SERIAL_PRINT(cmd_header);
    SERIAL_PRINTLN("///////////////////");
    SERIAL_PRINT("length of GET header: ");
    SERIAL_PRINTLN(strlen(cmd_header));    
}

// this has to be called immediately after checkForCommand() to retrieve the result
const char* WifiPoster::getCommand(){
    return (command_string);
}

 /**
// TODO parse the first line of HTTP response and fill in this
int WifiPoster::getResponseCode() {
    return (response_code);
} 
**/

int WifiPoster::sendStatus (const char *payload){
  if (!W.isConnected()) {
    SERIAL_PRINTLN("WifiPoster: No Wi-Fi connection");
    W.reconnect();   // this is essential for WifiMulti !
    return 1;
  }
  SERIAL_PRINTLN("WifiPoster: Posting data to Gateway...");
  
  WiFiClient wifi_client;
  wifi_client.setTimeout (HTTP_TIMEOUT);
  wifi_client.setNoDelay(true);  // disable Nagle algorithm
    bool connected = wifi_client.connect(pC->data_host, pC->data_port);
    if (!connected) {
        wifi_client.stop();
        delay(1000);  // https://github.com/esp8266/Arduino/issues/722
        connected = wifi_client.connect(pC->data_host, pC->data_port);
    }
    if (!connected) {
        SERIAL_PRINTLN ("WifiPoster: Could not connect to HTTP server.");
        wifi_client.stop();
        return 2;
    }   
    sprintf(final_header, "Content-length: %d\r\n\r\n", strlen(payload));
    wifi_client.print(data_header);
    wifi_client.print(final_header); // this has the protocol blank line also
    
    wifi_client.print(payload);  // send it off
    wifi_client.flush();
    
    if (pC->fire_and_forget) {
       SERIAL_PRINTLN("WifiPoster: FAF mode; Ending HTTP connection..");
       SERIAL_PRINTLN("");  
       delay(200);  // flushing - safety margin?
       wifi_client.stop();   // close connection
       return 3;  // result unknown
    }
    
    int timeout = DIY_TIMEOUT;  // Eg: 50*100 = 5 sec
    while (wifi_client.connected() && !wifi_client.available()) {
        delay(100);  // yield time to WiFi to do its job
        if (--timeout <= 0) {
            SERIAL_PRINTLN("WifiPoster: HTTP Server timed out !");
            wifi_client.stop(); 
            return 4;
        }
    }
    #ifdef ENABLE_DEBUG
       SERIAL_PRINTLN("------- Reply from Gateway: ------");    
       print_response(wifi_client);
       //basic_print_response(wifi_client);    
    #endif

   SERIAL_PRINTLN("Ending HTTP connection..");
   SERIAL_PRINTLN("");  
   wifi_client.stop();   // close connection
   return 0;  // success
}

// after checkForCommand(), call getCommand() to retrieve the result 
int WifiPoster::checkForCommand() {
   
   strcpy(command_string, "");  // you must reset the command cache to  
                               // avoid repeated execution  *** 
   if (!W.isConnected()) {
        SERIAL_PRINTLN("WifiPoster : No wifi connection !");  
        W.reconnect();  // this is very much needed for WifiMulti!        
        return 1;   
    }
    SERIAL_PRINTLN("WifiPoster: Checking for commands from Gateway...");
    
    WiFiClient wifi_client; // Note: it is a local variable
    wifi_client.setTimeout (HTTP_TIMEOUT);    
    wifi_client.setNoDelay(true);  // disable Nagle algorithm
    bool connected = wifi_client.connect(pC->cmd_host, pC->cmd_port);
    if (!connected) {
        wifi_client.stop();
        delay(1000);  // https://github.com/esp8266/Arduino/issues/722
        connected = wifi_client.connect(pC->cmd_host, pC->cmd_port);
    }
    if (!connected) {
        SERIAL_PRINTLN ("WifiPoster: Could not connect to HTTP server.");
        wifi_client.stop();
        return 2;
    }
        
    wifi_client.print(cmd_header);  // send it off
    
    int timeout = DIY_TIMEOUT;  // Eg: 50*100 = 5 sec
    while (wifi_client.connected() && !wifi_client.available()) {
        delay(100);  // yield time to WiFi to do its job
        if (--timeout <= 0) {
            SERIAL_PRINTLN("WifiPoster: HTTP Server timed out !");
            wifi_client.stop(); 
            return 3;
        }
    }
    advanced_print_response(wifi_client); // this stores it in command_string
    //basic_print_response(wifi_client);

    // if there are no commands in the queue, the gateway will send an empty string
    if (strlen(command_string)==0) 
      SERIAL_PRINTLN ("WifiPoster: No command was received from gateway.");  
    else {
        SERIAL_PRINTLN ("WifiPoster: Received a command from gateway: ");
        SERIAL_PRINTLN(command_string);
    }
    SERIAL_PRINTLN("Ending HTTP connection..");
    SERIAL_PRINTLN("");
    wifi_client.stop();   // close connectionconnection
    return 0;
}

// dump headers and payload without parsing
void WifiPoster::basic_print_response (WiFiClient wifi_client) {
    // TODO: time out - in case the server is holding the connection without talking
    char c;
    while (wifi_client.connected()) {
        if (wifi_client.available()) {
            c = wifi_client.read(); 
            SERIAL_PRINT(c);
        }
    }
    SERIAL_PRINTLN();
}

/***  
// Aliter:
// dump headers and payload without parsing
void WifiPoster::basic_print_response (WiFiClient wifi_client) {
    // TODO: time out - in case the server is holding the connection without talking
    while (wifi_client.connected()) {
        if (wifi_client.available()) {
            String line = wifi_client.readStringUntil('\n');
            SERIAL_PRINTLN (line);
        }
    }
}
***/

// TODO: rewrite this as a state machine
// skip the headers and print only the content
void WifiPoster::print_response(WiFiClient wifi_client) {
    bool eoh_found = false;
    bool response_code_found = false;
    String response_code;
    // TODO: time out - in case the server is holding the connection without talking
    // TODO: state machine; eliminate String class  
    while (wifi_client.connected()) {
        if (wifi_client.available()) {
            String line = wifi_client.readStringUntil('\n');
            if (!response_code_found) {
                response_code_found =true;
                response_code = line;
                SERIAL_PRINT ("HTTP Response code: ");
                SERIAL_PRINTLN (response_code);  
                //SERIAL_PRINTLN();        // if the newline is missing        
            } 
            else
            if (!eoh_found && line.length() < 2)  // skip the headers
                eoh_found = true;  
            if (eoh_found)
                SERIAL_PRINTLN (line);
        }
    }
}

// skip the headers and process payload; store the payload in the
// command cache object command_string
void WifiPoster::advanced_print_response (WiFiClient wifi_client) {
    // TODO: time out, state machine, eliminate String class and use char arrays
    bool eoh_found = false;
    bool response_code_found = false;
    String response_code;
    String body_content = "";
    while (wifi_client.connected()) {
        if (wifi_client.available()) {
            String line = wifi_client.readStringUntil('\n');
            if (!response_code_found) {
                response_code_found =true;
                response_code = line;
                SERIAL_PRINT ("HTTP Response code: ");
                SERIAL_PRINTLN (response_code);
                if (PH) SERIAL_PRINTLN ("------- Headers: ------");
            }
            if (!eoh_found && line.length() < 2) { // first blank line = '\n'
                eoh_found = true;
                if (PH) SERIAL_PRINT("----- end of headers. ------ ");
                if (PH) SERIAL_PRINTLN (line.length());
            }
            if (eoh_found) {
                SERIAL_PRINTLN (line);
                body_content = body_content +line +"\n" ;  // * this could grow DANGEROUSLY ! *
                if (body_content.length() >= MAX_STRING_LENGTH)
                  break; 
            }
            else
                if (PH) SERIAL_PRINTLN (line);
        }
    }
    if (body_content.length() > 0) 
         safe_strncpy (command_string, (char *)body_content.c_str());  
}



   
 
