// Split large packets int 512 byte chunks and send them one by one
// in response to prompts of 'N'. Aborts  if the prompt is 'A'.
//use the test tool: https://packetsender.com/documentation
// Or the python program udpReceiver.py
// Done:
// Receives commands over UDP and execute them;
// Uses serial/UDP to configure the local IP address of self.
// Saves IP address in Flash; updates over OTA upon receiving a command.

/***************************************************************************
  [TODO: Read pixel data from MLX90640 32x24IR camera and] transmit to PC over UDP 
  as a string of floats delimited by [ and ] with CR-LF in multiple chunks.
  If the sensor reading is NAN or is out of bounds, resets the sensor through a 2N2222.
  Test it with  Arduino serial monitor by sending any random characters as prompt. 
  Use it with Python visualizer ThermalCam32.py
  No need to install the library in the Arduino path; just add the cpp and h files in your
  project folder.  
  Connect I2C Data (SDA = D2 = GPIO 4) and Clock (SCK = D1 = GPIO 5) Pins of sensor 
  with 4.7 K Pullup reistors. Connect GND and VDD with a 100nF ceramic Capacitor.     
  Sometimes the sensor freezes, so you may need  a transistor to reset it.]   
****************************************************************************/


#include "thermalCam.h"
#include "myUdp.h"     // for NUM_CHUNKS

// uncomment this once per every new device, to set IP address
//#define  FLASH_REPAIR_MODE

Config C;
MyFi W;
MyUdp U;
MyOta O;
Hardware H;

int next_chunk = 0;
int error_count = 0;        // number of ABORT commands received
int ERROR_THREHSOLD1 = 10;  // reset sensor after 10 consecutive aborts
int ERROR_THREHSOLD2 = 30;  // reboot 8266 after 30 consecutive aborts

bool sensor_read_OK = false;
char *request_str; // memory for this string is allocated in myUdp
char *tmp_ptr;
char tmp_msg[128];
 
void setup()
{
    init_serial();
    C.init();
    C.dump();
    H.init(&C);  
    #ifdef FLASH_REPAIR_MODE
        display_flash_msg();
    #else
        W.init(&C);    
        O.init(&C);   
        U.init(&C);
        init_thermal_sensor();
        SERIAL_PRINTLN ("# Setup complete.");    
    #endif    
}

void loop()
{
  #ifdef FLASH_REPAIR_MODE
      repair_flash();
  #else      
      if (!W.isConnected()) 
          W.reconnect();
      sensor_read_OK = H.read_sensor();  // the data is stored within H
      // TODO: use double buffers
      request_str = (char *)U.get_request();  // storage allocated in MyUdp
      if (strlen(request_str) > 0)
          process_command (request_str);
  #endif
}

#ifdef FLASH_REPAIR_MODE
void display_flash_msg() {
    SERIAL_PRINTLN("\n--------------------------------------------------");
    SERIAL_PRINTLN("        ENTERING FLASH MEMORY REPAIR MODE           ");
    SERIAL_PRINTLN("Input 2 numbers on the serial console in the format:");
    SERIAL_PRINTLN("I 999 999");
    SERIAL_PRINTLN("[high_octet, low_octet] of the static IP address.");
    SERIAL_PRINTLN("Then recompile the code without FLASH_REPAIR_MODE defined");
    SERIAL_PRINTLN("--------------------------------------------------\n");
}
#endif

void init_thermal_sensor() {
    if (H.begin_sensor()) {
        SERIAL_PRINTLN("# Thermal sensor MLX90640 initialized [1].");
        return;
    }
    SERIAL_PRINTLN("# Sensor initialization error. trying again..");
    H.reset_sensor();
    if (H.begin_sensor()) {
        SERIAL_PRINTLN("# Thermal sensor MLX90640 initialized [2].");
        return;
    }
    SERIAL_PRINTLN("# Sensor MLX90640 not found");
}

