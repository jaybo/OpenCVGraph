"""
Python wrapper for functionality exposed in the TemcaGraph dll.

@author: jayb

"""
from ctypes import *
import logging
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
        ("status", c_int), # // 0: init complete, 1: grab complete (move stage), 2: graph complete, -1: error, see error_string
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

class TemcaGraphDLL(object):
    """
    dll setup.  These are all the foreign functions we are going to be using
        from the dll, along with their arguments types and return values.
    """
    _TemcaGraphDLL = WinDLL(dll_path)

    init = _TemcaGraphDLL.init
    init.argtypes = [c_char_p]
    init.restype = c_uint32

    fini = _TemcaGraphDLL.fini

    register_notify_callback = _TemcaGraphDLL.RegisterNotifyCallback
    register_notify_callback.argtypes = [STATUSCALLBACKFUNC]
    register_notify_callback.restype = None

    grab_frame = _TemcaGraphDLL.grabFrame
    grab_frame.argtypes = [c_char_p]
    grab_frame.restype = c_uint32

    frame_info = _TemcaGraphDLL.getFrameInfo
    frame_info.restype = FrameInfo

    get_status = _TemcaGraphDLL.getStatus
    get_status.restype = StatusCallbackInfo


class TemcaGraph(object):
    """

    Args:

    """
    def __init__(self, graphType='default'):
        t = time.clock()
        if not TemcaGraphDLL.init(graphType):
            raise EnvironmentError ('Cannot create graphType: ' + graphType + '. Other possiblities: camera, is offline, not installed, or already in use')
        logging.info("TemcaGraph DLL initialized in %s seconds" % (time.clock()-t))

    def fini(self):
        TemcaGraphDLL.fini()

    def get_frame_info(self):
        ''' fills FrameInfo structure with details of the capture format including width, height, and bytes per pixel
        '''
        return TemcaGraphDLL.frame_info()

    def get_status(self):
        return TemcaGraphDLL.get_status()

    def grab_frame(self, filename = "none"):
        TemcaGraphDLL.grab_frame(filename)

    def get_last_image(self):
        pass

    def statusCallback (self, statusInfo):
        ''' Called by the c++ Temca graph runner whenever status changes:
            status values:
                0: finishied init (startup)
                1: finished frame capture  (ie. time to move the stage)
                2: finished frame processing and file writing
                3: finished fini (shutdown)
            error values:
                0: no error
                ...
        '''
        status = statusInfo.contents.status
        error = statusInfo.contents.error_code
        logging.info ('callback status: ' + str(status) + ', error: ' + str(error))
        if error != 0:
            error_string = statusInfo.contents.error_string
            logging.error ('callback error is' + error_string)
        return True
    
    def registerNotifyCallback(self, callback):
        #prevent garbage collection of callback func
        self.callback = STATUSCALLBACKFUNC(callback)
        TemcaGraphDLL.register_notify_callback(self.callback)

def foo(statusInfo):
    return True

if __name__ == '__main__':
     
    import cv2
    import numpy as np
    
    logging.basicConfig(level=logging.INFO,
                format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')

    temcaGraph = TemcaGraph()
    temcaGraph.registerNotifyCallback(temcaGraph.statusCallback)

    fi = temcaGraph.get_frame_info()
    w = fi.width
    h = fi.height
    camera_id = fi.camera_id

    #stat = temcaGraph.get_status()
    #status = stat.status
    #error_string = stat.error_string

    im = np.zeros((w, h), dtype=np.uint32);

    frameCounter = 0

    for f in range(5):
        temcaGraph.grab_frame()
        time.sleep(0.5)

    temcaGraph.fini()

    #while True:
        #t = time.clock()
        #sapera.queue_frame()
        #frame = sapera.grab_frame()

        #frameCounter += 1

        ##frame = frame[
        #fmin = np.min(frame)
        #fmax = np.max(frame)
        #print((time.clock()-t)*1000, fmin, fmax)
        #resized = cv2.resize(frame, (frame.shape[0] / 2, frame.shape[1] / 2), interpolation = cv2.INTER_NEAREST)
        #scaled = resized * 4 # from 14 to 16 bits
        #cv2.imshow("TEST", scaled)
        #k = cv2.waitKey(1)
        #if k == 27:
        #    break
  
