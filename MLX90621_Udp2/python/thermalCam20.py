# read MLX90621 thermal sensor & detect objects by HSV color tracker.
# use it with the Arduino program  MLX90621_Udp2.ino
# running average of people count to suppress noise
# https://www.pyimagesearch.com/2015/09/21/opencv-track-object-movement/
# https://www.learnopencv.com/color-spaces-in-opencv-cpp-python/
# TODO: different thresholds, pseudo color, color based blob detection
# TODO: histogramEqualization

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
sensor_type = 'MLX 90621'
ROWS = 4
COLS = 16

# sensor UDP server
REMOTE_UDP_IP_ADDRESS = "192.168.0.109"   # thermal sensor device IP
REMOTE_UDP_PORT = 12345
data_packet_size = 512      # this is only for 16x4 thermal camera ***

# false-color palette
PALETTE = 10   # gray scale ***                 
INTERPOLATION = 2
MIN_TEMP = 28               # adjust this for your dynamic range
MAX_TEMP = 41               # adjust this for your dynamic range

# thresholding
BLUR_SIZE = 9
neighbourhood = 11   # must be odd
cparam = 3
 
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
print ('Usage: python ThermalCam.py [min_temperature] [max_tempeature]')
print()

cv_settings = pickle.load(open( "settings_90621.p", "rb" ))
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
 

if (len(sys.argv) > 1):
    MIN_TEMP = int(sys.argv[1])
print ("Minimum temperature : {}".format (MIN_TEMP))
if (len(sys.argv) > 2):
    MAX_TEMP = int(sys.argv[2])
print ("Maximum temperature : {}".format (MAX_TEMP))

print ("Thermal camera type: {}".format(sensor_type))
print ("Sensor IP address: {}".format (REMOTE_UDP_IP_ADDRESS))
print ("Sensor UDP port: {}".format (REMOTE_UDP_PORT))
print ("Interpolation : {}".format (interpolation_strings[INTERPOLATION]))
print ("Color palette : {}".format (palette_strings[PALETTE]))
print ("NOTE: If you change the palette, you must change the HSV filter also")
print()

#exit(1) 

# main  ------------------------------------------------------------------

prompt_string = "send"
prompt = prompt_string.encode('utf-8')
clientSock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
clientSock.settimeout(1.0)  # seconds

# running average of people count:
N = 10
buffer = np.zeros(N)
index = 0 

pixels = np.zeros ((ROWS,COLS), dtype=float)
plt.figure()
plt.axis("off")

img = plt.imshow(pixels, 
        cmap=palette_strings[PALETTE], 
        interpolation=interpolation_strings[INTERPOLATION],
        vmin = MIN_TEMP, vmax = MAX_TEMP)  

print ('Press ^C to exit...')
while True:
    try:
        #print('sending request...')
        clientSock.sendto(prompt, (REMOTE_UDP_IP_ADDRESS, REMOTE_UDP_PORT))
        #print('reading data...')
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
        pixels = [float(n) for n in decoded_data.split()]   
        #print (len(pixels))
        if (len(pixels) != ROWS*COLS):
            print ('--- packet error: length mismatch ---')
            continue
                            
        pixels = np.array (pixels)
        print ("min : {} , max : {}".format (min(pixels), max(pixels)))
        pixels = pixels.reshape(ROWS, COLS)
        img.set_array(pixels) 

        # transfer using in-memory file:
        buf = io.BytesIO()
        plt.savefig (buf, format='png')
        buf.seek(0)
        rgb_frame = np.asarray (bytearray(buf.read()), dtype=np.uint8)
        #rgb_frame = cv2.imdecode (rgb_frame, cv2.IMREAD_COLOR)
        gray_frame = cv2.imdecode (rgb_frame, cv2.IMREAD_GRAYSCALE)  # ***
        buf.close()
        
        blurred = cv2.GaussianBlur(gray_frame, (BLUR_SIZE, BLUR_SIZE), 0)

        #T, the_mask = cv2.threshold(blurred, 0, 255, cv2.THRESH_OTSU) 
        #print ("Otsu's threshold: %d" % (T))
        #the_mask = cv2.adaptiveThreshold (blurred, 255, cv2.ADAPTIVE_THRESH_GAUSSIAN_C, cv2.THRESH_BINARY_INV, neighbourhood, cparam) 
        retval,the_mask = cv2.threshold (blurred, 40,255,  cv2.THRESH_BINARY) 

        #NOTE : the third argument MUST be explicitly named as 'mask'
        masked = cv2.bitwise_and(gray_frame, gray_frame, mask=the_mask)
        
        cv2.imshow("Thermal camera", gray_frame)
        cv2.imshow("Thresholded", the_mask) 
        #np.vstack([gray_frame, the_mask, masked]))
        #cv2.imshow("Thermal Camera", np.vstack([gray_frame, the_mask]))
        
        key = cv2.waitKey(SLEEP_TIME) & 0xFF  # TODO: revisit the delay
        if (key==27): 
            break       
    except Exception as e:        
        print (e)

#sleep (1.0)
cv2.destroyAllWindows()
print("Bye !")

#-----------------------------------------------------------------------------------------------------

