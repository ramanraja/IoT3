# Save your thermal camera settings into a pickle file.
# This is for the serial port version of the program
# python settings_90640_S.py

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
    "blur_size" : 11,                
    "neighbourhood" : 11,            # must be odd
    "cparam" : 3,                    # for adaptive threhsolding
    "running_buffer_length" : 10,    # moving average buffer
    "num_erode_dilate" : 3,          # number of iterations
    "alpha" : 3.0,                   # contrast multiplier
    "beta" : -0.5,                    # brightness adjustment    
    "sleep_time" : 50
    }
    
pickle.dump(cv_settings , open("settings_90640_S.p", "wb"))
print ("Config saved as 'settings_90640_S.p'")


'''---------------------------------------------------------------------
# Keeping a backup copy for reference: do not modify the entries below:
------------------------------------------------------------------------
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