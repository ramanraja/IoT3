// commandHandler.h

#ifndef COMMAND_HANDLER_H
#define COMMAND_HANDLER_H

#include "common.h"
#include "config.h"
#include "jayson.h"
#include "otaHelper.h"
#include "flashHelper.h"

class CommandHandler {
public:
    CommandHandler();
    void init (Config* configptr);
    void handleCommand (CommandData& cmd);   
private:
    Config *pC;
    CommandData command_cache;  // local copy of the incoming command  
    void copyCommand (CommandData& cmd) ;   
    void sendResponse(int response);
    void targetProductionUrl();
    void targetTestUrl();
    void setDeviceID();
    void setGroupID();
    void setEnableRelay();    
    void setFirmwareServerUrl(); 
    void updateFirmware(); 
    void writeFlashConfig();
    void restart ();
    void sendParsingError (); 
    void printCommand();
    bool verifyURL(const char* new_url);
};

#endif 
