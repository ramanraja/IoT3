# All parameters just work. Do not modify any of them without a backup !
# Read GridEye AMG88xx thermal sensor (8x8) through UDP on air.
# Detect humans by HSV color tracker.
# use it with the Arduino program  GridEye9.ino
# manual recalibration of min and max temperatures when you press 'c'  or space bar.
# running average of people count.
# https://www.pyimagesearch.com/2015/09/21/opencv-track-object-movement/
# https://www.learnopencv.com/color-spaces-in-opencv-cpp-python/
 

from matplotlib import pyplot as plt
from time import sleep
import numpy as np
import socket
import pickle
import math
import time
import sys
import cv2
import os
import io


# config parameters  ---------------------------------------------------

SLEEP_TIME = 50            # milli seconds between packet requests

# 16x4 thermal camera:
sensor_type = 'AMG 88xx'
ROWS = 8
COLS = 8

# sensor UDP server
REMOTE_UDP_IP_ADDRESS = "192.168.0.110"   # thermal sensor device IP
REMOTE_UDP_PORT = 12345
data_packet_size = 512      # this is only for 16x4 thermal camera ***

# false-color palette
PALETTE = 1                 
INTERPOLATION = 2
MIN_TEMP = 28               # adjust this for your dynamic range
MAX_TEMP = 36               # adjust this for your dynamic range

# H ranges from 0 to 180 degrees
# S and V range from 0 to 255

# the following values worked well for bwr palette:
MIN_RED = np.array([0, 100,  100], dtype = "uint8")  
MAX_RED = np.array([10, 255, 255], dtype = "uint8")

#MIN_RED = np.array([0, 120, 150], dtype = "uint8")
#MAX_RED = np.array([10, 255, 255], dtype = "uint8")
 
# constants  -----------------------------------------------------------

# https://matplotlib.org/gallery/images_contours_and_fields/interpolation_methods.html

interpolation_strings = ['none', 'bilinear', 'bicubic', 'quadric', 'gaussian' ,
    'spline16', 'spline36', 'hanning', 'hamming', 'hermite', 
    'kaiser', 'catrom', 'bessel', 'mitchell', 'sinc', 
    'lanczos', 'nearest']

# https://matplotlib.org/users/colormaps.html

palette_strings = ['flag', 'bwr', 'coolwarm', 'RdYlGn', 'seismic', 
    'BrBG', 'RdGy', 'winter', 'YlOrRd', 'YlOrBr',
    'Greys', 'Greens', 'Oranges', 'Reds', 'bone', 
    'RdYlBu', 'cool', 'gnuplot']

# configure----------------------------------------------------------------

print ('Usage: python ThermalCam.py  [min_temperature] [max_tempeature]')
print()

cv_settings = pickle.load(open( "settings_AMG88_U.p", "rb"))
print (cv_settings)
print() 

SLEEP_TIME = cv_settings['sleep_time']
sensor_type = cv_settings['sensor_type']
ROWS = cv_settings['rows']
COLS = cv_settings['cols']
data_packet_size = cv_settings['packet_size']
REMOTE_UDP_IP_ADDRESS = cv_settings['sensor_ip']
REMOTE_UDP_PORT = cv_settings['sensor_port']
INTERPOLATION = cv_settings['interpolation']
PALETTE = cv_settings['palette']
MIN_TEMP = cv_settings['min_temp']
MAX_TEMP = cv_settings['max_temp']
MIN_RED = np.array (cv_settings['min_red'], dtype = "uint8")
MAX_RED = np.array (cv_settings['max_red'], dtype = "uint8")

if (len(sys.argv) > 1):
    MIN_TEMP = int(sys.argv[1])
print ("Minimum temperature : {}".format (MIN_TEMP))
if (len(sys.argv) > 2):
    MAX_TEMP = int(sys.argv[2])
print ("Maximum temperature : {}".format (MAX_TEMP))

print ("Thermal camera type: {}".format(sensor_type))
print ("Interpolation : {}".format (interpolation_strings[INTERPOLATION]))
print ("Color palette : {}".format (palette_strings[PALETTE]))
print ("NOTE: If you change the palette, you must change the HSV filter also")
print()
#exit(1) 

# main  ------------------------------------------------------------------

prompt_string = "D"
prompt = prompt_string.encode('utf-8')
clientSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
clientSock.settimeout(1.0)  # seconds
data_buffer = []

# running average of people count:
N = 10
people_count = np.zeros(N)
index = 0 

pixels = np.zeros ((ROWS,COLS), dtype=float)
plt.figure()
plt.axis("off")

min_temp = MIN_TEMP
max_temp = MAX_TEMP
img = plt.imshow(pixels, 
        cmap=palette_strings[PALETTE], 
        interpolation=interpolation_strings[INTERPOLATION],
        vmin = min_temp, vmax = max_temp)  

