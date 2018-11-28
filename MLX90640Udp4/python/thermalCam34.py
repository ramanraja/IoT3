# Read MLX90640 thermal sensor (24x32) through UDP socket
# Detect humans by gray scale contours + binary threhsolding
# use it with the Arduino program  MLX90640_Udp3.ino
# manual recalibration of min and max temperatures when you press 'c' or space bar.
# running average of people count to suppress noise
# https://www.pyimagesearch.com/2015/09/21/opencv-track-object-movement/
# https://www.learnopencv.com/color-spaces-in-opencv-cpp-python/
# TODO: Otsu threshold, pseudo color, color based blob detection

#from __future__ import print_function
from matplotlib import pyplot as plt
import UdpReceiver as sensor
from time import sleep
import numpy as np
import datetime
import pickle
import math
import time
import sys
import cv2
import os
import io


# config parameters  ---------------------------------------------------

SLEEP_TIME = 50            # milli seconds between packet requests

# 24x32 thermal camera:
sensor_type = 'MLX 90640'
ROWS = 24
COLS = 32

# address of the UDP sender
REMOTE_UDP_IP_ADDRESS = "192.168.0.123"   
REMOTE_UDP_PORT_NO = 12345
UDP_TIMEOUT = 3.0

# false-color palette
PALETTE = 10                 
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
LOWER_THRESHOLD =  100
UPPER_THRESHOLD =  255

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

cv_settings = pickle.load(open( "settings_90640_UG.p", "rb"))
print (cv_settings)
print() 

REMOTE_UDP_IP_ADDRESS = cv_settings['sensor_ip']
REMOTE_UDP_PORT = cv_settings['sensor_port']
UDP_TIMEOUT = cv_settings['udp_timeout']

SLEEP_TIME = cv_settings['sleep_time']
sensor_type = cv_settings['sensor_type']
ROWS = cv_settings['rows']
COLS = cv_settings['cols']
#SERIAL_PORT = cv_settings['serial_port']
INTERPOLATION = cv_settings['interpolation']
PALETTE = cv_settings['palette']
MIN_TEMP = cv_settings['min_temp']
MAX_TEMP = cv_settings['max_temp']
 
BLUR_SIZE = cv_settings['blur_size']
NEIGHBOURHOOD = cv_settings['neighbourhood']   # must be odd
CPARAM = cv_settings['cparam']
ED_CYCLES =  cv_settings['num_erode_dilate']
LOWER_THRESHOLD = cv_settings['lower_threshold']
UPPER_THRESHOLD = cv_settings['upper_threshold']
ALPHA = cv_settings['alpha']
BETA = cv_settings['beta']
N = cv_settings['running_buffer_length']

# Now override with command line arguments
if (len(sys.argv) > 1):
    MIN_TEMP = int(sys.argv[1])
print ("Minimum temperature : {}".format (MIN_TEMP))
if (len(sys.argv) > 2):
    MAX_TEMP = int(sys.argv[2])
print ("Maximum temperature : {}".format (MAX_TEMP))
if (MIN_TEMP < 6 or MAX_TEMP > 60):
    print ("Temperature range must be between 6 and 60 degree C");
    exit(1)
    
print ("Thermal camera type: {}".format(sensor_type))
print ("Interpolation : {}".format (interpolation_strings[INTERPOLATION]))
print ("Color palette : {}".format (palette_strings[PALETTE]))
print ("NOTE: If you change the palette, you must change the HSV filter also")
print()
#exit(1) 

# main  ------------------------------------------------------------------
   
udp = sensor.ThermalUdp()
if (not udp.open(REMOTE_UDP_IP_ADDRESS, REMOTE_UDP_PORT_NO, UDP_TIMEOUT)):
     print ('UDP Socket error')
     exit(1)
         
