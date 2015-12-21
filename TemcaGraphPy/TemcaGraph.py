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

if __debug__:
    rel = "../x64/Debug/TemcaGraphDLL.dll"
else:
    rel = "../x64/Release/TemcaGraphDLL.dll"

dll_path = os.path.join(os.path.dirname(__file__), rel)

class StatusCallbackInfo(Structure):
    _fields_ = [
        ("status", c_int), 
        # 0: finishied init (startup), 
        # 1: starting new frame, 
        # 2: finished frame capture  (ie. time to move the stage), 
        # 3: finished frame processing and file writing, 
        # 4: finished fini (shutdown)
        ("error_code", c_int),
        ("error_string", c_char * 256)
        ]

STATUSCALLBACKFUNC = CFUNCTYPE(c_int, POINTER(StatusCallbackInfo))  # returns c_int

class FrameInfo(Structure):
    _fields_ = [
        ("width", c_int),
        ("height", c_int),
        ("format", c_int),
        ("pixel_depth", c_int),
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

    frame_info = _TemcaGraphDLL.getFrameInfo
    frame_info.restype = FrameInfo

    grab_frame = _TemcaGraphDLL.grabFrame
    grab_frame.argtypes = [c_char_p, c_int, c_int]
    grab_frame.restype = None
    
    #get_last_frame = _TemcaGraphDLL.getLastFrame
    #get_last_frame.argtypes = [POINTER (c_byte)]
    #get_last_frame.restype = None

    get_status = _TemcaGraphDLL.getStatus
    get_status.restype = StatusCallbackInfo

    setRoiInfo = _TemcaGraphDLL.setROI
    setRoiInfo .restype = None
    setRoiInfo .argtypes = [ POINTER( ROIInfo) ]

class TemcaGraph(object):
    """
    Wrapper for the C++ TemcaGraphDLL
    """
    def __init__(self, graphType='default', callback = None):
        ''' graphType: 'default', 'dummy'
        '''
        if callback == None:
            callback = self.statusCallback
        # prevent the callback from being garbage collected
        self.callback = STATUSCALLBACKFUNC(callback)

        t = time.clock()
        self.eventInitCompleted = threading.Event()
        self.eventStartNewFrame = threading.Event()
        self.eventCaptureCompleted = threading.Event()
        self.eventProcessingCompleted = threading.Event()
        self.eventFiniCompleted = threading.Event()

        if not TemcaGraphDLL.init(graphType, self.callback):
            raise EnvironmentError ('Cannot create graphType: ' + graphType + '. Other possiblities: camera, is offline, not installed, or already in use')

        logging.info("TemcaGraph DLL initialized in %s seconds" % (time.clock()-t))

    def fini(self):
        ''' Closing down all graphs
        '''
        TemcaGraphDLL.fini()

    def get_frame_info(self):
        ''' fills FrameInfo structure with details of the capture format including width, height, and bytes per pixel
        '''
        return TemcaGraphDLL.frame_info()

    def get_status(self):
        return TemcaGraphDLL.get_status()

    def grab_frame(self, filename = "none", roiX = 0, roiY = 0):
        ''' Trigger capture of a frame.
        '''
        TemcaGraphDLL.grab_frame(filename, roiX, roiY)

    def get_last_frame(self):
        ''' get a copy of the last frame captured.  This must be called only after eventCaptureCompleted is signaled.
        '''
        pass

    def set_roi_info (self, roiInfo):
        TemcaGraphDLL.setRoiInfo (roiInfo)

    def statusCallback (self, statusInfo):
        ''' Called by the c++ Temca graph runner whenever status changes:
            status values:
                0: finishied init (startup)
                1: starting new frame
                2: finished frame capture  (ie. time to move the stage)
                3: finished frame processing and file writing
                4: finished fini (shutdown)
            error values:
                0: no error
                ...
        '''
        status = statusInfo.contents.status
        error = statusInfo.contents.error_code
        logging.info ('callback status: ' + str(status) + ', error: ' + str(error))
        tid = threading.currentThread()

        if status == 0:
            self.eventInitCompleted.set()
        elif status == 1:
            self.eventProcessingCompleted.clear()
            self.eventStartNewFrame.set()
        elif status == 2:
            self.eventCaptureCompleted.set()
        elif status == 3:
            self.eventCaptureCompleted.clear()
            self.eventStartNewFrame.clear()
            self.eventProcessingCompleted.set()
        elif status == 4:
            self.eventFiniCompleted.set()
        if error != 0:
            error_string = statusInfo.contents.error_string
            logging.error ('callback error is' + error_string)

        return True
    

if __name__ == '__main__':
     
    import cv2
    import numpy as np
    
    logging.basicConfig(level=logging.INFO,
                format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')

    temcaGraph = TemcaGraph()
    #temcaGraph = TemcaGraph('dummy')

    # get info about frame dimensions
    fi = temcaGraph.get_frame_info()
    w = fi.width
    h = fi.height
    camera_id = fi.camera_id



    waitTime = None

    # wait for graph to complete initialization
    temcaGraph.eventInitCompleted.wait(waitTime)

    # set ROI grid size (for stitching only)
    roiInfo = ROIInfo()
    roiInfo.gridX = 2
    roiInfo.gridY = 2
    temcaGraph.set_roi_info (roiInfo)

    frameCounter = 0

    for y in range(roiInfo.gridY):
        for x in range (roiInfo.gridX):
            temcaGraph.eventStartNewFrame.wait(waitTime)
            temcaGraph.grab_frame('j:/junk/frame' + str(frameCounter) + '.tif', x, y)
            temcaGraph.eventCaptureCompleted.wait(waitTime)

            # move stage here
            temcaGraph.eventProcessingCompleted.wait(waitTime)
            frameCounter += 1

    temcaGraph.fini()
    temcaGraph.eventFiniCompleted.wait(waitTime)

 
