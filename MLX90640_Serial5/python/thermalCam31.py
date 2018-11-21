# All parameters just work. Do not modify any of them without a backup !
# Read MLX90640 thermal sensor (24x32) through serial port 
# Detect humans by HSV color tracker.
# use it with the Arduino program  MLX90640_Serial2.ino
# manual recalibration of min and max temperatures when you press 'c' or space bar.
# running average of people count to suppress noise
# https://www.pyimagesearch.com/2015/09/21/opencv-track-object-movement/
# https://www.learnopencv.com/color-spaces-in-opencv-cpp-python/
# TODO: Otsu threshold, pseudo color, color based blob detection

from matplotlib import pyplot as plt
from time import sleep
import thermalSerial as sensor
import numpy as np
import pickle
import math
import time
import sys
import cv2
import os
import io


# config parameters  ---------------------------------------------------

SLEEP_TIME = 50            # milli seconds between packet requests
SERIAL_PORT = 'COM12'

# 24x32 thermal camera:
sensor_type = 'MLX 90640'
ROWS = 24
COLS = 32

# false-color palette
PALETTE = 1                 
INTERPOLATION = 2
MIN_TEMP = 28               # adjust this for your dynamic range
MAX_TEMP = 41               # adjust this for your dynamic range

# thresholding; TODO: vary these parameters and try
BLUR_SIZE = 13
NEIGHBOURHOOD = 21   # must be odd
CPARAM = 3
ALPHA = 2.0     # contrast improvement
BETA = -0.4      # brightness adjustment
# erode-dilate cycles and masking thresholds
ED_CYCLES =  3

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

print ('Usage: python ThermalCam.py [serial_port] [min_temperature] [max_tempeature]')
print()

cv_settings = pickle.load(open( "settings_90640_S.p", "rb"))
print (cv_settings)
print() 

SLEEP_TIME = cv_settings['sleep_time']
sensor_type = cv_settings['sensor_type']
ROWS = cv_settings['rows']
COLS = cv_settings['cols']
SERIAL_PORT = cv_settings['serial_port']
INTERPOLATION = cv_settings['interpolation']
PALETTE = cv_settings['palette']
MIN_TEMP = cv_settings['min_temp']
MAX_TEMP = cv_settings['max_temp']
MIN_RED = np.array (cv_settings['min_red'], dtype = "uint8")
MAX_RED = np.array (cv_settings['max_red'], dtype = "uint8")
BLUR_SIZE = cv_settings['blur_size']
NEIGHBOURHOOD = cv_settings['neighbourhood']   # must be odd
CPARAM = cv_settings['cparam']
ED_CYCLES =  cv_settings['num_erode_dilate']
ALPHA = cv_settings['alpha']
BETA = cv_settings['beta']

if (len(sys.argv) > 1):
    SERIAL_PORT = sys.argv[1]
print ("Serial port : {}".format (SERIAL_PORT))
if (len(sys.argv) > 2):
    MIN_TEMP = int(sys.argv[2])
print ("Minimum temperature : {}".format (MIN_TEMP))
if (len(sys.argv) > 3):
    MAX_TEMP = int(sys.argv[3])
print ("Maximum temperature : {}".format (MAX_TEMP))

print ("Thermal camera type: {}".format(sensor_type))
print ("Interpolation : {}".format (interpolation_strings[INTERPOLATION]))
print ("Color palette : {}".format (palette_strings[PALETTE]))
print ("NOTE: If you change the palette, you must change the HSV filter also")
print()
#exit(1) 

# main  ------------------------------------------------------------------

seri = sensor.ThermalSerial()
# without timeout, packets are fragmented
if not seri.open(SERIAL_PORT, timeout=0.5):   # assuming packet interval of 250 or 330 mSec
    print ('Cannot open serial port.')
    sys.exit(1)
    
seri.start()

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

