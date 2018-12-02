
// commandHandler.cpp

#include "commandHandler.h"
//#include "externDeclare.h"

extern void send_response(long command, int response);
extern void safe_strncpy (char *dest, char *src, int length = MAX_STRING_LENGTH);

CommandHandler::CommandHandler() {
}
 
void CommandHandler::init (Config* configptr) {
    this->pC = configptr;
} 

// this internal version is named sendResponse().
// compare it with the external function name send_response()  
void CommandHandler::sendResponse(int response) {
    send_response (command_cache.command, response);
}

void CommandHandler::handleCommand (CommandData& cmd) {
    copyCommand (cmd);
    printCommand();
    switch(command_cache.command) { 
        case CMD_HELLO: 
            SERIAL_PRINTLN("Ping hello received.");
            break;
        case CMD_TARGET_PROD_URL:
            targetProductionUrl();
            break;              
        case CMD_TARGET_TEST_URL:
            targetTestUrl();
            break;   
        case CMD_SET_FW_SERVER_URL:
            setFirmwareServerUrl();
            break;       
        case CMD_SET_DEVICE_ID:  
            setDeviceID ();
            break;            
        case CMD_SET_GROUP_ID:  
            setGroupID ();
        case CMD_SET_EXTRAS:
            setEnableRelay();
            break;
        case CMD_SET_INTERVAL:
            SERIAL_PRINTLN("CMD_SET_INTERVAL not implemented.");
            break;  
        case CMD_WRITE_FLASH_CONFIG: // to save it permanently, you must call this 
            writeFlashConfig();
            break;                     
        case CMD_UPDATE_FIRMWARE:
            updateFirmware();
            break;      
        case CMD_RESTART:
            restart();
            break;              
        default:
            sendParsingError();
            break;
    }
}

//  Makes a local copy of the command object before another command arrives
void CommandHandler::copyCommand (CommandData& cmd) {
    command_cache.command = cmd.command;
    command_cache.long_param = cmd.long_param; 
    if (strlen(cmd.string_param) > 0)      // if it is not garbage
        safe_strncpy (command_cache.string_param, cmd.string_param);
}

void CommandHandler::sendParsingError() {
    SERIAL_PRINT ("* ERROR: Command "); 
    SERIAL_PRINT(command_cache.command);     
    SERIAL_PRINTLN(" is not found *");
    sendResponse (CMDX_PARSING_ERROR); 
}            

// temporarily set device_id for the rest of the session
void CommandHandler::setDeviceID()
{
    pC->device_id = command_cache.long_param;
    SERIAL_PRINT("Till this device is restarted, temporary device id is: ");
    SERIAL_PRINTLN(pC->device_id);
    sendResponse (STATUS_CMD_SUCCESS); // with new id 
}

// temporarily set group_id for the rest of the session
// Tip: First set the device_id to the new value and then change the group_id.
void CommandHandler::setGroupID()
{
    pC->group_id = command_cache.long_param;
    SERIAL_PRINT("Till this device is restarted, temporary group id is: ");
    SERIAL_PRINTLN(pC->group_id);
    sendResponse (STATUS_CMD_SUCCESS); // with new id 
}

// temporarily set extra param for the rest of the session
// used for enabling/disabling the relay; the new value is in long_param.
void CommandHandler::setEnableRelay(){
    pC->relay_enabled = (bool)command_cache.long_param;
    SERIAL_PRINT("Relay temporarily ");
    SERIAL_PRINTLN(pC->relay_enabled ? "enabled." : "disabled.");
    sendResponse (STATUS_CMD_SUCCESS);  
}

// Permanently save the device config to flash.
// Before calling this, you must set deviceID, groupID, and optionally, 
// the threshold into the config object pC.
void CommandHandler::writeFlashConfig() {
    FlashHelper F;
    F.init(pC);
    F.begin();
    bool result = F.writeFlash();
    F.end();
    if (result) {
        SERIAL_PRINTLN ("Device configuration stored in Flash.");
        sendResponse (STATUS_CMD_SUCCESS); 
    } else {
        SERIAL_PRINTLN ("*** Failed to write device configuration to Flash ***");
        sendResponse (STATUS_CMD_FAILED); 
    }
}
   
bool CommandHandler::verifyURL (const char *new_url) {
    if (strlen(new_url) >= MAX_STRING_LENGTH) {
      SERIAL_PRINTLN ("* ERROR: URL string is too long *");
      return (false);
    }
    if (!(new_url[0]=='h' && new_url[1]=='t' && new_url[2]=='t' && new_url[3]=='p' && new_url[4]==':') ) {
      SERIAL_PRINTLN ("* ERROR: malformed URL *");
      return (false);     
    }
    return (true);  // TODO: check with a list of preconfigured urls, domain check etc 
}

// temporarily set device config for the current session
// The new firmware URL is in command_cache.string_param  
void CommandHandler::setFirmwareServerUrl () {
    if (!verifyURL(command_cache.string_param)) {
        sendResponse (STATUS_CMD_FAILED);   
        return;    
    }
    safe_strncpy (pC->firmware_server_url, command_cache.string_param);
    SERIAL_PRINT("Firmware server URL temporarily set to: ");
    SERIAL_PRINTLN(pC->firmware_server_url);    
    sendResponse (STATUS_CMD_SUCCESS);    
}

void CommandHandler::updateFirmware() {  
    sendResponse (STATUS_CMD_ACK);
    OtaHelper O;
    O.init(pC);
    O.check_and_update(); // this will restart the device
}
  
void CommandHandler::restart () {
    sendResponse (STATUS_CMD_ACK);
    SERIAL_PRINTLN ("\n\n*** Occupancy sensor is about to restart! ***\n");
    delay(1000);
    ESP.restart();
}
  
void CommandHandler::targetProductionUrl() {
    pC->targetProdUrl();
    sendResponse (STATUS_CMD_SUCCESS);      
}

void CommandHandler::targetTestUrl(){
    pC->targetTestUrl();
    sendResponse (STATUS_CMD_SUCCESS);      
}
  
void CommandHandler::printCommand() {   // print from cache
    SERIAL_PRINT("command: ");
    SERIAL_PRINT(command_cache.command);
    SERIAL_PRINT("long_param: ");
    SERIAL_PRINTLN(command_cache.long_param);   
    SERIAL_PRINT("string_param: ");
    SERIAL_PRINTLN(command_cache.string_param);
    SERIAL_PRINTLN("-------------------------");   
}


