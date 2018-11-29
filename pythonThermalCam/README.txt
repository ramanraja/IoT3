Refactored and organized MLX90640 python code.
A single unified code base for all transports and detection logics. 


thermalCam35.py:
Derived from thermalCam34.py. 
Reads MLX90640 either through 
    UDP chunks through 8266 
    OR serial communication 
    OR I2C on Raspberry Pi
through a simple config switch.
Can do either HSV or gray scale detection by another config param. (HSV code from thermalCam32.py)
A single unified configuration file across all transports and detection logics.  
Use it with: 
    MLX90640_Udp4.ino/MLX90640_Udp3.ino  
    OR  MLX90640_Serial5.ino 
    OR  the RPi library MLX90640

