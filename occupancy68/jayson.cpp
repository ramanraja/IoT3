// jayson.cpp

#include "jayson.h"
//#include "externDeclare.h"
extern void safe_strncpy (char *dest, char *src, int length = MAX_STRING_LENGTH);
 
// static variables 
DynamicJsonBuffer  jbufferSensors (JSON_BUFFER_SIZE);   // outgoing - serialize
DynamicJsonBuffer  jbufferCommand (JSON_BUFFER_SIZE);   // incoming - deserialize  

/*
  Memory allocation for tx_str_buffer object is done in this class.
  So do not call serialize() more than once, before consuming the string !
 */
const char* MyJson::serialize (const SensorData& tx_payload) {
    jbufferSensors.clear(); // to reuse 
    JsonObject& root = jbufferSensors.createObject();    
    root["ID"] = tx_payload.device_id;     
    root["G"] = tx_payload.group_id; 
    root["S"] = tx_payload.node_status;  
    root["P"] = tx_payload.status_param;     
    root["T"] = tx_payload.temperature;
    root["H"] = tx_payload.humidity;
    root["I"] = tx_payload.heat_index;          
    root["L"] = tx_payload.light;         
    root.printTo(tx_str_buffer, JSTRING_BUFFER_SIZE);
    return ((const char*)tx_str_buffer);
}

/*
  Memory allocation for CommandData object is done in this class.
  So do not call deserialize() more than once, before consuming the command_data_cache object !
 */
const CommandData& MyJson::deserialize (const char* rx_payload) {
    jbufferCommand.clear(); // to reuse 
    JsonObject& root = jbufferCommand.parseObject (rx_payload);
    if (!root.success()) {
        SERIAL_PRINTLN("Command: Json parsing failed !");
        return (cmd_parsing_error);
    }    
    // if any numeric key is absent, it is filled with zero
    ////command_data_cache.device_id = root["ID"];
    ///command_data_cache.group_id = root["G"];    
    command_data_cache.command = root["C"]; 
    command_data_cache.long_param = root["L"];   
    // if the string key is garbage, 8266 crashes !
    if (root.containsKey("S")) 
        safe_strncpy (command_data_cache.string_param, (char *)((const char *)root["S"])); // some quirk!
    else
        strcpy (command_data_cache.string_param, "");
    return(command_data_cache); 
}