print ('Press ^C to exit...')
data_ready = False
while True:
    try:
        seri.send("S")  # prompt for data and WAIT..
         
        timeout = 50    # 5 seconds
        while (not sensor.data_ready):   
            sleep(0.1)
            timeout = timeout-1
            if (timeout <= 0): 
                break
        if (timeout <= 0): 
            print('Serial Rx timed out.')
            continue
        
        sensor.data_ready = False;    # prepare for the next packet 
        # now the data is in the global array buffer[]
        
        #print (len(sensor.data_buffer))
        if (len(sensor.data_buffer) != ROWS*COLS):
            print ('--- Packet error: length mismatch ---')
            print (len(sensor.data_buffer))
            continue
                            
        sensor.data_buffer = np.array (sensor.data_buffer)
        min_temp = min(sensor.data_buffer)
        max_temp =  max(sensor.data_buffer)
        print ("min : {} , max : {}".format (min_temp, max_temp))
        sensor.data_buffer = sensor.data_buffer.reshape(ROWS, COLS)
         
        img.set_array(sensor.data_buffer) 
        
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
        
        # increase contrast, adjust brightness
        gray_frame = cv2.convertScaleAbs(rgb_frame, alpha=ALPHA, beta=BETA) 
                
        hsv_frame = cv2.cvtColor (rgb_frame, cv2.COLOR_BGR2HSV)       
        red_pixels = cv2.inRange (hsv_frame, MIN_RED, MAX_RED)  
        
        red_pixels = cv2.GaussianBlur(red_pixels, (BLUR_SIZE, BLUR_SIZE), 0)   
        red_pixels = cv2.erode(red_pixels, None, iterations=ED_CYCLES)
        red_pixels = cv2.dilate(red_pixels, None, iterations=ED_CYCLES)
        
        # finding contours destroys the original image, so make a copy if you need it later
        (junk_img, contours, hierarchy) = cv2.findContours (red_pixels,   # red_pixels.copy(), 
                                          cv2.RETR_EXTERNAL, 
                                          cv2.CHAIN_APPROX_SIMPLE)
        ccount = len(contours)
        #print ("{} contour(s) found.".format(ccount))
        if (ccount > 0):
            cntrs = sorted(contours, key = cv2.contourArea, reverse = True)
            cv2.drawContours(rgb_frame, cntrs, -1, (0, 255, 0), 1)
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
                    3.0, (0,0,0), 5, cv2.LINE_AA)     
                                
        cv2.imshow("Thermal Camera", rgb_frame)
        
        key = cv2.waitKey(SLEEP_TIME) & 0xFF  # TODO: revisit the delay
        if (key==27): 
            break     
        if (key==ord('c') or key==ord(' ')): 
            print("----->>> Calibrating...")    
            min_t = math.floor(min_temp)
            max_t = math.ceil(max_temp)
            img = plt.imshow(sensor.data_buffer, 
                    cmap=palette_strings[PALETTE], 
                    interpolation=interpolation_strings[INTERPOLATION],
                    vmin = min_t, vmax = max_t)
            print ("min_t : {} , max_t : {}".format (min_t, max_t)) 
    except KeyboardInterrupt:
        break
    except Exception as e:        
        print (e)

seri.close()
sleep (1.0)
cv2.destroyAllWindows()
print("Main thread exits.")

#-----------------------------------------------------------------------------------------------
# Configuration program: run it once before starting the main program
#-----------------------------------------------------------------------------------------------
'''
# Save your thermal camera settings into a pickle file.
import pickle
cv_settings = { 
    "sensor_type" : "MLX-90640",
    "rows" :  24, 
    "cols" :  32,
    "serial_port": "COM12", 
    "interpolation": 2,
    "palette" :  1, 
    "min_temp" : 28, 
    "max_temp" : 34, 
    "min_red" :  [0, 100, 100], 
    "max_red" :  [10, 255, 255],    
    "sleep_time" : 50
    }
pickle.dump(cv_settings , open("settings_90640_S.p", "wb"))
print ("Config saved as 'settings_90640_S.p'")
---------------------------------------------------------------------------------------------'''



              