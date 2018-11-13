# All parameters just work. Do not modify any of them without a backup !
# Read MLX90621 thermal sensor through serial port 
# Detect humans by HSV color tracker.
# use it with the Arduino program  MLX90621_Serial2.ino
# manual recalibration of min and max temperatures when you press 'c'
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

# 16x4 thermal camera:
sensor_type = 'MLX 90621'
ROWS = 4
COLS = 16

# false-color palette
PALETTE = 1                 
INTERPOLATION = 2
MIN_TEMP = 28               # adjust this for your dynamic range
MAX_TEMP = 41               # adjust this for your dynamic range

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

cv_settings = pickle.load(open( "settings_90621_S.p", "rb"))
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
        
        hsv_frame = cv2.cvtColor (rgb_frame, cv2.COLOR_BGR2HSV)       
        red_pixels = cv2.inRange (hsv_frame, MIN_RED, MAX_RED)  
        
        red_pixels = cv2.GaussianBlur(red_pixels, (11, 11), 0)   
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
            #cv2.drawContours(rgb_frame, cntrs, -1, (0, 255, 0), 2)
            #rect = np.int32(cv2.BoxPoints(cv2.minAreaRect(cntrs)))
            #cv2.drawContours(rgb_frame, [rect], -1, (0, 255, 0), 2)
            i = 0
            for c in cntrs:
                (x,y,w,h) = cv2.boundingRect(c)
                cv2.rectangle(rgb_frame, (x,y),(x+w,y+h), (0, 255, 0), 1)
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
    "sensor_type" : "MLX-90621",
    "rows" :  4, 
    "cols" :  16,
    "serial_port": "COM12", 
    "interpolation": 2,
    "palette" :  1, 
    "min_temp" : 28, 
    "max_temp" : 34, 
    "min_red" :  [0, 120, 150], 
    "max_red" :  [10, 255, 255],    
    "sleep_time" : 50
    }
pickle.dump(cv_settings , open("settings_90621_S.p", "wb"))
print ("Config saved as 'settings_90621_S.p'")
#-----------------------------------------------------------------------------------------------
# Arduino
#-----------------------------------------------------------------------------------------------
// Streams data from MLX90621 through Serial port
// sends it as a string of floats delimited by [ and ]
// If the sensor reading is NAN or is out of bounds, resets the sensor through a 2N2222.
// test it with thermalSerial.py or  "python -m serial.tools.miniterm  COM3  115200"
 
// MLX90621_Arduino_Processing code is downloaded  from
// https://github.com/robinvanemden/MLX90621_Arduino_Processing
// and modified by Rajaraman in Oct 2018 to replace i2c_t3 library with standard Wire library
// Original code and discussion at:
// http://forum.arduino.cc/index.php/topic,126244.msg949212.html#msg949212

#include "MLX90621_Udp.h"

Timer T;
Config C;
OtaHelper O;

boolean use_static_IP = false;
IPAddress udp_ip(192,168,0,109);          // static IP,if any, of this sensor device
IPAddress udp_gateway(192,168,0,1);
IPAddress udp_subnet(255,255,255,0);
  
char response_packet[OUT_BUFFER_SIZE];      
int  data_timer = DATA_TIMEOUT; // time out for no incoming data requests (x seconds * 2)
bool data_timed_out = true;
      
MLX90621 sensor; //  an instance of the Sensor class
      
// Comment out any one of the following two options:      
//int  FPS_param = 16;   // thermal cam 8 frames per second
int  FPS_param = 4;   // thermal cam 2 frames per second

// Never use D1 and D2 for anything else - they are meant for I2C
int  led1 = 16;  // D0
int  sensor_enable_pin = 12;  // D6 
 
void setup(){ 
    pinMode (led1, OUTPUT);
    pinMode(sensor_enable_pin, OUTPUT);
    digitalWrite(sensor_enable_pin, HIGH);
    blinker();
        
    Serial.begin(115200); 
    SERIAL_PRINTLN("\n# Serial thermal camera MLX 90621 starting...");
       
    C.init();
    sensor.initialise (FPS_param);  
    SERIAL_PRINTLN("# MLX Sensor initialized.");
    init_wifi();     
    O.init(&C);    

    T.every(3600000L, check_for_updates);  //  every hour
}

void loop(){
    T.update();   
    if (WiFi.status() != WL_CONNECTED)
        init_wifi();
    if (!Serial.available())
        return;
    while (Serial.available())
        Serial.read();        // drain the input
    send_data();
}

char tmp[12];     
double data;
// data will be sent when a request is received through serial port
void send_data() {    
  
    sensor.measure(); // collect pixel data from thermal sensor

    response_packet[0] = '[';   // start a new packet
    response_packet[1] = '\0';    
    
    for(int y=0; y<ROWS; y++) {       // y = 4 rows
        for(int x=0; x<COLS; x++) {   // x = 16 columns
            data = sensor.getTemperature(y+x*4); // read the temperature at position (x,y)
            if (isnan(data) || data < LOWER_LIMIT || data > UPPER_LIMIT) {
               // reset_sensor();  // TODO: check if MOST pixels are nan, then only reset
               // return;
              #ifdef VERBOSE_MODE
                SERIAL_PRINT("# ----->>>> PIXEL ERROR: pixel= ");  
                SERIAL_PRINT(x);
                SERIAL_PRINT("  value= ");
                SERIAL_PRINTLN(data);
              #endif
              data =0.0f;  // TODO: replace with neighbouring values!            
            }
            // dtostrf(FLOAT, WIDTH, PRECSISION, BUFFER)
            dtostrf(data,1,2,tmp); 
            strcat(response_packet, tmp);
            strcat(response_packet, " ");
        }
    }
    strcat(response_packet, "]");
    SERIAL_PRINTLN(response_packet);  // the ending newline is needed for the client
}

void reset_sensor() {
  SERIAL_PRINTLN("# -------------->>>> Sensor error ! resetting...");
  digitalWrite(sensor_enable_pin, LOW);
  delay(2000);
  digitalWrite(sensor_enable_pin, HIGH);
  delay(500);  
  //// TODO: Wire.begin();
  sensor.initialise (FPS_param);
  delay(500);    
  // TODO: send messge to remote listener  
}

void check_for_updates(){
  O.check_and_update();
}

void init_wifi() {
  SERIAL_PRINT("# Connecting to ");
  SERIAL_PRINTLN(C.wifi_ssid1);
  WiFi.begin(C.wifi_ssid1, C.wifi_password1);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    //SERIAL_PRINT(".");
  }
  SERIAL_PRINTLN("# WiFi Connected.");
  if (use_static_IP) {
      WiFi.config(udp_ip, udp_gateway, udp_subnet);  // TODO: take them from Config
      SERIAL_PRINTLN("# WiFi Configured.");
  }
//  SERIAL_PRINTLN("# IP address: "); 
//  SERIAL_PRINTLN(WiFi.localIP());
}

void blinker() {
    for (int i=0; i<5; i++) {
        digitalWrite(led1, LOW); 
        delay(200);
        digitalWrite(led1, HIGH); 
        delay(200);    
    }
}
---------------------------------------------------------------------------------------------'''



              