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
import math
import numpy as np
from temca_graph import *
from pytemca.stage.stage import RadishStage, DummyRadishStage

if __name__ == '__main__':

    # user changable parameters
    nm_per_pix = 3.6363636363636363636363636363636
    rotation_angle = -180 + 96.3    # degrees
    overlap = 0.50         # overlap on EACH side (0.1 = 10% on each side)
    grid_x = 3            # size of grid to create
    grid_y = 3
    base_path = r'j:\temcaRaw'
    # end user changable parameters

    logging.basicConfig(level=logging.INFO,
                format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')

    timestr = time.strftime("%Y%m%d-%H%M%S") # 20120515-155045
    if not os.path.exists(base_path):
        os.mkdir(base_path)
    image_dir = os.path.join(base_path, timestr)
    if not os.path.exists(image_dir):
        os.mkdir(image_dir)
    meta_file = open(os.path.join(base_path, timestr, 'meta.txt'), 'w')

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

    def rotatePoint(centerPoint,point,angle):
        """Rotates a point around another centerPoint. Angle is in degrees.
        Rotation is counter-clockwise"""
        angle = math.radians(angle)
        temp_point = point[0] - centerPoint[0] , point[1] - centerPoint[1]
        temp_point = (temp_point[0] * math.cos(angle) - temp_point[1] * math.sin(angle) , temp_point[0] * math.sin(angle) + temp_point[1] * math.cos(angle))
        temp_point = temp_point[0] + centerPoint[0] , temp_point[1] + centerPoint[1]
        return temp_point

    sx, sy = stage._get_pos_2d()
    cx = sx
    cy = sy
        
    w = temcaGraph.image_width
    h = temcaGraph.image_height
    fov_nm = nm_per_pix * w
    step_nm = fov_nm * (1 - overlap)
    x_axis_unit_vector = rotatePoint([0,0], [1, 0], -rotation_angle)
    y_axis_unit_vector = rotatePoint([0,0], [0, 1], -rotation_angle)
    #dx = dy = .01  # in mm
    dx_x_move = x_axis_unit_vector[0] * fov_nm * (1 - overlap) / 1e6
    dy_x_move = x_axis_unit_vector[1] * fov_nm * (1 - overlap) / 1e6
    dx_y_move = y_axis_unit_vector[0] * fov_nm * (1 - overlap) / 1e6
    dy_y_move = y_axis_unit_vector[1] * fov_nm * (1 - overlap) / 1e6

    step_pix_x = w * (1 - overlap)
    step_pix_y = h * (1 - overlap)
    pix_x = 0
    pix_y = 0

    #temcaGraph.optimize_exposure()
    stage.set_pos(0,0)
    time.sleep(0.5)
    
    temcaGraph.set_mode('temca')
    frameCounter = 0

    # set ROI grid size (for stitching only)
    roiInfo = ROIInfo()
    roiInfo.gridX = grid_x
    roiInfo.gridY = grid_y
    temcaGraph.set_roi_info(roiInfo)

    moves = []
    for y in range(roiInfo.gridY):
        if y != 0:
            pix_y += step_pix_y
        for x in range(roiInfo.gridX):
            if x == 0:
                pix_x = 0
            else:
                pix_x += step_pix_x

            image_file = 'frame' + '-' + str(y) + '-' + str(x) + '.tif'
            image_path = os.path.join(base_path, timestr, image_file)
            pX, pY = (x * dx_x_move + y * dx_y_move, y * dy_y_move + x * dy_x_move)
            print y, x, pY, pX

            move = (x, y, pX, pY, pix_x, pix_y, image_path, image_file)
            moves.append(move)

    move0 = moves[0]
    stage.set_pos(move0[2], move0[3])   

    for i, move in enumerate(moves):
        if temcaGraph.aborting:
            break

        x, y, pX, pY, pix_x, pix_y, image_path, image_file = move

        temcaGraph.wait_start_of_frame()
        temcaGraph.grab_frame(image_path, x, y) 
        meta_file.write(image_file + '\t' + str(pix_x) + '\t' + str(pix_y) + "\t0\n")
        sys.stdout.write('.')
        temcaGraph.wait_graph_event(temcaGraph.eventCaptureCompleted)
            
        # move the stage here
        #stage._set_pos_2d(cx, cy)
        #pX, pY = (x * dx_x_move + y * dx_y_move, y * dy_y_move + x * dy_x_move)
        #print y, x, pY, pX
        if i != grid_x * grid_y - 1:
            next_move = moves[i+1]
            pX_next, pY_next = (next_move[2], next_move[3])
            stage.set_pos(pX_next, pY_next)
        time.sleep(0.5)

        # wait for Async ready event (stitching complete for previous
        # frame)
        if frameCounter > 0:
            temcaGraph.wait_graph_event(temcaGraph.eventAsyncProcessingCompleted)

        # wait for preview ready event
        temcaGraph.wait_graph_event(temcaGraph.eventCapturePostProcessingCompleted)

        # get a copy of the frame and display it?
        if showRawImage:
            temcaGraph.get_last_frame(imgRaw)
            cv2.imshow('imgRaw', imgRaw)
            cv2.waitKey(1)

        # get a copy of the preview and display it?
        if showPreviewImage:
            temcaGraph.get_preview_frame(imgPreview)
            cv2.imshow('imgPreview', imgPreview)
            cv2.waitKey(1)

        # wait for Sync ready event (QC and Focus complete)
        temcaGraph.wait_graph_event(temcaGraph.eventSyncProcessingCompleted)

        #qcInfo = temcaGraph.get_qc_info()
        #histogram = qcInfo['histogram']
        #focusInfo = temcaGraph.get_focus_info()

        frameCounter += 1


    stage._set_pos_2d(sx, sy)
    temcaGraph.close()
    temcaGraph.wait_graph_event(temcaGraph.eventFiniCompleted)
    meta_file.close()
    



 
