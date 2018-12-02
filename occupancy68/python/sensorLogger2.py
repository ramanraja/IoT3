# receives PIR and Radar status from Occupancy sensor
# and stores in a series of CSV files.
# use it with Occupancy67.ino

import datetime
import socket
import time
import csv

def write_log (status):  
    global f, record_count, prev_ts
    ts = datetime.datetime.now()
    tdelta = (ts - prev_ts)
    if (tdelta.microseconds < 500000):
        tdelta = tdelta.seconds 
    else:
        tdelta = tdelta.seconds+1
    prev_ts = ts
    f.write(ts.strftime("%Y-%m-%d %H:%M:%S")); f.write(",") 
    f.write(str(device_id)); f.write(",")    
    f.write(str(data)); f.write(",")      
    f.write(str(tdelta)); f.write("\n")                   
    record_count += 1
    if (record_count >= 1000):   
        record_count = 0
        print("flushing...")
        f.flush()
        f.close()
        f = open(file_name, "a+")
#-------------------------------------------------------------------
# all these are globals:
 
device_id = 1  
record_count = 0
prev_ts = datetime.datetime.now()

ts = datetime.datetime.now()
file_name = ts.strftime("log_%Y-%m-%d_%H.%M.%S.csv")
print (file_name)
f = open (file_name, "w")
f.write("time,device_id,status,delta\n")

 
# static IP address of the occupancy sensor  
REMOTE_UDP_IP_ADDRESS = "192.168.0.109"   
REMOTE_UDP_PORT_NO = 12345
data_packet_size = 3   # the data is a single digit
prompt_str = "S"
prompt = prompt_str.encode ('utf-8')
clientSock = socket.socket (socket.AF_INET, socket.SOCK_DGRAM)
clientSock.settimeout(2.1)  # seconds


print ("Press ^C to quit...")
packet_count=0
while True:
    try :
        if (packet_count == 0):
            #print('Prompting...')
            clientSock.sendto(prompt, (REMOTE_UDP_IP_ADDRESS, REMOTE_UDP_PORT_NO))
        packet_count = (packet_count + 1) % 50
        data, addr = clientSock.recvfrom (data_packet_size)
        #print(addr)
        #print(len(data))
        if (len(data) > 0):
            data = data.decode('utf-8')
            print (data)
            write_log(data)
    except Exception as e:
        print(e)        
    except KeyboardInterrupt:
        print ("^C pressed.")
        break

clientSock.close()
f.flush()
f.close()
print("Bye!")        