#ifdef FLASH_REPAIR_MODE
void repair_flash() {
    if (!Serial.available())
        return;
    char c = Serial.read();      // get the command character
    if (c == 'r' || c == 'R') 
        H.restart_ESP();         // to quickly check the new values? 
    if (c == 'i' || c == 'I') {  
        // enter the last two octets separated by a space. Eg:  "I 3 107" for 192.168.3.107 
        // the first command char is already consumed
        String config_str = Serial.readStringUntil('\n');
        
        C.repairFlash(config_str.c_str());
        
        SERIAL_PRINTLN("\nFlash repair completed.");
        SERIAL_PRINTLN("Next steps:"); 
        SERIAL_PRINTLN("  1. Comment out 'FLASH_REPAIR_MODE' in code and compile.");
        SERIAL_PRINTLN("  2. Restart ESP.");
    }    
    else
        SERIAL_PRINTLN ("- Invalid command -");
    // junk the rest of it
    while(Serial.available()) 
        Serial.read();    
}
#endif

void process_command (char* request_str) {
    //SERIAL_PRINTLN(request_str);
    char cmd = request_str[0];   // the command is of the form  "X 999 999"  
    switch (cmd) {
        case 'N':
            error_count = 0;  // any successful transmission resets error count
            if (sensor_read_OK) {
                if (next_chunk==0)
                    H.refresh_cached_frame();
                U.send_data (H.cached_frame, next_chunk);
                next_chunk = (next_chunk+1) % NUM_CHUNKS;                
            }
            else
                U.send_message( "# --- sensor error ---");
            break;     
        case 'A':   // abort the remaining chunks
            SERIAL_PRINTLN("--- Aborting chunks ---");
            next_chunk = 0;
            error_count++;
            if (error_count >= ERROR_THREHSOLD1) 
                H.reset_sensor();
            else 
            if (error_count >= ERROR_THREHSOLD2) 
                H.restart_ESP();
            break;                  
        case 'H':   // hand shake
            sprintf (tmp_msg, "# Hello, this is IP 192.168.%d.%d", C.ip_high, C.ip_low);
            U.send_message (tmp_msg);
            break;      
        case 'R':
            U.send_message ("# Rebooting ESP!..");        
            H.restart_ESP();
            break;
        case 'S':
            U.send_message ("# Resetting sensor..");
            H.reset_sensor();
            break;
        case 'I':
            set_IP(request_str);
            break;        
        case 'F':
            update_firmware();
            break;        
        case '@':  // hope this never happens
            U.send_message ("# --->>> UDP PROTOCOL FAILURE ! <<<----");
            break;         
    }
}

// set the local IP temporarily
void set_IP (const char* request_str) {
      tmp_ptr = (char *)request_str+1;  // go past the cmd char; spaces will be ignored
      bool result = C.repairFlash (tmp_ptr);
      if (!result) {
          U.send_message("# ERROR: could not set new IP");
          return;
      }
      // now C has the new IP
      sprintf (tmp_msg, "# My new IP will be: 192.168.%d.%d", C.ip_high, C.ip_low); 
      U.send_message (tmp_msg);
      U.send_message ("# See you at the new IP. Bye !");
      //delay(1000);  // let the messge go out
      // somehow, the following is not taking effect:
//      W.init(&C);   // set the static IP
//      U.init(&C);   // listen at the new address
      // for now, just reboot. TODO: debug the above 2 lines
      U.send_message ("# Restarting ESP...");
      delay(1000);   // let the messge go out
      H.restart_ESP();
}
 
// this is done only on-demand through a UDP command 
void update_firmware() {
    U.send_message ("# Checking for firmware updates..");
    int result = O.check_and_update();  // if there was an update, this will restart 8266
    SERIAL_PRINT ("OtaHelper: response code: ");  
    SERIAL_PRINTLN (result);
    U.send_message ("# No firmware updates.");    
}
 
void init_serial() {
    Serial.begin(BAUD_RATE);
    SERIAL_PRINTLN();  // go over 8266 garbage
    SERIAL_PRINTLN("\n# **************** UDP thermal camera MLX90640 starting... *******************\n");
}

 
