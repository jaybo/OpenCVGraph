"""
Python wrapper for functionality exposed in the TemcaGraph dll.

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

    stage = RadishStage("*", 8000, 8001, "http://10.128.26.122:8090")
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

    sx, sy = stage._get_pos_2d()
    cx = sx
    cy = sy
    dx = dy = .002  # in mm 
    angle = 120

    #temcaGraph.optimize_exposure()
    #temcaGraph.set_mode('preview')
    #for mode in ['temca', 'preview', 'raw']:
    temcaGraph.set_mode('temca')
    
    while True:
        # get the template
        temcaGraph.wait_start_of_frame()
        temcaGraph.grab_frame('j:/junk/junk.tif', 0,0) # filename doesn't matter in preview
        temcaGraph.wait_graph_event(temcaGraph.eventAsyncProcessingCompleted)
        if msvcrt.kbhit() and (msvcrt.getch() == ' '):
            break  

    stage._set_pos_2d(sx+dx, sy)

    while True:
        # measure the offset
        temcaGraph.wait_start_of_frame()
        temcaGraph.grab_frame('j:/junk/junk.tif', 0,0) # filename doesn't matter in preview
        temcaGraph.wait_graph_event(temcaGraph.eventAsyncProcessingCompleted)
        if msvcrt.kbhit() and (msvcrt.getch() == ' '):
            break  

    stage._set_pos_2d(sx, sy)
    temcaGraph.close()
    temcaGraph.wait_graph_event(temcaGraph.eventFiniCompleted)
    



 
