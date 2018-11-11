# Save your thermal camera settings into a pickle file.
# This includes parameters for the gray scale version of thresholding

import pickle

cv_settings = { 
    "sensor_type" : "MLX-90621",     # 16x4 thermal camera
    "rows" :  4, 
    "cols" :  16,
    "packet_size" : 512,             # this is only for 16x4 camera
    "sensor_ip": "192.168.0.109", 
    "sensor_port": 12345, 
    "sleep_time" : 50,               # mSec between packet requests    
    
    "interpolation": 2,              # 2 = bicubic
    "palette" :  10,                 # 10 = gray scale
    "min_temp" : 28, 
    "max_temp" : 34, 
    
    "min_red" :  [0, 120, 150],      # for HSV separation only    
    "max_red" :  [10, 255, 255],  
        
    "blur_size" : 11,                
    "neighbourhood" : 11,            # must be odd
    "cparam" : 3,
    
    "running_buffer_length" : 10,    # moving average buffer
    "num_erode_dilate" : 3,          # number of iterations
    "lower_threshold" : 100,         # gray scale thresholds
    "upper_threshold" : 255
    }

pickle.dump(cv_settings , open("settings_90621_G.p", "wb" ))

print ("Config saved as 'settings_90621_G.p'")
 
'''------------------------------------------------------------------------
backup copy - do not edit this section
---------------------------------------------------------------------------
cv_settings = { 
    "sensor_type" : "MLX-90621",     # 16x4 thermal camera
    "rows" :  4, 
    "cols" :  16,
    "packet_size" : 512,             # this is only for 16x4 camera
    "sensor_ip": "192.168.0.109", 
    "sensor_port": 12345, 
    "sleep_time" : 50,               # mSec between packet requests    
    
    "interpolation": 2,              # 2 = bicubic
    "palette" :  10,                 # 10 = gray scale
    "min_temp" : 28, 
    "max_temp" : 34, 
    
    "min_red" :  [0, 120, 150],      # for HSV separation, ignore these
    ##"min_red" :  [0, 100, 100],      
    "max_red" :  [10, 255, 255],  
        
    "blur_size" : 11,
    "neighbourhood" : 11,            # must be odd
    "cparam" : 3,
    
    "running_buffer_length" : 10,    # moving average buffer
    "num_erode_dilate" : 3,          # number of iterations
    "lower_mask" : 100,              # gray scale thresholds
    "upper_mask" : 255
    }
-----------------------------------------------------------------------''' 