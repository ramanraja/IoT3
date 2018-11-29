from time import sleep
import SerialReceiver as seri  

PORT = 'COM12' 
sensor = seri.SerialReceiver()
# NOTE: timeout is the third param, so it should be explicitly named
if (not sensor.open(PORT, timeout=3.0)): 
     print ('Serial port error')
     exit(1)
          
sensor.start()

data_ptr = None            
print ('Press ^C to exit...')
while True:
    try:
        sensor.requestData()
        data_ptr = sensor.getData()
        if (len(data_ptr) == 0):
            print(" --->>> MLX sensor timed out.")
            continue
        # now the data is in the sensor's internal buffer, referenced by data_ptr
        # *** This works because this is a request-response protocol ***
        print (len(data_ptr))
    except KeyboardInterrupt:
        break        
    except Exception as e:        
        print (e)

sensor.close()
sleep (5.0)
print("Main thread exits.")  

