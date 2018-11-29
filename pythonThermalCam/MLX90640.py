# MLX90640 library simulator
# for original python library:
# pip install MLX90640
# https://github.com/pimoroni/mlx90640-library/tree/master/python

import numpy as np
from time import sleep
from random import randint

ROWS = 24
COLS = 32


def setup(fps):
    print ("Dummy Setup for MLX90640")
    
    
def get_frame():
    sleep(1.9)
    return np.array([randint(25,35) for i in range(ROWS*COLS)])    
   
    
def cleanup():
    print ("Dummy cleanup for MLX90640")