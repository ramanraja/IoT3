// myfi.cpp

#include "myfi.h"

    
MyFi::MyFi() {
}
 
bool MyFi::init(Config* configptr) {
    this->pC = configptr;
    
    WiFi.mode(WIFI_OFF);  // Prevents reconnection issue (eg: taking too long to connect) *
    delay(1000);
    // it is important to set STA mode: https://github.com/knolleary/pubsubclient/issues/138
    WiFi.mode(WIFI_STA); 
    //wifi_set_sleep_type(NONE_SLEEP_T);  // revisit & understand these
    //wifi_set_sleep_type(LIGHT_SLEEP_T);    
    wifi_set_sleep_type(MODEM_SLEEP_T);
    
    SERIAL_PRINTLN("Enlisting SSIDs:");  
    wifi_multi_client.addAP(pC->wifi_ssid1, pC->wifi_password1);
    SERIAL_PRINTLN(pC->wifi_ssid1);
    if (strlen(pC->wifi_ssid2) > 0) {
        wifi_multi_client.addAP(pC->wifi_ssid2, pC->wifi_password2); 
        SERIAL_PRINTLN(pC->wifi_ssid2);
    }
    if (strlen(pC->wifi_ssid3) > 0) {
        wifi_multi_client.addAP(pC->wifi_ssid3, pC->wifi_password3); 
        SERIAL_PRINTLN(pC->wifi_ssid3);
    }
            
    int MAX_ATTEMPTS = 60;  // 15 sec
    int attempts = 0;
    while(wifi_multi_client.run() != WL_CONNECTED) {   
        delay(250);
        SERIAL_PRINT ("."); 
        attempts++;
        if (attempts >= MAX_ATTEMPTS) {
            SERIAL_PRINTLN ("!\n- MyFi [1]: Could not connect to WiFi -"); 
            return false;
        }
    }
    SERIAL_PRINTLN();
    if (use_static_IP) 
        setStaticIp();
    SERIAL_PRINTLN ("!\nWi-Fi connected.");  
    dump ();
    return true; 
} 

void MyFi::setStaticIp() {
    SERIAL_PRINT("Subnet mask: ");
    SERIAL_PRINTLN(WiFi.subnetMask());
    SERIAL_PRINT("Gateway IP: ");
    SERIAL_PRINTLN(WiFi.gatewayIP());
    IPAddress udp_ip(192, 168, pC->ip_high, pC->ip_low);   // static IP of this device    
    WiFi.config(udp_ip, WiFi.gatewayIP(), WiFi.subnetMask());   
    SERIAL_PRINTLN("WiFi Configured.");
    
    // TODO TODO: restart UDP server also ***
}

bool MyFi::isConnected() {
    return (WiFi.status() == WL_CONNECTED); 
}

// Do NOT call this every 2 seconds or so ! reconnect() takes 10 sec to time out.
// this will delay your main program loop
// check status and reconnect automaticaly, if necessary
bool MyFi::checkWifi() {
     SERIAL_PRINTLN("MyFi: checking connection...");
     if(WiFi.status() == WL_CONNECTED) 
          return (true);
     SERIAL_PRINTLN("MyFi: No wifi connection !");
     return (reconnect());  // this is very much needed for WifiMulti!
}

// Do NOT call this in the main loop ! It takes 10 sec to time out
bool MyFi::reconnect () {
    SERIAL_PRINTLN("MyFi: Trying to reconnect to WifiMulti..");
    WiFi.mode(WIFI_STA);
    int MAX_ATTEMPTS = 40;  // 10 sec
    int attempts = 0;
    bool connected = true;
    while(wifi_multi_client.run() != WL_CONNECTED) {   
        delay(250);
        SERIAL_PRINT ("."); 
        attempts++;
        if (attempts >= MAX_ATTEMPTS) {
            SERIAL_PRINTLN ("!\n- MyFi [2]: Could not reconnect to WiFi -"); 
            connected = false;
            break;
        }
    }
    SERIAL_PRINT("\nMyFi: Wifi connected? : "); 
    SERIAL_PRINTLN(connected);    
    if (connected) {
        if (use_static_IP) 
            setStaticIp();
        dump();  
    }
    return(connected);
}

void MyFi::disable() {
    SERIAL_PRINTLN ("\nSwitching off wifi..");
    WiFi.disconnect(); 
    WiFi.mode(WIFI_OFF);
    WiFi.forceSleepBegin();
    delay(10); 
}

void MyFi::dump() {
  SERIAL_PRINT("Connected to WiFi SSID: ");
  SERIAL_PRINTLN(WiFi.SSID());

  // print the WiFi shield's IP address
  IPAddress ip = WiFi.localIP();
  SERIAL_PRINT("IP Address: ");
  SERIAL_PRINTLN(ip);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  SERIAL_PRINT("Signal strength (RSSI):");
  SERIAL_PRINT(rssi);
  SERIAL_PRINTLN(" dBm");
  SERIAL_PRINTLN();
}