print ('Press ^C or ESC to exit...')
while True:
    try:
        clientSock.sendto(prompt, (REMOTE_UDP_IP_ADDRESS, REMOTE_UDP_PORT))
        data, addr = clientSock.recvfrom (data_packet_size)
        #print('read has returned.')
        if (len(data) < 1):
            #print ('timed out !')
            continue
        #print(addr)
        #print(data)          
        decoded_data = data.decode('utf-8')
        #decoded_data = decoded_data.strip()
        if (decoded_data[0] != '[' or decoded_data[-1] != ']'):
            print ('--- packet error: no delimiters ---')
            continue        
        decoded_data = decoded_data[1:-1]     # remove the delimiters
        data_buffer = [float(n) for n in decoded_data.split()]    
 
        #print (len(data_buffer))
        if (len(data_buffer) != ROWS*COLS):
            print ('--- Packet error: length mismatch ---')
            print (len(data_buffer))
            continue
                            
        data_buffer = np.array (data_buffer)
        min_temp = min(data_buffer)
        max_temp =  max(data_buffer)
        print ("min : {} , max : {}".format (min_temp, max_temp))
        data_buffer = data_buffer.reshape(ROWS, COLS)
         
        img.set_array(data_buffer) 
        
        #------------------------------------------ 
        # this also works:
        #plt.savefig('frame.png', bbox_inches='tight')
        #frame = cv2.imread('frame.png')
        #-------------------------------------------
        
        # using in-memory file:
        buf = io.BytesIO()
        plt.savefig (buf, format='png')
        buf.seek(0)
        rgb_frame = np.asarray (bytearray(buf.read()), dtype=np.uint8)
        rgb_frame = cv2.imdecode (rgb_frame, cv2.IMREAD_COLOR)
        buf.close()
        
        hsv_frame = cv2.cvtColor (rgb_frame, cv2.COLOR_BGR2HSV)       
        red_pixels = cv2.inRange (hsv_frame, MIN_RED, MAX_RED)  
        
        red_pixels = cv2.GaussianBlur(red_pixels, (11, 11), 0)   # TODO: parameterize these
        red_pixels = cv2.erode(red_pixels, None, iterations=3)
        red_pixels = cv2.dilate(red_pixels, None, iterations=3)
        
        # finding contours destroys the original image, so make a copy if you need it later
        (junk_img, contours, hierarchy) = cv2.findContours (red_pixels,   # red_pixels.copy(), 
                                          cv2.RETR_EXTERNAL, 
                                          cv2.CHAIN_APPROX_SIMPLE)
        ccount = len(contours)
        #print ("{} contour(s) found.".format(ccount))
        if (ccount > 0):
            cntrs = sorted(contours, key = cv2.contourArea, reverse = True)
            cv2.drawContours(rgb_frame, cntrs, -1, (0, 255, 0), 2)
            #rect = np.int32(cv2.BoxPoints(cv2.minAreaRect(cntrs)))
            #cv2.drawContours(rgb_frame, [rect], -1, (0, 255, 0), 2)
            i = 0
            for c in cntrs:
                (x,y,w,h) = cv2.boundingRect(c)
                #cv2.rectangle(rgb_frame, (x,y),(x+w,y+h), (0, 255, 0), 1)
                cv2.putText(rgb_frame, str(i+1), (x,y), 
                            cv2.FONT_HERSHEY_SIMPLEX, 
                            0.7, (0,0,0), 1, cv2.LINE_AA)  
                i += 1
        people_count[index] = ccount
        index = (index+1) % N
        ave_count = int(np.round(np.mean(people_count)))
        cv2.putText(rgb_frame, str(ave_count), (20,70), 
                    cv2.FONT_HERSHEY_SIMPLEX, 
                    4.0, (0,0,0), 5, cv2.LINE_AA)     
                                
        cv2.imshow("Thermal Camera", rgb_frame)
        
        key = cv2.waitKey(SLEEP_TIME) & 0xFF  # TODO: revisit the delay
        if (key==27): 
            break     
        if (key==ord('c') or key==ord(' ')): 
            print("----->>> Calibrating...")    
            min_t = math.floor(min_temp)
            max_t = math.ceil(max_temp)
            img = plt.imshow(data_buffer, 
                    cmap=palette_strings[PALETTE], 
                    interpolation=interpolation_strings[INTERPOLATION],
                    vmin = min_t, vmax = max_t)
            print ("min_t : {} , max_t : {}".format (min_t, max_t)) 
    except KeyboardInterrupt:
        break
    except Exception as e:        
        print (e)

sleep (1.0)
cv2.destroyAllWindows()
print("Main thread exits.")

#-----------------------------------------------------------------------------------------------
# Configuration program: run it once before starting the main program
#-----------------------------------------------------------------------------------------------
'''
import pickle

cv_settings = { 
    "sensor_type" : "AMG-88xx",
    "rows" :  8, 
    "cols" :  8,
    "packet_size" : 512, 
    "sensor_ip": "192.168.0.109", 
    "sensor_port": 12345, 
    "interpolation": 2,
    "palette" :  1, 
    "min_temp" : 28, 
    "max_temp" : 34, 
    "min_red" :  [0, 120, 150], 
    "max_red" :  [10, 255, 255],    
    "sleep_time" : 50
    }
    
pickle.dump(cv_settings , open("settings_AMG88_U.p", "wb"))
print ("Config saved as 'settings_AMG88_U.p'")
---------------------------------------------------------------------------------------------'''



              