udp.start()

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
        start_time = datetime.datetime.now()
        udp.requestData()
        data_ptr = udp.getData()
        if (len(data_ptr) == 0):
            print(" --->>> MLX sensor timed out.")
            continue
        # now the data is in the sensor's internal buffer, referenced by data_ptr
        # *** This works because this is a request-response protocol ***
        #print (len(data_ptr))
        if (len(data_ptr) != ROWS*COLS):
            print ('--- Packet error: length mismatch ---')
            print (len(data_ptr))
            continue
                            
        data_ptr = np.array (data_ptr)
        min_temp = min(data_ptr)
        max_temp =  max(data_ptr)
        print ("min : {} , max : {}".format (min_temp, max_temp))
        data_ptr = data_ptr.reshape(ROWS, COLS)
         
        img.set_array(data_ptr) 
        
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
        #rgb_frame = cv2.imdecode (rgb_frame, cv2.IMREAD_COLOR)      # from HSV separation ..
        gray_frame = cv2.imdecode (rgb_frame, cv2.IMREAD_GRAYSCALE)  # we moved to grayscale
        buf.close()
        
        # increase contrast, adjust brightness
        gray_frame = cv2.convertScaleAbs(gray_frame, alpha=ALPHA, beta=BETA) 
                
        blurred = cv2.GaussianBlur(gray_frame, (BLUR_SIZE, BLUR_SIZE), 0)
        blurred = cv2.erode(blurred , None, iterations =ED_CYCLES)
        blurred = cv2.dilate(blurred , None, iterations =ED_CYCLES)
        
        # For contour finding the foreground should be white against black background         
        #T,the_mask = cv2.threshold(blurred, 0, 255, cv2.THRESH_OTSU+cv2.THRESH_BINARY_INV) 
        #print ("Otsu's threshold: %d" % (T))
        
        #the_mask = cv2.adaptiveThreshold (blurred, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, 
        #           cv2.THRESH_BINARY_INV, NEIGHBOURHOOD, CPARAM) 
        
        # For contour finding the foreground should be white against black background 
        retval,the_mask = cv2.threshold (blurred, LOWER_THRESHOLD,UPPER_THRESHOLD, cv2.THRESH_BINARY_INV) 

        #NOTE : the third argument MUST be explicitly named as 'mask'
        #masked = cv2.bitwise_and(gray_frame, gray_frame, mask=the_mask)   
                
        # finding contours destroys the original image, so make a copy if you need it later
        (junk_img, contours, hierarchy) = cv2.findContours (the_mask,   # the_mask.copy(), 
                                          #cv2.RETR_TREE,   
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
                #cv2.rectangle(gray_frame, (x,y),(x+w,y+h), (0, 255, 0), 1)
                cv2.putText(gray_frame, str(i+1), (x,y), 
                            cv2.FONT_HERSHEY_SIMPLEX, 
                            0.7, (0,0,0), 1, cv2.LINE_AA)  
                i += 1
        people_count[index] = ccount
        index = (index+1) % N
        ave_count = int(np.round(np.mean(people_count)))
        cv2.putText(gray_frame, str(ave_count), (20,70), 
                    cv2.FONT_HERSHEY_SIMPLEX, 
                    3.0, (0,0,0), 5, cv2.LINE_AA)     
                                
        cv2.imshow("Thermal Camera", gray_frame)
        #cv2.imshow("The Mask", the_mask)
        end_time = datetime.datetime.now()        
        delta = end_time - start_time
        print("Time taken for one frame: {} seconds".format(delta))        
        key = cv2.waitKey(SLEEP_TIME) & 0xFF  # TODO: revisit the delay
        if (key==27): 
            break     
        if (key==ord('c') or key==ord(' ')): 
            print("----->>> Calibrating...")    
            min_t = math.floor(min_temp)
            max_t = math.ceil(max_temp)
            img = plt.imshow(data_ptr, 
                    cmap=palette_strings[PALETTE], 
                    interpolation=interpolation_strings[INTERPOLATION],
                    vmin = min_t, vmax = max_t)
            print ("min_t : {} , max_t : {}".format (min_t, max_t)) 
    except KeyboardInterrupt:
        break
    except Exception as e:        
        print (e)

udp.close()
sleep (5.0)
cv2.destroyAllWindows()
print("Main thread exits.")

#-----------------------------------------------------------------------------------------------
# Configuration program: run it once before starting the main program
#-----------------------------------------------------------------------------------------------
'''
---------------------------------------------------------------------------------------------'''



              