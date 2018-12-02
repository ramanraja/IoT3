// flashHelper.cpp

#include "flashHelper.h" 

void FlashHelper::init(Config* configptr) {   
    this->pC = configptr;
}

void FlashHelper::begin() {
    SERIAL_PRINT("Beginning EEPROM with ");
    SERIAL_PRINT(TOTAL_ALLOC_SIZE);
    SERIAL_PRINTLN(" bytes of memory..");    
    EEPROM.begin(TOTAL_ALLOC_SIZE);  // request memory allocation for all 3 copies
}

void FlashHelper::commit() {
    EEPROM.commit(); 
    SERIAL_PRINTLN ("EEPROM writes committed.");
}

void FlashHelper::end() {
    EEPROM.end(); 
    SERIAL_PRINTLN ("EEPROM released.");
}

// Take the data from Config object pC and store in flash:
// Write 3 copies of the data structure for safety.
 
bool FlashHelper::writeFlash (StorageBlock& block) {    
    block.magic_number = MAGIC_NUMBER;  // it is our duty to put in the finger print  
    SERIAL_PRINTLN("Writing device id...");
    begin();  
    EEPROM.put (BLOCK_ADDRESS1, block);
    EEPROM.put (BLOCK_ADDRESS2, block);
    EEPROM.put (BLOCK_ADDRESS3, block);
    commit();  // do not end(), as you will need to read it back later  
    delay(250);
    SERIAL_PRINTLN ("\nFinished writing memory block.");
    return true;
}  

/* 
   Read all 3 blocks from EEPROM and compare them. 
   Returns : true if data is OK, false if corrupted
*/

bool FlashHelper::readFlash (StorageBlock* pBlock) {
    StorageBlock block1, block2, block3;  
    if (!readBlock(BLOCK_ADDRESS1, &block1)) {  // true if the block is OK
        SERIAL_PRINTLN ("Block 1 is corrupted");      
        return false;
    }
    if (!readBlock(BLOCK_ADDRESS2, &block2)) {  
        SERIAL_PRINTLN ("Block 2 is corrupted");      
        return false;  
    }
    if (!readBlock(BLOCK_ADDRESS3, &block3)) {  
        SERIAL_PRINTLN ("Block 3 is corrupted");
        return false;
    }
    bool mismatch = false;
    if ((block1.lparam1 != block2.lparam1) || (block1.lparam1 != block3.lparam1))
        mismatch = true;
    if ((block1.lparam2 != block2.lparam2) || (block1.lparam2 != block3.lparam2))
        mismatch = true;
    if ((block1.lparam3 != block2.lparam3) || (block1.lparam3 != block3.lparam3))
        mismatch = true;
    if ((block1.lparam4 != block2.lparam4) || (block1.lparam4 != block3.lparam4))
        mismatch = true;
    if ((block1.lparam5 != block2.lparam5) || (block1.lparam5 != block3.lparam5))
        mismatch = true;                            
    if (mismatch) {
        SERIAL_PRINTLN ("Data in 3 copies do not match !");
        return false;
    }
    pBlock->lparam1 = block1.lparam1;
    pBlock->lparam2 = block1.lparam2;
    pBlock->lparam3 = block1.lparam3;
    pBlock->lparam4 = block1.lparam4;
    pBlock->lparam5 = block1.lparam5;                
    
    return true;
}

// Returns true if the block is OK, and false if it is corrupted 
bool FlashHelper::readBlock (int block_addr, StorageBlock* pBlock) {
    StorageBlock block;  
    EEPROM.get (block_addr, block);
    #ifdef VERBOSE_MODE    
        SERIAL_PRINT("memory address :");
        SERIAL_PRINTLN(block_addr);    
        SERIAL_PRINT("magic_number: ");
        SERIAL_PRINTLNF(block.magic_number, HEX);   
        SERIAL_PRINTLN("LPRAM values:");
        SERIAL_PRINTLN(block.lparam1);   
        SERIAL_PRINTLN(block.lparam2);   
        SERIAL_PRINTLN(block.lparam3);   
        SERIAL_PRINTLN(block.lparam4);   
        SERIAL_PRINTLN(block.lparam5);                                                
        SERIAL_PRINTLN("-------------------------");   
    #endif    
    if (block.magic_number != MAGIC_NUMBER) {
        SERIAL_PRINTLN("FLASH ERROR: Magic number is corrupted !");
        return (false);
    }
    pBlock->lparam1 = block.lparam1;
    pBlock->lparam2 = block.lparam2;
    pBlock->lparam3 = block.lparam3;
    pBlock->lparam4 = block.lparam4;
    pBlock->lparam5 = block.lparam5;   
    pBlock->magic_number = block.magic_number; 
        
    return(true);
}

bool FlashHelper::testMemory() {
    SERIAL_PRINTLN("--------------------------");
    SERIAL_PRINTLN ("Performing memory self-test...");
    //SERIAL_PRINT ("\nsize of long: ");  // 4 bytes
    //SERIAL_PRINTLN (sizeof(long));
    SERIAL_PRINT ("size of StorageBlock: ");
    SERIAL_PRINTLN (sizeof(StorageBlock)); 
      
    uint32_t realSize = ESP.getFlashChipRealSize();
    uint32_t ideSize = ESP.getFlashChipSize();  // set in the IDE
    FlashMode_t ideMode = ESP.getFlashChipMode();

    SERIAL_PRINTF("Flash chip id:   %08X\n", ESP.getFlashChipId());
    SERIAL_PRINTF("Flash real size: %u\n", realSize);
    SERIAL_PRINTF("Flash IDE  size: %u\n", ideSize);
    SERIAL_PRINTF("Flash IDE speed: %u\n", ESP.getFlashChipSpeed());
    SERIAL_PRINTF("Flash IDE mode:  %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? 
        "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));

    boolean result = 1;
    if(ideSize != realSize) {
        SERIAL_PRINTLN("Flash Chip configuration in the IDE is wrong!\n");
        result = 0;
    } else {
        SERIAL_PRINTLN("Flash Chip configuration is OK.");
        result = 1;
    }
    SERIAL_PRINTLN("--------------------------");
    return (result);
}



 
