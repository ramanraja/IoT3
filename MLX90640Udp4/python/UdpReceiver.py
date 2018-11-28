'''
CAUTION: all the time outs and delays were working perfectly with MLX90640_Udp3.ino,
which sends actual sensor data. Do not change any timings !!
socket timeout: 3 sec
if no chunk received in response to NEXT prompt, sleep 300 mSec and try again.
on chunk error, send ABORT and sleep for 5 sec.
Client Application thread timeout: 15 sec.
 
TODO:
print timestamps
log all errors with timestamp
if a duplicate chunk is received, ignore it and move on ?
if there is an error, wait for a random delay and then send Abort signal.
'''
# UDP client Class to receive large text packets from 8266 in chunks of 64 floats.
# keep sending prompts'N' for next chunk; on error sends 'A' for abort.
# use it with  MLX90640Udp3.ino which sends actual sensor data
# chunk structure: [x|99.99 99.99 ....99.99] where x is the chunk number in HEX.
# Note: chunk number is a single Hex digit; so maximum of 16 chunks per frame.

#from __future__ import print_function
from time import sleep
import numpy as np
import threading
import socket
import time
import sys

#-------------------------------------------------------------------------------------------
# global constants used by UDP server also

NEXT_PROMPT     = "N".encode('utf-8')   # send next chunk of the frame
ABORT_PROMPT    = "A".encode('utf-8')   # discard remaining chunks, start a fresh frame
RESET_PROMPT    = "S".encode('utf-8')   # sensor reset
REBOOT_PROMPT   = "R".encode('utf-8')   # reboot 8266

# parameters for MLX90640 sensor
            
PACKET_SIZE = 512                               # buffer size to receive on-air packets           
CHUNK_LENGTH = 64                               # every chunk has 64 floats
CHUNKS_PER_FRAME = 12                           # 768 floats are sent as 12 chunks  
FRAME_LENGTH = CHUNKS_PER_FRAME * CHUNK_LENGTH  # a frame has 24*32 = 768 pixels

RUN_LENGTH = 10                                 # number of samples to compute running min/max/median
MAX_WAIT_CYCLES = 150                           # application Rx timeout; in 200 mSec blocks 
#-----------------------------------------------------------------------------------------
   
