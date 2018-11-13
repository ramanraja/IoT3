# Save your thermal camera settings into a pickle file.
# This is for the serial port version of the program

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



'''---------------------------------------------------------------------
# Keeping a backup copy for reference: do not modify the entries below:
------------------------------------------------------------------------
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
----------------------------------------------------------------------'''    