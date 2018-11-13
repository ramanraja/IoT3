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
    "palette" :  1, 
    "min_temp" : 28, 
    "max_temp" : 34, 
    "min_red" :  [0, 120, 150], 
    "max_red" :  [10, 255, 255],    
    "sleep_time" : 50,
    "autocal_interval" : 10
    }

pickle.dump(cv_settings , open("settings_90621.p", "wb" ))

print ("Config saved as 'settings_90621.p'")
 
'''--------------------------------------
backup copy - do not edit this section
-----------------------------------------
cv_settings = { 
    "sensor_type" : "MLX-90621",
    "rows" :  4, 
    "cols" :  16,
    "packet_size" : 512, 
    "sensor_ip": "192.168.0.109", 
    "sensor_port": 12345, 
    "interpolation": 2,
    "palette" :  1, 
    "min_temp" : 28, 
    "max_temp" : 34, 
    "min_red" :  [0, 120, 150],  ### try also [0, 100, 100],
    "max_red" :  [10, 255, 255],    
    "sleep_time" : 50,
    "autocal_interval" : 100
    }
---------------------------------------''' 