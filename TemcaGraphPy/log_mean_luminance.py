"""
Measure luminance change over time.

@author: jayb

"""
from ctypes import *
import logging
import threading
import time
import os
import sys
import yaml
import subprocess
import numpy as np
import cv2
from temca_graph import *
import msvcrt
from pytemca.stage.stage import RadishStage, DummyRadishStage

if __name__ == '__main__':

    logging.basicConfig(level=logging.INFO,
                format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    #while True:
    #    print(ord(msvcrt.getch()))r

    def process_keys(stage, delta):
        if msvcrt.kbhit():
            k = ord(msvcrt.getch())
            if k == 224:
                x, y = stage.get_pos()
                k = ord(msvcrt.getch())
                if k == 72:
                    # up arrow
                    stage.set_pos(x, y - delta)
                elif k == 80:
                    # down arrow
                    stage.set_pos(x, y + delta)
                elif k == 77:
                    # right arrow
                    stage.set_pos(x + delta, y)
                elif k == 75:
                    # left arrow
                    stage.set_pos(x - delta, y)
                return None
            return k
        return None

    stage = RadishStage("*", 8000, 8001, "http://10.128.26.51:8090")
    #stage = DummyRadishStage("*", 8000, 8001, "http://localhost:8090")

    # Open the DLL which runs all TEMCA graphs
    temcaGraph = TemcaGraph() 
    temcaGraph.open(dummyCamera = False) 

    showRawImage = False
    showPreviewImage = False

    if showRawImage or showPreviewImage:
        import numpy as np
        if showRawImage:
            imgRaw = temcaGraph.allocate_frame()
        if showPreviewImage:
            imgPreview = temcaGraph.allocate_preview_frame()  # 8bpp and decimated

    # wait for graph to complete initialization
    temcaGraph.eventInitCompleted.wait(temcaGraph.wait_time)

    sx, sy = stage.get_pos()
    cx = sx
    cy = sy
    dx = dy = .003  # in mm 
    angle = 120

    #temcaGraph.optimize_exposure()
    #temcaGraph.set_mode('preview')
    #for mode in ['temca', 'preview', 'raw']:
    temcaGraph.set_mode('temca')
    frame_counter = 0
    
    while True:
        # get the template
        temcaGraph.wait_start_of_frame()
        temcaGraph.grab_frame('j:/junk/junk.tif', 0,0) # filename doesn't matter in preview
        temcaGraph.wait_graph_event(temcaGraph.eventAsyncProcessingCompleted)
        qcInfo = temcaGraph.get_qc_info()
        if frame_counter % 10 == 0:
            logging.info ('mean: ' + str(qcInfo['mean']) + ', min: ' + str(qcInfo['min']) + ', max: ' + str(qcInfo['max']))
        k = process_keys(stage, dx)
        frame_counter += 1
        if k == ord(' '):
            break  

    stage.set_pos(sx, sy)
    temcaGraph.close()
    temcaGraph.wait_graph_event(temcaGraph.eventFiniCompleted)
    

