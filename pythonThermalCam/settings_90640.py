# Save your thermal camera settings into a pickle file.
# This can weork with serial/UDP/I2C port version of the program
# python settings_90640.py

import pickle

cv_settings = { 
    "sensor_type" : "MLX-90640",     # 24x32 thermal camera
    "rows" :  24, 
    "cols" :  32,
    "sleep_time" : 50,               # mSec between packet requests    
    "comm_type" : 2,                 # 1=UDP, 2=serial, 3=I2C
    
    "serial_port" : "COM12",
    "udp_ip" : "192.168.0.123",
    "udp_port" : 12345,
    "server_timeout" : 2.0,          # I2C frames per second 
    "fps" : 2,
    
    "interpolation": 2,              # 2 = bicubic
    "palette" :  1,                  # 10 = gray scale
    "min_temp" : 28, 
    "max_temp" : 34, 
    "use_gray_scale" : 0,            # 0 = use HSV, 1 = use grayscale

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


'''---------------------------------------------------------------------
# Keeping a backup copy for reference: do not modify the entries below:
------------------------------------------------------------------------
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
----------------------------------------------------------------------'''    