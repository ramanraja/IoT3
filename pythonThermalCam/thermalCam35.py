# Read MLX90640 thermal sensor (24x32) through UDP socket
# Detect humans by gray scale thresholding OR HSV based separation.
# use it with the Arduino program  MLX90640_Udp3.ino or  MLX90640_Udp4.ino
# manual recalibration of min and max temperatures when you press 'c' or space bar.
# running average of people count to suppress noise
# https://www.pyimagesearch.com/2015/09/21/opencv-track-object-movement/
# https://www.learnopencv.com/color-spaces-in-opencv-cpp-python/

from matplotlib import pyplot as plt
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

# -------------------------------------------------------------------------
# constants  
# -------------------------------------------------------------------------
COMM_UDP = 1
COMM_SERIAL = 2
COM_I2C = 3

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

# -------------------------------------------------------------------------
# configure
# -------------------------------------------------------------------------

print ('\nUsage: python ThermalCam.py [min_temperature] [max_tempeature]')
print()

cv_settings = pickle.load(open( "settings_90640.p", "rb"))
print (cv_settings)
print() 

COMM_TYPE = cv_settings['comm_type']
FPS = cv_settings['fps']
SERIAL_PORT = cv_settings['serial_port']
REMOTE_UDP_IP_ADDRESS = cv_settings['udp_ip']
REMOTE_UDP_PORT = cv_settings['udp_port']
SERVER_TIMEOUT = cv_settings['server_timeout']

sensor_type = cv_settings['sensor_type']
ROWS = cv_settings['rows']
COLS = cv_settings['cols']
MIN_TEMP = cv_settings['min_temp']
MAX_TEMP = cv_settings['max_temp']

USE_GRAY_SCALE = cv_settings['use_gray_scale']
INTERPOLATION = cv_settings['interpolation']
PALETTE = cv_settings['palette']
MIN_RED = np.array (cv_settings['min_red'], dtype = "uint8")
MAX_RED = np.array (cv_settings['max_red'], dtype = "uint8")

BLUR_SIZE = cv_settings['blur_size']
NEIGHBOURHOOD = cv_settings['neighbourhood']   # must be odd
CPARAM = cv_settings['cparam']
ED_CYCLES =  cv_settings['num_erode_dilate']
LOWER_THRESHOLD = cv_settings['lower_threshold']
UPPER_THRESHOLD = cv_settings['upper_threshold']
ALPHA = cv_settings['alpha']
BETA = cv_settings['beta']

N = cv_settings['running_buffer_length']
SLEEP_TIME = cv_settings['sleep_time']

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
    
# -------------------------------------------------------------------------   
# main  
# -------------------------------------------------------------------------

sensor = None

if (COMM_TYPE == COMM_UDP):
    import UdpReceiver as udp   
    sensor = udp.UdpReceiver()
    if (not sensor.open(REMOTE_UDP_IP_ADDRESS, REMOTE_UDP_PORT, SERVER_TIMEOUT)):
         print ('UDP Socket error')
         exit(1)
     
elif (COMM_TYPE == COMM_SERIAL):
    import SerialReceiver as seri   
    sensor = seri.SerialReceiver()
    # NOTE: sensor.open() has 3 parameters, so if you skip the middle one(baud), then
    # the third param has to be explicitly named *
    if (not sensor.open(SERIAL_PORT, timeout=SERVER_TIMEOUT)): 
         print ('Serial port error')
         exit(1)

else: 
    import I2CReceiver as i2cr
    sensor = i2cr.I2CReceiver()
    if (not sensor.open(FPS)):
         print ('I2C sensor error')
         exit(1)
          
sensor.start()

# running average of people count:
people_count = np.zeros(N)
index = 0 
min_temp = MIN_TEMP
max_temp = MAX_TEMP

black_img = np.zeros ((ROWS,COLS), dtype=float)
plt.figure()
plt.axis("off")
img = plt.imshow(black_img, 
        cmap=palette_strings[PALETTE], 
        interpolation=interpolation_strings[INTERPOLATION],
        vmin = min_temp, vmax = max_temp)  

