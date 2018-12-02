// sends PIR and Radar status (0 - 3) over UDP every 2 seconds
//use the test tool: https://packetsender.com/documentation
/*
    TODO: 
    put the correct FW server URL in config.h
    replace flashhelper to just return 5 longs, and config to consume it
    config.repair_flash()
    revice commandHandler; implement on the air update of IP address, device id and group id.
    temperature and humidity are often 0. investigate.
    surround all constant strings with F()
    The JsonParser is very heavy. Implement your own.
    Bring all the HTTP work in OTA helper into HttpPoster class
    Develop the utility class with other utility functions
    Disable serial port and test it.
*/
#include "occupancy.h"

//#define  SUMULATION_MODE
//#define  FLASH_REPAIR_MODE

Config C;
Timer T;
MyFi W;
MyUdp U;
MyJson  J;
Hardware H; 
OtaHelper O;
HttpPoster POSTER; 
////WifiPoster POSTER; 
////CommandHandler CMD;

SensorData  tx_data;
////CommandData rx_data;
char *request_str; // memory for this string is allocated in myUdp

char *tmp_ptr;
char tmp_msg[128];

// shared globals
int data_timer_id = 0;
boolean  occupied = true;   // start in occupied state
unsigned int pir_tick_counter = 0;
unsigned int radar_tick_counter = 0;

void setup() {
    randomSeed(micros());
    init_serial();
    C.init();
    C.targetTestUrl(); // TODO: revisit this !
    C.dump();   
    H.init(&C);
    H.switchLightsOn(); // light should be ON at startup    
    H.beep(T);
    #ifdef FLASH_REPAIR_MODE
        display_flash_msg();
    #else
        W.init(&C);  
        U.init(&C);
        POSTER.init(&C, &W);
        O.init(&C);
        ////CMD.init(&C);
        yield();  
        H.blinker();     // includes a random startup delay    
        read_sensors();  // priming read
        T.after (10000L, send_restart_message);
        T.every (C.tick_interval, ticker);    
        T.every (C.sensing_interval, read_sensors);
        T.every (C.data_interval,  send_status);        
        T.every (C.sensor_staus_interval, send_udp_status);  
    #endif  
    
    SERIAL_PRINTLN("Setup complete.");
}

#ifdef FLASH_REPAIR_MODE
void display_flash_msg() {
      SERIAL_PRINTLN("\n--------------------------------------------------");
      SERIAL_PRINTLN("        ENTERING FLASH MEMORY REPAIR MODE           ");
      SERIAL_PRINTLN("Input 5 numbers on the serial console in the format:");
      SERIAL_PRINTLN("9 9 9 9 9");
      SERIAL_PRINTLN("[device_id, group_id, relay_buzzer_enabled, high_ip, low_ip]"); 
      SERIAL_PRINTLN("Enable: 0=None; 1=Relay; 2=Buzzer; 3=Both.");
      SERIAL_PRINTLN("--------------------------------------------------\n");
}
#endif

void loop() {
 #ifdef FLASH_REPAIR_MODE
     repair_flash();
 #else
     T.update();
      // TODO: use double buffers
      request_str = (char *)U.get_request();  // storage allocated in MyUdp
      if (strlen(request_str) > 0)
          process_command (request_str);     
 #endif    
}  

#ifdef FLASH_REPAIR_MODE
void repair_flash() {
    if (!Serial.available())
        return;
    String config_str = Serial.readStringUntil('\n');
    
    bool result = C.repairFlash(config_str.c_str());   
    if (result) {
      SERIAL_PRINTLN("\nFlash repair completed.");
      SERIAL_PRINTLN("Next steps:"); 
      SERIAL_PRINTLN("  1. Comment out 'FLASH_REPAIR_MODE' in code and compile.");
      SERIAL_PRINTLN("  2. Upload the program again.");
    }
    else 
      SERIAL_PRINTLN("\nFlash repair failed.");
    // junk the remaining chars, if any
    while(Serial.available()) 
        Serial.read();     
}
#endif

void ticker () {
    H.readPM();  // first, read PIR and Radar
    // if there is any movement, keep resetting the count
    if (H.pir_status) 
        pir_tick_counter = 0;
    if(H.radar_status)
        radar_tick_counter = 0;
    update_status();
}

