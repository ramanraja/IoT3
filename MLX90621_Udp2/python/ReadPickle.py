# Load the thermal camera settings back from the pickle file
# This is just to test the file; include the following code into your program

import pickle

cv_settings = pickle.load(open( "settings_90621.p", "rb" ))
print (cv_settings)
print()
print (cv_settings['sensor_type'])
print (cv_settings['rows'])
print (cv_settings['cols'])
print (cv_settings['packet_size'])
print (cv_settings['sensor_ip'])
print (cv_settings['sensor_port'])
print (cv_settings['interpolation'])
print (cv_settings['palette'])
print (cv_settings['min_temp'])
print (cv_settings['max_temp'])
print (cv_settings['min_red'])
print (type(cv_settings['min_red']))
print (cv_settings['max_red'])
print (type(cv_settings['max_red']))
print (cv_settings['sleep_time'])

 