class ThermalUdp (threading.Thread):

    def __init__ (self):
        threading.Thread.__init__(self)
        self.terminate = False
        self.in_waiting = False
        self.is_data_available = False
        self.datastr = "[DataStringPlaceHolder]"
        self.data = np.zeros(FRAME_LENGTH)
        self.run_min_buf = np.zeros(RUN_LENGTH)
        self.run_max_buf = np.zeros(RUN_LENGTH)
        self.run_median_buf = np.zeros(RUN_LENGTH)
        self.run_min = 0.0
        self.run_max = 0.0
        self.run_median = 0.0        
        self.running_index = 0
        self.frame_min = 0.0
        self.frame_max = 0.0
        self.frame_median = 0.0
        self.frame_count = 0
        self.frame_errors = 0
        self.duplicate_chunks = 0
        self.sock = None
        self.server_ip = "0.0.0.0"
        self.server_port = 12345
        self.timeout = 0.0
         
                            
    def open (self, serverIP, serverPort, timeout=0.5):
        self.server_ip = serverIP
        self.server_port = serverPort
        self.timeout = timeout
        try:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self.sock.settimeout(self.timeout)  # seconds
            return True
        except Exception as e:
            print (e)
            self.sock = None
            return False
        
        
    def isOpen(self):
        return (self.sock is not None)
        
            
    def close (self):
        self.terminate = True    # terminate the worker thread
        print ('Closing UDP port...')
        if self.sock is None:
            print ("UDP socket is not open [2].")
            return   
        try :       
            self.sock.close()
            self.sock = None
            print ("UDP socket closed.")
        except Exception as e:
            print (e)          
        
        
    def run (self):
        if self.sock is None:    
            print ("UDP socket is not open [1].")
            return   
        try:
            self.sock.sendto (ABORT_PROMPT,(self.server_ip,self.server_port)) # reset the server
            dummy = self.sock.recv (4*PACKET_SIZE)  # flush input
        except Exception as e:
            print (e)
        while not self.terminate:
            if (not self.in_waiting): # not expecting any data at the moment
                sleep(0)  # equivalent to yielding CPU time
                continue
            next_chunk_num = 0
            try:    
                # iterate over all the chunks of a frame
                while (next_chunk_num < CHUNKS_PER_FRAME): # TODO: watchdog to force out of this loop
                    try:
                        self.sock.sendto (NEXT_PROMPT,(self.server_ip,self.server_port))
                        self.datastr = self.sock.recv (PACKET_SIZE)
                        if (len(self.datastr)==0): # discard 'empty packets' ?
                            sleep(0.3)
                            continue    # keep sending NEXT_PROMPT
                        self.datastr = self.datastr.decode('utf-8')
                        #print (self.datastr)
                        print ('chunk length: {}'.format(len(self.datastr)))
                        if (self.datastr[0] == '#'):   # it was a comment/FYI packet
                            print(self.datastr)
                            continue   # send NEXT_PROMPT
                        # validate and store the chunk
                        if (not self.store(next_chunk_num)):
                            self.frame_errors = (self.frame_errors+1)
                            sleep(0.01)
                            self.sock.sendto (ABORT_PROMPT,(self.server_ip,self.server_port))
                            next_chunk_num = 0    # must get a fresh frame next time
                            # TODO: flush input buffer
                            sleep(5.0)     
                            dummy = self.sock.recv (4*PACKET_SIZE)                       
                            break                 # break inner loop (over the chunks)
                        # chunk received successfully, advance to the next chunk
                        next_chunk_num = (next_chunk_num+1)    
                    except socket.timeout:
                        #print('- Timed out -')  
                        print('.', end='')
                # got all the chunks, now we are out of the inner loop  
                self._process_data ()             # just for illustration; can comment out 
                self.in_waiting = False   
                self.is_data_available = True     # if watchdog bites, you must change this line    
                self.frame_count = (self.frame_count+1)
                print ('------------------- end of frame --------------------')   
            except Exception as e:
                print(e)  
        print ('Worker thread exits.')
        
        
    def store(self, expected_chunk):
        try :
            if (self.datastr[0] != '[' or self.datastr[-1] != ']' or self.datastr[2] != '|') :
                print ('--- Delimiter error ---')
                return False
            chunk_received = int(self.datastr[1], 16)
            print ('Expected chunk:{} , received:{}'.format(expected_chunk, chunk_received))
            if (chunk_received == expected_chunk-1):  
                 print ('--- Duplicate chunk ---')
                 #return True  # just ignore, but move on to the next chunk?
                 return False            
            if (chunk_received != expected_chunk):
                print ('--- Chunk # mismatch ---')
                return False
            self.datastr = self.datastr[3:-1]     # remove delimiters and chunk number
            temp_data = [float(n) for n in self.datastr.split()]
            if (len(temp_data) != CHUNK_LENGTH):
                print ('--- Chunk length is wrong ---')
                return False
            # store the chunk at the correct location in the frame
            self.data [expected_chunk*CHUNK_LENGTH : (expected_chunk+1)*CHUNK_LENGTH] = temp_data
            return True    
        except Exception as e:
            print(e)
            return False
            
        
    def requestData(self):
        print ("\nSending new frame request...")
        self.in_waiting = True  # this will nudge the thread out of its rut  
        self.is_data_available = False
        
        
    def isDataAvailable(self):
        return self.is_data_available 
    
    
    def getData(self) :
        attempts = 0;
        while(not self.is_data_available):
            sleep(0.2)
            attempts += 1
            if (attempts > MAX_WAIT_CYCLES):
                return (np.array([]))  # empty array
        return (self.data)             # * return a pointer to internal data buffer *
            
            
    
    # if you want to outsource some of the data processing to this worker thread,
    # this is the place to do it..
    # NOTE: this is an internal method. Do not call it asynchronously from outside the class
    def _process_data (self):
        self.frame_min = np.amin(self.data)
        self.frame_max = np.amax(self.data)
        self.frame_median = np.median(self.data)
        
        self.run_min_buf [self.running_index] = self.frame_min
        self.run_max_buf [self.running_index] = self.frame_max
        self.run_median_buf [self.running_index] = self.frame_median
        self.running_index = (self.running_index+1) % RUN_LENGTH
        
        self.run_min = np.amin(self.run_min_buf)
        self.run_max = np.amax(self.run_max_buf)  
        self.run_median = np.median(self.run_median_buf)  
        print ("Frame Min:{}, Max:{}, Median:{}".format(self.frame_min, self.frame_max, self.frame_median))
        print ("Running Min:{}, Max:{}, Median:{}".format(self.run_min, self.run_max, self.run_median))    
                                
    
    def printStats(self):
        print ("Good frames received: {}".format(self.frame_count))
        print ("Bad frames aborted: {}".format(self.frame_errors))
        print ("   of these, duplicates: {}".format(self.duplicate_chunks))
        self.frame_count = 0      # reset the stats every time you read it (?)
        self.frame_errors = 0    
        self.duplicate_chunks = 0
        
#----------------------------------------------------------------------------- 
# MAIN
#----------------------------------------------------------------------------- 

def main():
    # address of the UDP sender
    REMOTE_UDP_IP_ADDRESS = "192.168.0.123"   
    REMOTE_UDP_PORT_NO = 12345
    TIMEOUT = 3.0
    FRAME_DELAY = 10.0
    
    udp = ThermalUdp()
    if (not udp.open(REMOTE_UDP_IP_ADDRESS, REMOTE_UDP_PORT_NO, TIMEOUT)):
         print ('UDP Socket error')
         exit(1)
    udp.start()
    
    print ("Press CTRL+C to quit...")
    
    while True:
        try :
            udp.requestData()
            dat = udp.getData()
            if (len(dat) == 0):
                print(" --->>> could not contact sensor")
            else:
                print ("Client received  a frame.")    
            sleep (FRAME_DELAY)
        except KeyboardInterrupt:
            print ("CTRL+C pressed.")
            break
        except Exception as e:
            print(e)            
    
    udp.printStats()
    udp.close()
    sleep (1.0)
    print("Main thread exits.") 

if __name__ == "__main__":
    main()  