// 0=both off,  1=radar on, 2=PIR on, 3=both on  
void send_udp_status() {
    byte pr_status = 0;
    if (H.pir_status) 
        pr_status = 2;
    if(H.radar_status)   
        pr_status += 1;       
    U.send_sensor_status('0' + pr_status);
}

// Uses shared variables; Call readPM before calling this !
void update_status() {    
    pir_tick_counter++;      
    radar_tick_counter++;    
    if (!occupied) {
        if (H.pir_status && H.radar_status)    // both the sensors fired, so occupy the room
            occupy_room();
    }
    if (radar_tick_counter == C.buzzer_ticks1) {
        if (occupied)
            warn(STATUS_WARNED1);  // warn about the imminent release
    }
    if (pir_tick_counter == C.buzzer_ticks2) {
        if (occupied)
            warn(STATUS_WARNED2);  // warn about the imminent release
    }
    if (radar_tick_counter >= C.release_ticks1){  // Even the radar was silent for 2 minutes.
         radar_tick_counter = 0; // to avoid counter overflow during nights/holidays
         if (occupied)
            release_room();  
    }
    if (pir_tick_counter >= C.release_ticks2){  // PIR was silent for 8 minutes.
         pir_tick_counter = 0; // to avoid counter overflow during nights/holidays
         if (occupied)
            release_room();              
    }  
}

// Read temperature, humidity and light
void read_sensors () {
    H.readTHL();  // temp, humidity, light are stored inside H
}

void occupy_room() {
    occupied = true; 
    H.switchLightsOn(); 
    SERIAL_PRINTLN ("Room is now ccupied.");    
    send_event(STATUS_OCCUPIED); 
}

// pre-release warning flashes
void warn(byte warning_type) { 
    SERIAL_PRINTLN ("About to release the room.."); 
    H.warn(T);
    send_event(warning_type); 
}

void release_room() {
    occupied = false; 
    H.switchLightsOff();      
    SERIAL_PRINTLN ("Room is now released.");
    send_event(STATUS_FREE); 
}
//------------------------------------------------------------------------------
void send_restart_message() {
    SERIAL_PRINTLN("\n-------------------->>> sending restart msg...\n");  
    tx_data.device_id = C.device_id;
    tx_data.group_id = C.group_id;
    tx_data.node_status = STATUS_RESTARTED;                   
    tx_data.status_param = C.current_firmware_version; // it is not CMDX_REGULAR_DATA *  
    const char* msg = J.serialize (tx_data);
    int result = POSTER.sendStatus(msg);
    SERIAL_PRINT ("Http post [test] result: ");
    SERIAL_PRINTLN(result);
    result = POSTER.sendStatus(msg, DATA_PROD_URL);
    SERIAL_PRINT ("Http post [prod] result: ");
    SERIAL_PRINTLN(result);    
}

int packet_count = 0;  
#ifdef SUMULATION_MODE
// periodic staus updates   
void  send_status() {
    SERIAL_PRINT("------>>> sending simulated data. Pkt# =");
    SERIAL_PRINTLN(packet_count);
    packet_count = (packet_count+1)%5;
    tx_data.device_id = C.device_id;
    tx_data.group_id = C.group_id;
    tx_data.node_status = random(1, 4);
    tx_data.status_param = CMDX_REGULAR_DATA;   
    tx_data.temperature = random(19, 36);
    tx_data.humidity = random (10, 90);
    tx_data.light = random (10, 90);   
    tx_data.heat_index = random(19, 40);
    const char* msg = J.serialize (tx_data);
    SERIAL_PRINTLN(msg);
//    SERIAL_PRINT("Json payload size: ");
//    SERIAL_PRINTLN(strlen(msg));
    int result = POSTER.sendStatus(msg);
    SERIAL_PRINT ("Http post [test] result: ");
    SERIAL_PRINTLN(result);  // TODO: send to AWS also?
//    if (packet_count==0) {
//        result = POSTER.sendStatus(msg, DATA_PROD_URL);   
//        SERIAL_PRINT ("Http post [prod] result: ");
//        SERIAL_PRINTLN(result);        
//    }         
}   
#else
void  send_status() {
    SERIAL_PRINT("------>>> sending data. Pkt# =");
    SERIAL_PRINTLN(packet_count);
    packet_count = (packet_count+1)%5;
    tx_data.device_id = C.device_id;
    tx_data.group_id = C.group_id;
    tx_data.node_status = (occupied ? STATUS_OCCUPIED : STATUS_FREE);
    tx_data.status_param = CMDX_REGULAR_DATA;   
    tx_data.temperature = H.temperature;
    tx_data.humidity = H.humidity;
    tx_data.light = H.light;   
    tx_data.heat_index = H.heat_index;
    const char* msg = J.serialize (tx_data);
    SERIAL_PRINTLN(msg);
//    SERIAL_PRINT("Json payload size: ");
//    SERIAL_PRINTLN(strlen(msg));
    int result = POSTER.sendStatus(msg);  
    SERIAL_PRINT ("Http post [test] result: ");
    SERIAL_PRINTLN(result);
    if (packet_count==0) {
        result = POSTER.sendStatus(msg, DATA_PROD_URL);   
        SERIAL_PRINT ("Http post [prod] result: ");
        SERIAL_PRINTLN(result);        
    }     
} 
#endif