'''
# Save your thermal camera settings into a pickle file.
import pickle

cv_settings = { 
    "sensor_type" : "MLX-90621",
    "rows" :  4, 
    "cols" :  16,
    "packet_size" : 512, 
    "sensor_ip": "192.168.0.109", 
    "sensor_port": 12345, 
    "interpolation": 2,
    "palette" :  10, 
    "min_temp" : 28, 
    "max_temp" : 34, 
    "min_red" :  [0, 120, 150], 
    "max_red" :  [10, 255, 255],    
    "sleep_time" : 50
    }

pickle.dump(cv_settings , open("settings_90621.p", "wb" ))

print ("Config saved as 'settings_90621.p'")
#-----------------------------------------------------------------------------------------------------
 
// MLX90621_Arduino_Processing code : MLX90621_Udp2.ino
#include <stdlib.h>
#include <Arduino.h>
#include "MLX90621.h"
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Timer.h>

#define ssid      "myssid"  
#define passwd    "mypasswd" 

#define ROWS              4
#define COLS              16
#define DATA_LENGTH       64   // 4*16
#define IN_BUFFER_SIZE    32
#define OUT_BUFFER_SIZE   512  // ASSUMING 64*8 characters payload size
#define DATA_TIMEOUT      10   // x seconds*2 
#define LOWER_LIMIT       0    // sensor error
#define UPPER_LIMIT       100  // sensor error

Timer T;
WiFiClient wifi_client;
MLX90621 sensor; // create an instance of the Sensor class
WiFiUDP Udp;
unsigned int local_port = 12345;                  // local UDP port to listen for data requests
char request_packet[IN_BUFFER_SIZE];     
char response_packet[OUT_BUFFER_SIZE];      
IPAddress remote_IP = IPAddress(192,168,0,105);   // default values, but
unsigned int remote_port = 54321;                 // these will be overridden by requestor
boolean use_static_IP = true;
//int  FPS_param = 16;   // thermal cam 8 frames per second
int  FPS_param = 4;   // thermal cam 2 frames per second
int  data_timer = DATA_TIMEOUT; // time out for no incoming data requests (x seconds * 2)
bool data_timed_out = true;
bool verbose = true;
// NOTE: do not use pins D1 and D2, as they are meant for I2C
int  sensor_enable_pin = 12;  // D6 
int  led1 = 16;  // D0
 
void setup(){ 
    Serial.begin(115200);     
    pinMode (led1, OUTPUT);
    blinker();
    pinMode(sensor_enable_pin, INPUT);
    digitalWrite(sensor_enable_pin, HIGH);
    sensor.initialise (FPS_param);  
    init_wifi(); 
    Udp.begin(local_port);
    Serial.printf("Listening at IP %s, UDP port %d ...\n", WiFi.localIP().toString().c_str(), local_port);
    Serial.printf("Default remote IP %s, UDP port %d ...\n", remote_IP.toString().c_str(), remote_port);
    T.every(2000, data_watchdog); // 2x seconds
}

void loop(){
    T.update();   
    if (WiFi.status() != WL_CONNECTED)
        init_wifi();
    check_for_updates();  // TODO: enable OTA
    send_data();
}

// data will be sent when a request is received or suo motu after a time out
void send_data() {    
    sensor.measure(); // collect pixel data from thermal sensor
    int packetSize = Udp.parsePacket();
    if (packetSize==0)   { // no incoming request
        if (!data_timed_out) 
          return;
    }  else {  // received a request on UDP
      //Serial.printf("Received %d bytes from %s, port %d\n", packetSize, Udp.remoteIP().toString().c_str(), Udp.remotePort());
      remote_IP = Udp.remoteIP();
      remote_port = Udp.remotePort();  
      int len = Udp.read(request_packet, IN_BUFFER_SIZE);
      if (verbose && len > 0) {
          request_packet[len] = 0; // null terminate
          Serial.printf("packet request: %s\n", request_packet);
        }
    }
    // send back a reply, to the IP address and port of the requestor
    char tmp[12];     
    double data;
    response_packet[0] = '[';   // start a new packet
    response_packet[1] = '\0';    
    
    for(int y=0; y<ROWS; y++) {       // y = 4 rows
        for(int x=0; x<COLS; x++) {   // x = 16 columns
            data = sensor.getTemperature(y+x*4); // read the temperature at position (x,y)
            if (isnan(data) || data < LOWER_LIMIT || data > UPPER_LIMIT) {
                reset_sensor();
                return;
            }
            // dtostrf(FLOAT, WIDTH, PRECSISION, BUFFER)
            dtostrf(data,1,2,tmp); 
            strcat(response_packet, tmp);
            strcat(response_packet, " ");
        }
    }
    strcat(response_packet, "]");
    Udp.beginPacket(remote_IP, remote_port);
    Udp.write(response_packet);
    Udp.endPacket();
    data_timed_out = false;
    data_timer = DATA_TIMEOUT;   // keep reloading the hour glass
}

void data_watchdog() {
    data_timer = data_timer-2;  // 2 seconds timer
    if (data_timer <= 0) {
        data_timed_out = true;
        Serial.println("Data watchdog timed out.");
    }
}

void reset_sensor() {
  Serial.println("-------------->>>> Sensor error ! resetting...");
  digitalWrite(sensor_enable_pin, LOW);
  delay(2000);
  digitalWrite(sensor_enable_pin, HIGH);
  delay(500);  
  sensor.initialise (FPS_param);
  delay(500);    
}

void init_wifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, passwd);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi Connected.");
  if (use_static_IP) {
      IPAddress ip(192,168,0,109);
      IPAddress gateway(192,168,0,1);
      IPAddress subnet(255,255,255,0);
      WiFi.config(ip,gateway,subnet);
      Serial.println("WiFi Configured.");
  }
  Serial.println("IP address: "); 
  Serial.println(WiFi.localIP());
}

void check_for_updates(){}
void blinker() {}
#-----------------------------------------------------------------------------------------------------
'''


              