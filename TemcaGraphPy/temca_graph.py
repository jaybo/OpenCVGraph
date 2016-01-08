"""
Python wrapper for functionality exposed in the TemcaGraph dll.

@author: jayb

"""
from ctypes import *
import logging
import threading
import time
import os
import numpy as np
from numpy.ctypeslib import ndpointer

if __debug__:
    rel = "../x64/Debug/TemcaGraphDLL.dll"
else:
    rel = "../x64/Release/TemcaGraphDLL.dll"

dll_path = os.path.join(os.path.dirname(__file__), rel)

class StatusCallbackInfo(Structure):
    _fields_ = [
        ("status", c_int), 
        # -1 : fatal error
        # 0: finishied init (startup), 
        # 1: starting new frame, 
        # 2: finished frame capture  (ie. time to move the stage), 
        # 3: finished frame processing and file writing, 
        # 4: finished fini (shutdown)
        ("error_code", c_int),
        ("error_string", c_char * 256)
        ]

STATUSCALLBACKFUNC = CFUNCTYPE(c_int, POINTER(StatusCallbackInfo))  # returns c_int

class CameraInfo(Structure):
    _fields_ = [
        ("width", c_int),
        ("height", c_int),
        ("format", c_int),
        ("pixel_depth", c_int),
        ("camera_bpp", c_int),
        ("camera_model", c_char * 256),
        ("camera_id", c_char * 256)
        ]

class FocusInfo(Structure):
    _fields_ = [
        ("score", c_float),
        ("astigmatism", c_float),
        ("angle", c_float),
        ]

class ROIInfo(Structure):
    _fields_ = [
        ("gridX", c_int),
        ("gridY", c_int),
        ]

class TemcaGraphDLL(object):
    """
    dll setup.  These are all the foreign functions we are going to be using
        from the dll, along with their arguments types and return values.
    """
    _TemcaGraphDLL = WinDLL(dll_path)

    init = _TemcaGraphDLL.init
    init.argtypes = [c_char_p, STATUSCALLBACKFUNC]
    init.restype = c_uint32

    fini = _TemcaGraphDLL.fini

    camera_info = _TemcaGraphDLL.getCameraInfo
    camera_info.restype = CameraInfo

    grab_frame = _TemcaGraphDLL.grabFrame
    grab_frame.argtypes = [c_char_p, c_int, c_int]
    grab_frame.restype = None
    
    get_last_frame = _TemcaGraphDLL.getLastFrame
    get_last_frame.argtypes = [ndpointer(c_uint16, flags="C_CONTIGUOUS")]
    get_last_frame.restype = None

    get_status = _TemcaGraphDLL.getStatus
    get_status.restype = StatusCallbackInfo

    setRoiInfo = _TemcaGraphDLL.setROI
    setRoiInfo .restype = None
    setRoiInfo .argtypes = [ POINTER( ROIInfo) ]

