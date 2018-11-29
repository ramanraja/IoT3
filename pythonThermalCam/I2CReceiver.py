# MLX90640 library wrapper

# for original python library:
# pip install MLX90640
# https://github.com/pimoroni/mlx90640-library/tree/master/python

 
import MLX90640  as mlx

class I2CReceiver():
       
    def open(self, fps):
        mlx.setup(fps)
        return True
    
    def start(self):
        print ("MLX sensor starts..")
        
    def requestData(self):
        pass
    
    def getData(self):    
        return (mlx.get_frame())
        
    def close(self):
        mlx.cleanup()
        print ("MLX sensor closed.")        
 