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

class CallbackInfo(Structure):
    _fields_ = [
        ("status", c_int), # // 0: init complete, 1: grab complete (move stage), 2: graph complete, -1: error, see error_string
        ("error_code", c_int),
        ("error_string", c_char * 256)
        ]

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

    #register_callback = _TemcaGraphDLL.RegisterNotifyCallback

    grab_frame = _TemcaGraphDLL.grabFrame
    grab_frame.argtypes = [c_char_p]
    grab_frame.restype = c_uint32

    frame_info = _TemcaGraphDLL.getFrameInfo
    frame_info.restype = FrameInfo

    get_status = _TemcaGraphDLL.getStatus
    get_status.restype = CallbackInfo


    #queue_frame = _TemcaGraphDLL.queueFrame
    #queue_frame.restype = c_uint32


    #acquire_images = _TemcaGraphDLL.acquireImages

    #get_parameter = _TemcaGraphDLL.getParameter
    #get_parameter.argtypes = (c_int32,)
    #get_parameter.restype = c_int32

    #set_parameter = _TemcaGraphDLL.setParameter
    #set_parameter.argtypes = (c_int32, c_int32)
    #set_parameter.restype = c_int32

    #get_status_text = _TemcaGraphDLL.getStatusText
    #get_status_text.argtypes = (c_uint32,)
    #get_status_text.restype = c_char_p



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
        ''' fills FrameInfo structure with details of the capture format including width and height
        '''
        return TemcaGraphDLL.frame_info()

    def get_status(self):
        return TemcaGraphDLL.get_status()

    def grab_frame(self, filename = "none"):
        TemcaGraphDLL.grab_frame(filename)

    def get_last_image(self):
        pass

    def statusCallback (status, stringResults):
        pass
    



    #STATUSCALLBACKFUNC = ctypes.CFUNCTYPE(c_int, POINTER(c_char_p)) 

    #def getCallbackFunc(self):
    #    def func(status, stringResults):
    #        self.statusCallback(status, stringResults)
    #    #prevent garbage collection of func
    #    self.callback = func 
    #    return STATUSCALLBACKFUNC(func)

    #def registerNotifyCallback(self):
    #    TemcaGraphDLL.RegisterNotifyCallback(self.getCallbackFunc())


        #self.frame_width = self.get_width()
        #self.frame_height = self.get_height()
        #self.pixel_format = self.get_format()
        #self.pixel_depth = self.get_pixel_depth()

        #if self.pixel_depth == 16:
        #    self.data_ctype = c_uint16
        #    self.data_numpy_type = np.uint16
        #elif self.pixel_depth == 8:
        #    self.data_ctype = c_uint8
        #    self.data_numpy_type = np.uint8
        #else:
        #    raise TypeError("Pixel depth should be either 16 or 8. Got %s instead." % self.pixel_depth)





if __name__ == '__main__':
     
    import cv2
    import numpy as np
    
    logging.basicConfig(level=logging.INFO,
                format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')

    temcaGraph = TemcaGraph()
   
    fi = temcaGraph.get_frame_info()
    w = fi.width
    h = fi.height
    camera_id = fi.camera_id

    stat = temcaGraph.get_status()
    status = stat.status
    error_string = stat.error_string

    im = np.zeros((w, h), dtype=np.uint32);

    frameCounter = 0

    for f in range(20):
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
  
