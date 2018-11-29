# Serial helper class with double buffers 
# Specially designed to read thermal cameras
# It works through a ON-OFF flow control system; request for a packet,
# receive and procss it, then request the next packet.

# python -m serial.tools.list_ports
# python -m serial.tools.miniterm  COM12  115200

from time import sleep
import numpy as np
import threading
import serial
import time
import sys

# global constants used by UDP server also

NEXT_PROMPT     = "N".encode('utf-8')   # send next chunk of the frame
ABORT_PROMPT    = "A".encode('utf-8')   # discard remaining chunks, start a fresh frame
RESET_PROMPT    = "S".encode('utf-8')   # sensor reset
REBOOT_PROMPT   = "R".encode('utf-8')   # reboot 8266

# for MLX90640 sensor
FRAME_LENGTH = 768    # 24x32 pixels
MAX_WAIT_CYCLES = 25  # Rx data timeout, in units of 200 mSec

class SerialReceiver (threading.Thread):

    def open(self, port='COM1', baud=115200, timeout=0.5):
        self.terminate = False
        self.data_ready = False
        self.port = port
        self.baud = baud
        self.timeout = timeout
        self.ser = None
        self.data_buffer = np.zeros(FRAME_LENGTH)
        self.datastr = "DataStringPlaceHolder"  
        try:
            print ('Opening serial port...')
            self.ser = serial.Serial(self.port, self.baud, timeout=self.timeout)
            #print (self.ser)
            return True   # no exception = success
        except Exception as e:
            print (e)   
            print (self.ser)
            return False     
        
        
    def run(self):
        if self.ser is None:   # it was never opened at the start
            print ("Serial port is not open[1]")
            return   
        self.ser.flushInput()  # discard any previous garbage
        while not self.terminate:
            try:
                if (self.ser is None):  # reopen the port
                    if not self.open(self.port, self.baud, self.timeout):
                        for i in range(10):
                            if self.terminate: break
                            sleep(1)             
                        continue
                #if self.ser.inWaiting():
                self.datastr = self.ser.readline()  # this needs '\n' at the end (TODO: use inWaiting())
                if (self.datastr is None or len(self.datastr)==0):
                    continue
                #print (len(self.datastr))
                #print (type(self.datastr))                
                #print (self.datastr)
                self.process()
            except serial.serialutil.SerialException:  # this gives it a chance to reopen the port
                print('- Cannot read serial port -')
                if (self.ser is not None):
                    self.ser.close()  # Note: do not use self.close() ! it will set terminate=True
                    self.ser = None   # this is needed for re-attempts
                    print ("Serial port closed.[1]")
            except Exception as e:
                print(e)
        self.close()
        print ('Serial thread exits.')
                
                
    def close(self):  
        self.terminate = True    # terminate the worker thread
        print ('Closing serial port...')
        try:    
            if self.ser is None:
                print ("Serial port is not open[2]")
                return   
            if not self.ser.isOpen():
                print ("Serial port is not open[3]")
                return  
        except Exception as e: 
            print (e) 
            print ("Serial port is not open[4]")
            return 
        try :       
            self.ser.flushOutput()
            self.ser.close()
            self.ser = None
            print ("Serial port closed.[2]")
        except Exception as e:
            print (e)  
            
        
    # if you want to outsource some of the preprocessing to this worker thread,
    # this is the place to do it..
    def process(self):
        try:
            #print(self.datastr)
            # The string is of the form 'b[9.9 9.9 9.9 ... 9.9 ]'   including the single quotes !   
            # Note: this is applicable for Python 3; But Python 2 continues to be without the 'b  
            self.datastr = self.datastr.decode('UTF-8')  # convert bytes to chars 
            self.datastr = self.datastr.strip()      
            #print(self.datastr)        
            if (self.datastr[0] == '#'):   # comment/info packet
                print(self.datastr)
                return
            if (self.datastr[0] != '[' or self.datastr[-1] != ']'):
                print ('-- Packet error: No delimiters --')
                return  
            self.datastr = self.datastr[1:-1]     # remove the delimiters
            self.data_buffer = [float(n) for n in self.datastr.split()]  # int(float(n))
            #print (len(self.data_buffer))
            self.data_ready = True
        except Exception as e:
            print (e)              
            
            
    def send(self, message):
        try:
            #print(message)
            self.ser.write(message.encode('utf-8'))
            self.ser.flush()
        except Exception as e:
            print (e)   
               
               
    def requestData(self):
        #print ("Sending new frame request...")
        self.data_ready = False;    # prepare for the next packet
        self.send('N')
        
        
    def isDataAvailable(self):
        return self.data_ready 
    
    
    # this is called from the client thread
    def getData(self) :  
        attempts = 0;
        while(not self.data_ready):
            sleep(0.2)
            attempts += 1
            if (attempts > MAX_WAIT_CYCLES):
                return (np.array([]))       # empty array = no data available
        return (self.data_buffer)           # return a pointer to internal data buffer *
            
                       
#----------------------------------------------------------------------------
# Place holder for client business logic
#----------------------------------------------------------------------------- 
def process_image (pixels):
    ROWS = 24  # 4   
    COLS = 32  # 16
    #print (pixels)
    print (len(pixels))
    if (len(pixels) != ROWS*COLS):
        print('- Data Error: Length mismatch -')
        print('(Expected {} rows X {} cols)'.format (ROWS,COLS))
        return
    print ("min: %d" %min(pixels), end=" , ")
    print ("max: %d" %max(pixels))    
    print()
    time.sleep(2)
#----------------------------------------------------------------------------- 
# MAIN
#----------------------------------------------------------------------------- 

def main():
    port = 'COM12'
    if (len(sys.argv) > 1):
        port = int(sys.argv[1])
    print ("Serial port:{}".format(port))
    
    seri = SerialReceiver()
    # without timeout, packets are fragmented
    if not seri.open(port, timeout=1.0):  
        print ('Cannot open serial port.')
        sys.exit(1)
        
    seri.start()
    
    data_ptr = None            
    print ('Press ^C to exit...')
    while True:
        try:
            seri.requestData()
            data_ptr = seri.getData()
            if (len(data_ptr)==0):
                print ("--- timed out -----")
            else:
                process_image (data_ptr) # example function
        except KeyboardInterrupt:
            break        
        except Exception as e:        
            print (e)
    
    seri.close()
    sleep (1.0)
    print("Main thread exits.")       
     
if __name__ == "__main__":
    main()   
    
          