print ('Press CTRL+C to exit...')
data_ptr = None 
while True:
    try:
        start_time = datetime.datetime.now()
        sensor.requestData()
        data_ptr = sensor.getData()
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
        
        # transfer to cv2 using in-memory file:
        buf = io.BytesIO()
        plt.savefig (buf, format='png')
        buf.seek(0)
        frame = np.asarray (bytearray(buf.read()), dtype=np.uint8)
        if (USE_GRAY_SCALE):
            frame = cv2.imdecode (frame, cv2.IMREAD_GRAYSCALE)  # for grayscale
        else:
            frame = cv2.imdecode (frame, cv2.IMREAD_COLOR)      # for HSV separation  
        buf.close()
        
        #-------- preprocessing ----------------
        # increase contrast, adjust brightness
        frame = cv2.convertScaleAbs(frame, alpha=ALPHA, beta=BETA) 
        # first blur, and then threshold        
        blurred = cv2.GaussianBlur(frame, (BLUR_SIZE, BLUR_SIZE), 0)
        blurred = cv2.erode(blurred , None, iterations =ED_CYCLES)
        blurred = cv2.dilate(blurred , None, iterations =ED_CYCLES)
        
        #-------- thresholding ----------------
        the_mask = None
        if (USE_GRAY_SCALE):
            # For contour finding, the foreground should be white against black background         
            #T,the_mask = cv2.threshold(blurred, 0, 255, cv2.THRESH_OTSU+cv2.THRESH_BINARY_INV) 
            #print ("Otsu's threshold: %d" % (T))
            
            #the_mask = cv2.adaptiveThreshold (blurred, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, 
            #           cv2.THRESH_BINARY_INV, NEIGHBOURHOOD, CPARAM) 
            
            # For contour finding the foreground should be white against black background 
            retval,the_mask = cv2.threshold (blurred, LOWER_THRESHOLD,UPPER_THRESHOLD, cv2.THRESH_BINARY_INV) 
    
            #NOTE : the third argument MUST be explicitly named as 'mask'
            #masked = cv2.bitwise_and(frame, frame, mask=the_mask)   
        else:
            hsv_frame = cv2.cvtColor (frame, cv2.COLOR_BGR2HSV)       
            the_mask = cv2.inRange (hsv_frame, MIN_RED, MAX_RED)     
                 
        #----------- contour detection ----------
        # finding contours destroys the original image, so make a copy if you need it later
        (junk_img, contours, hierarchy) = cv2.findContours (the_mask,   # the_mask.copy(), 
                                          #cv2.RETR_TREE,   
                                          cv2.RETR_EXTERNAL, 
                                          cv2.CHAIN_APPROX_SIMPLE)
        ccount = len(contours)
        #print ("{} contour(s) found.".format(ccount))
        if (ccount > 0):
            cntrs = sorted(contours, key = cv2.contourArea, reverse = True)
            cv2.drawContours(frame, cntrs, -1, (0, 255, 0), 1)
            #rect = np.int32(cv2.BoxPoints(cv2.minAreaRect(cntrs)))
            #cv2.drawContours(frame, [rect], -1, (0, 255, 0), 2)
            i = 0
            for c in cntrs:
                (x,y,w,h) = cv2.boundingRect(c)
                #cv2.rectangle(frame, (x,y),(x+w,y+h), (0, 255, 0), 1)
                cv2.putText(frame, str(i+1), (x,y), 
                            cv2.FONT_HERSHEY_SIMPLEX, 
                            0.7, (0,0,0), 1, cv2.LINE_AA)  
                i += 1
                
        #---------- post processing --------------
        people_count[index] = ccount
        index = (index+1) % N
        ave_count = int(np.round(np.mean(people_count)))
        cv2.putText(frame, str(ave_count), (20,70), 
                    cv2.FONT_HERSHEY_SIMPLEX, 
                    3.0, (0,0,0), 5, cv2.LINE_AA)     
                                
        cv2.imshow("Thermal Camera", frame)
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

sensor.close()
sleep (5.0)
cv2.destroyAllWindows()
print("Main thread exits.")

#-----------------------------------------------------------------------------------------------
# Configuration program: run it once before starting the main program
'''--------------------------------------------------------------------------------------------
import pickle
cv_settings = { 
    "sensor_type" : "MLX-90640",     # 24x32 thermal camera
    "rows" :  24, 
    "cols" :  32,
    "sleep_time" : 50,               # mSec between packet requests    
    "comm_type" : 3,                 # 1=UDP, 2=serial, 3=I2C
    "serial_port" : "COM12",
    "udp_ip" : "192.168.0.123",
    "udp_port" : 12345,
    "server_timeout" : 2.0,          # I2C frames per second 
    "fps" : 2,
    "interpolation": 2,              # 2 = bicubic
    "palette" :  10,                 # 10 = gray scale
    "min_temp" : 28, 
    "max_temp" : 34, 
    "use_gray_scale" : 1,            # 0 = use HSV, 1 = use grayscale
    "min_red" : [0, 100, 100], 
    "max_red" : [10, 255, 255],              
    "blur_size" : 13,                
    "neighbourhood" : 11,            # must be odd
    "cparam" : 3,
    "running_buffer_length" : 5,     # moving average buffer
    "num_erode_dilate" : 3,          # number of iterations
    "lower_threshold" : 220,         # gray scale thresholds
    "upper_threshold" : 255,
    "alpha" : 2.0,                   # contrast multiplier
    "beta" : 0.5                     # brightness adjustment
    }
pickle.dump(cv_settings , open("settings_90640.p", "wb"))
print ("Config saved as 'settings_90640.p'")    
---------------------------------------------------------------------------------------------'''



              