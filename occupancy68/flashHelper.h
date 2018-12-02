// flashHelper.h

#ifndef  FLASH_HELPER_H
#define  FLASH_HELPER_H

/*************************************************************************************** 
   Saves config data into Flash. To avoid overwriting the config,
   select Tools/Erase Flash/ Only Sketch option while uploading code from the IDE. 
****************************************************************************************/

#include "common.h"
#include "config.h"
#include <EEPROM.h>

// Three copies of the data structure DeviceIdentifier are stored
// Each data block is 100 bytes long: 
// A block contains 6 long int variables 
// First copy starts at address: 10 (decimal)
// The next 2 copies are at 110 and 210 (all decimal)
// TODO: replace magic number with the checksum of all other bytes

#define MAGIC_NUMBER        0x55aa

#define BLOCK_ADDRESS1      10
#define BLOCK_ADDRESS2      110
#define BLOCK_ADDRESS3      210
#define TOTAL_ALLOC_SIZE    310

// sizeof(long) = 4 bytes

struct StorageBlock {
  long  lparam1;
  long  lparam2;
  long  lparam3;          
  long  lparam4;
  long  lparam5;  
  long  magic_number;    // to verify data integrity  
};

class FlashHelper {
  public:
    void init (Config* configptr); // flash contents are saved in the Config object
    void begin();
    void commit();
    void end();
    bool testMemory ();
    bool readFlash (StorageBlock* pBlock);   // Returns true if flash data is OK, false if corrupted    
    bool writeFlash (StorageBlock& block);   // Returns true if the write is OK, false if it failed 
  private:
    Config *pC;
    bool readBlock (int block_addr, StorageBlock* pBlock);  // true= block is OK, false=corrupted 
};

#endif