// when an event happens, notify immediately
void send_event (int status_code) {
    tx_data.device_id = C.device_id;
    tx_data.group_id = C.group_id;
    tx_data.node_status = status_code;
    tx_data.status_param = CMDX_EVENT_DATA;
    const char* msg = J.serialize (tx_data);
    SERIAL_PRINTLN("Main: sending event data..");    
    SERIAL_PRINT("Json payload size: ");
    SERIAL_PRINTLN(strlen(msg));
    int result = POSTER.sendStatus(msg);
    SERIAL_PRINT ("Http post [test] result: ");
    SERIAL_PRINTLN(result);
    result = POSTER.sendStatus(msg, DATA_PROD_URL);   
    SERIAL_PRINT ("Http post [prod] result: ");
    SERIAL_PRINTLN(result); 
}
 
// TODO: move this to command handler class
void process_command (char* request_str) {
    //SERIAL_PRINTLN(request_str);
    char cmd = request_str[0];   // the command is of the form  "X 999 999"  
    switch (cmd) {
        case 'H':   
            U.send_handshake();
            break;      
        case 'R':
            U.send_message ("# Rebooting ESP!..");        
            H.restartEsp();
            break;     
        case 'C':
            set_config(request_str);
            break;                
        case 'F':
            update_firmware();
            break;        
        case '@':  // hope this never happens
            U.send_message ("# --->>> UDP PROTOCOL FAILURE ! <<<----");
            break;         
    }
}

void set_config(const char* request_str) {  
      tmp_ptr = (char *)request_str+1;  // go past the cmd char; any spaces will be ignored
      bool result = C.repairFlash (tmp_ptr);
      if (!result) {
          U.send_message("# ERROR: could not save configuration.");
          return;
      }
      U.send_message ("# Configuration saved successfully.");
      U.send_message ("# If you have changed the IP address, reconnect to it now.");
      /*-----------------------------------------
      //delay(1000);  // let the messge go out
      // somehow, the following is not taking effect:
      //W.init(&C);   // set the static IP
      //U.init(&C);   // listen at the new address
      // for now, just reboot. TODO: debug the above 2 lines
      -------------------------------------------*/
      U.send_message ("# Restarting ESP...");
      delay(1000);   // let the messge go out
      H.restartEsp();
}

// this is done only on-demand through a targetted UDP command 
void update_firmware() {
    U.send_message ("# Checking for firmware updates..");
    int result = O.check_and_update();  // if there was an update, this will restart 8266
    SERIAL_PRINT ("OtaHelper: response code: ");  
    SERIAL_PRINTLN (result);
    U.send_message ("# No firmware updates.");
}

void init_serial () {
    #ifdef ENABLE_DEBUG
        //Serial.begin(C.baud_rate);  // there is no C !
        Serial.begin(115200); 
        #ifdef VERBOSE_MODE
           Serial.setDebugOutput(true);
        #endif
        Serial.setTimeout(250);
    #endif    
    SERIAL_PRINTLN("\n\n********************* Vz IoT starting... ********************\n"); 
}
