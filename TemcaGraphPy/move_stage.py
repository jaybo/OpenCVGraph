from pytemca.stage.stage import RadishStage
import msvcrt
import time
import os,sys

stage = RadishStage("*", 8000, 8001, "http://10.128.26.122:8090")

stage._set_pos_2d(0.0,0.0)

while True:
    if msvcrt.kbhit():
        print msvcrt.getch()
    else:
        print 'bar'
    time.sleep(0.5)
