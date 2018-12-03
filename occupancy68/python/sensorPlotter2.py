# receives PIR and Radar status from Occupancy sensor
# and displays a moving graph.
# use it with Occupancy67.ino

import socket
import threading
import numpy as np
from time import sleep
from random import randint
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import matplotlib.animation as animation
 

def acquire_data(): 
    global  packet_count, last_data
    while (not terminate):
        try :
            if (packet_count == 0):
                clientSock.sendto(prompt, (REMOTE_UDP_IP_ADDRESS, REMOTE_UDP_PORT_NO))
            packet_count = (packet_count + 1) % 50     # even if there is no data, keep increasing packet_count
            data = clientSock.recv (data_packet_size)
            #print(len(data))
            if (len(data) > 0):
                data = data.decode('utf-8')
                print (data)
                last_data = int(data)
        except Exception as e:
            print(e)      
            #buffer[index] = randint(0,3)  # simulate !
            #index = (index+1) % N            
        
#------------------------------------------------------------------------------------

def get_data():
    global buffer, index
    # TODO: check if data is ready, or wait?
    buffer[N-1] = last_data
    buffer = np.roll(buffer, -1)
    return (buffer)
    
          
def animate(i):
    line.set_ydata (get_data())  
    return line,
#------------------------------------------------------------------------------------

# globals

terminate = False
last_data = 0
packet_count=0

N = 100
x = np.arange(N)
buffer = np.zeros(N)
    
# static IP address of the occupancy sensor  
REMOTE_UDP_IP_ADDRESS = "192.168.0.109"   
REMOTE_UDP_PORT_NO = 12345
data_packet_size = 3   # the data is a single digit
prompt_str = "S"       # prompt is needed only once initially to register with UDP server
prompt = prompt_str.encode ('utf-8')

clientSock = socket.socket (socket.AF_INET, socket.SOCK_DGRAM)
clientSock.settimeout(2.1)  # seconds

# the next 4 lines should be in the exact order as below:
plt.rcParams['toolbar'] = 'None'
fig, ax = plt.subplots(figsize=(12,3.5))  # default: 6.4 x 4.8 inches
#plt.axis("off")
fig.canvas.set_window_title("Sensor status")
ax.xaxis.set_visible(False)
ax.yaxis.set_major_locator(ticker.MultipleLocator(1))
ax.yaxis.set_major_formatter(ticker.FixedFormatter(['x', 'NONE','RADR','PIR','BOTH']))
# This should be called after all axes have been added
fig.tight_layout()

ax.set_xlim(0, N-3)
ax.set_ylim(-0.1, 3.2)
ax.plot ((0,N-1), (0,0), color='gray', ls='dotted')
ax.plot ((0,N-1), (1,1), color='gray', ls='dotted')
ax.plot ((0,N-1), (2,2), color='gray', ls='dotted')
ax.plot ((0,N-1), (3,3), color='gray', ls='dotted')
line, = ax.plot(x,buffer,'r-')


thr = threading.Thread(target=acquire_data) 
thr.start()

ani = animation.FuncAnimation(fig, animate, np.arange(1,25), interval=500, blit=True)
plt.show()   # this is an infinite loop

terminate = True
clientSock.close()
sleep (2.5)
print ("Main thread exits.")       


