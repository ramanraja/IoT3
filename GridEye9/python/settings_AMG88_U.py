# python settings_AMG88_U.py

import pickle

cv_settings = { 
    "sensor_type" : "AMG-88xx",
    "rows" :  8, 
    "cols" :  8,
    "packet_size" : 512, 
    "sensor_ip": "192.168.0.109", 
    "sensor_port": 12345, 
    "interpolation": 2,
    "palette" :  1, 
    "min_temp" : 28, 
    "max_temp" : 34, 
    "min_red" :  [0, 120, 150], 
    "max_red" :  [10, 255, 255],    
    "sleep_time" : 50
    }
    
pickle.dump(cv_settings , open("settings_AMG88_U.p", "wb"))
print ("Config saved as 'settings_AMG88_U.p'")

'''------------------------------------------------------------------------
backup copy - do not edit this section
---------------------------------------------------------------------------
cv_settings = { 
    "sensor_type" : "AMG-88xx",
    "rows" :  8, 
    "cols" :  8,
    "packet_size" : 512, 
    "sensor_ip": "192.168.0.109", 
    "sensor_port": 12345, 
    "interpolation": 2,
    "palette" :  1, 
    "min_temp" : 28, 
    "max_temp" : 34, 
    "min_red" :  [0, 120, 150], 
    "max_red" :  [10, 255, 255],    
    "sleep_time" : 50
    }
-----------------------------------------------------------------------''' 