class TemcaGraph(object):
    '''
    Wrapper for the C++ TemcaGraphDLL
    '''
    def __init__(self,):
        '''
        Most class variables are created in the init function
        '''
        self.aborting = False
        self.eventInitCompleted = threading.Event()
        self.eventStartNewFrame = threading.Event()
        self.eventCaptureCompleted = threading.Event()
        self.eventProcessingCompleted = threading.Event()
        self.eventFiniCompleted = threading.Event()

    def open(self,  graphType='temca', callback=None):
        ''' 
        graphType: 'temca', 'dummy', 'delay'
        '''
        if callback == None:
            callback = self.statusCallback
        # prevent the callback from being garbage collected !!!
        self.callback = STATUSCALLBACKFUNC(callback)

        t = time.clock()
        if not TemcaGraphDLL.init(graphType, self.callback):
            raise EnvironmentError('Cannot create graphType: ' + graphType + '. Other possiblities: camera, is offline, not installed, or already in use')
        logging.info("TemcaGraph DLL initialized in %s seconds" % (time.clock() - t))

    def close(self):
        ''' 
        Close down all graphs.
        '''
        TemcaGraphDLL.fini()

    def get_camera_info(self):
        ''' 
        Fills CameraInfo structure with details of the capture format including width, height, and bytes per pixel.
        '''
        ci = TemcaGraphDLL.camera_info()
        #self.frame_width = ci.width
        #self.frame_height = ci.height
        return ci

    def get_status(self):
        return TemcaGraphDLL.get_status()

    def grab_frame(self, filename = "none", roiX = 0, roiY = 0):
        ''' Trigger capture of a frame. '''
        TemcaGraphDLL.grab_frame(filename, roiX, roiY)

    def get_last_frame(self, img):
        ''' 
        Get a copy of the last frame captured as an ndarray.  
        This must be called only after eventCaptureCompleted has signaled.
        '''
        #assert (img.shape() == (3840, 3840) && im.type() == uint16])
        TemcaGraphDLL.get_last_frame(img)
        pass

    def set_roi_info (self, roiInfo):
        TemcaGraphDLL.setRoiInfo (roiInfo)

    def statusCallback (self, statusInfo):
        ''' Called by the c++ Temca graph runner whenever status changes:
            status values:
               -1: fatal error
                0: finishied init (startup)
                1: starting new frame
                2: finished frame capture  (ie. time to move the stage)
                3: finished frame processing and file writing
                4: finished fini (shutdown)
                ...
        '''
        status = statusInfo.contents.status
        error = statusInfo.contents.error_code
        logging.info ('callback status: ' + str(status) + ', error: ' + str(error))
        tid = threading.currentThread()
        if (status == -1):
            self.aborting = True
            error_string = statusInfo.contents.error_string
            logging.error ('callback error is' + error_string)
            return False
        elif status == 0:
            # finished initialization of all graphs
            self.eventInitCompleted.set()
        elif status == 1:
            # ready to start the next frame
            self.eventProcessingCompleted.clear()
            self.eventStartNewFrame.set()
        elif status == 2:
            # capture frame is now available
            # (move the stage now)
            self.eventCaptureCompleted.set()
        elif status == 3:
            # all processing for the frame is complete
            self.eventCaptureCompleted.clear()
            self.eventStartNewFrame.clear()
            self.eventProcessingCompleted.set()
        elif status == 4:
            # graph is finished all processing. Close app.
            self.eventFiniCompleted.set()
        return True
    

if __name__ == '__main__':
     
    import cv2
    import numpy as np


    logging.basicConfig(level=logging.INFO,
                format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')

    # Open the DLL which runs all TEMCA graphs
    temcaGraph = TemcaGraph()

    # Create all graphs ('dummy' means use fake camera)
    #temcaGraph.open('temca')
    temcaGraph.open('dummy')
    #temcaGraph.open('delay')

    # get info about frame dimensions
    fi = temcaGraph.get_camera_info()
    w = fi.width
    h = fi.height
    pixel_depth = fi.pixel_depth
    camera_id = fi.camera_id

    img = np.zeros(shape=(w,h), dtype= np.uint16)

    waitTime = 3.0

    # wait for graph to complete initialization
    temcaGraph.eventInitCompleted.wait(waitTime)

    # set ROI grid size (for stitching only)
    roiInfo = ROIInfo()
    roiInfo.gridX = 4
    roiInfo.gridY = 4
    temcaGraph.set_roi_info (roiInfo)

    frameCounter = 0

    for y in range(roiInfo.gridY):
        for x in range (roiInfo.gridX):
            if temcaGraph.aborting:
                break
            temcaGraph.eventStartNewFrame.wait(waitTime)
            temcaGraph.grab_frame('j:/junk/pyframe' + str(frameCounter) + '.tif', x, y)
            temcaGraph.eventCaptureCompleted.wait(waitTime)

            # move stage here
            temcaGraph.eventProcessingCompleted.wait(waitTime)

            # get a copy of the frame?
            # temcaGraph.get_last_frame(img)

            frameCounter += 1

    temcaGraph.close()
    temcaGraph.eventFiniCompleted.wait(waitTime)

 
