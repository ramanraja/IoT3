# Save your thermal camera settings into a pickle file.
# This is for the serial port version of the program
# python settings_90640_UG.py

import pickle

cv_settings = { 
    "sensor_type" : "MLX-90640",     # 24x32 thermal camera
    "rows" :  24, 
    "cols" :  32,
    "sleep_time" : 50,               # mSec between packet requests    

    "sensor_ip" : "192.168.0.123",
    "sensor_port" : 12345,
    "udp_timeout" : 3.0,
    
    "interpolation": 2,              # 2 = bicubic
    "palette" :  10,                 # 10 = gray scale
    "min_temp" : 28, 
    "max_temp" : 34, 
           
    "blur_size" : 21,                
    "neighbourhood" : 21,            # must be odd
    "cparam" : 3,
    
    "running_buffer_length" : 5,     # moving average buffer
    "num_erode_dilate" : 5,          # number of iterations
    "lower_threshold" : 220,         # gray scale thresholds
    "upper_threshold" : 255,
    "alpha" : 1.5,                   # contrast multiplier
    "beta" : 1.5                     # brightness adjustment
    }
    
pickle.dump(cv_settings , open("settings_90640_UG.p", "wb"))
print ("Config saved as 'settings_90640_UG.p'")


'''---------------------------------------------------------------------
# Keeping a backup copy for reference: do not modify the entries below:
------------------------------------------------------------------------
cv_settings = { 
    "sensor_type" : "MLX-90640",
    "rows" :  24, 
    "cols" :  32,
    "interpolation": 2,
    "palette" :  10, 
    "min_temp" : 28, 
    "max_temp" : 34, 
    "min_red" :  [0, 100, 100], 
    "max_red" :  [10, 255, 255],    
    "blur_size" : 11,                
    "neighbourhood" : 11,            # must be odd
    "cparam" : 3,                    # for adaptive threhsolding
    "running_buffer_length" : 10,    # moving average buffer
    "num_erode_dilate" : 3,          # number of iterations
    "alpha" : 2.0,                   # contrast multiplier
    "beta" : 0.0,                    # brightness adjustment    
    "sleep_time" : 50
    }
----------------------------------------------------------------------'''    