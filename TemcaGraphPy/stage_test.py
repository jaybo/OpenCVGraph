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
from temca_graph import *
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
    dx = dy = .01  # in mm 
    angle = 120

    #temcaGraph.optimize_exposure()
    #temcaGraph.set_mode('preview')

    #for mode in ['temca', 'preview', 'raw']:
    for mode in ['temca']:
        print
        print mode
        temcaGraph.set_mode(mode)
        frameCounter = 0

        # set ROI grid size (for stitching only)
        roiInfo = ROIInfo()
        roiInfo.gridX = 10
        roiInfo.gridY = 10
        temcaGraph.set_roi_info (roiInfo)

        for y in range(roiInfo.gridY):
            if y != 0:
                cy += dy
            for x in range (roiInfo.gridX):
                if x == 0:
                    cx = sx
                else:
                    cx += dx
                if temcaGraph.aborting:
                    break

                temcaGraph.wait_start_of_frame()
                temcaGraph.grab_frame('j:/RoiCapture/frame' + '-' + str(x) + '-' + str(y) + '.tif', x, y) # filename doesn't matter in preview
                sys.stdout.write('.')
                temcaGraph.wait_graph_event(temcaGraph.eventCaptureCompleted)

                # move the stage here
                stage._set_pos_2d(cx, cy)
                
                # wait for Async ready event (stitching complete for previous frame)
                if frameCounter > 0:
                    temcaGraph.wait_graph_event(temcaGraph.eventAsyncProcessingCompleted)

                # wait for preview ready event
                temcaGraph.wait_graph_event(temcaGraph.eventCapturePostProcessingCompleted)

                # get a copy of the frame and display it?
                if showRawImage:
                    temcaGraph.get_last_frame(imgRaw)
                    cv2.imshow('imgRaw', imgRaw)
                    cv2.waitKey(1);

                # get a copy of the preview and display it?
                if showPreviewImage:
                    temcaGraph.get_preview_frame(imgPreview)
                    cv2.imshow('imgPreview', imgPreview)
                    cv2.waitKey(1);

                # wait for Sync ready event (QC and Focus complete)
                temcaGraph.wait_graph_event(temcaGraph.eventSyncProcessingCompleted)

                #qcInfo = temcaGraph.get_qc_info()
                #histogram = qcInfo['histogram']
                #focusInfo = temcaGraph.get_focus_info()

                frameCounter += 1


    stage._set_pos_2d(sx, sy)
    temcaGraph.close()
    temcaGraph.wait_graph_event(temcaGraph.eventFiniCompleted)
    



 
