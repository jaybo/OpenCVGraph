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

    grab_frame = _TemcaGraphDLL.grabFrame
    grab_frame.argtypes = [c_char_p]
    grab_frame.restype = c_uint32

    get_width = _TemcaGraphDLL.getWidth
    get_width.restype = c_uint32

    get_height = _TemcaGraphDLL.getHeight
    get_height.restype = c_uint32

    get_format = _TemcaGraphDLL.getFormat
    get_format.restype = c_uint32

    get_pixel_depth = _TemcaGraphDLL.getPixelDepth
    get_pixel_depth.restype = c_uint32

    STATUSCALLBACKFUNC = CFUNCTYPE(c_int, POINTER(c_char_p)) 

    #queue_frame = _TemcaGraphDLL.queueFrame
    #queue_frame.restype = c_uint32


    #acquire_images = _TemcaGraphDLL.acquireImages

    #get_parameter = _TemcaGraphDLL.getParameter
    #get_parameter.argtypes = (c_int32,)
    #get_parameter.restype = c_int32

    #get_width = _TemcaGraphDLL.getWidth
    #get_width.restype = c_uint32

    #get_height = _TemcaGraphDLL.getHeight
    #get_height.restype = c_uint32

    #get_format = _TemcaGraphDLL.getFormat
    #get_format.restype = c_uint32

    #get_buffer_type = _TemcaGraphDLL.getBufferType
    #get_buffer_type.restype = c_uint32

    #get_buffer_data_depth = _TemcaGraphDLL.getBufferDataDepth
    #get_buffer_data_depth.restype = c_uint32

    #get_buffer_pixel_depth = _TemcaGraphDLL.getBufferPixelDepth
    #get_buffer_pixel_depth.restype = c_uint32

    #set_parameter = _TemcaGraphDLL.setParameter
    #set_parameter.argtypes = (c_int32, c_int32)
    #set_parameter.restype = c_int32

    #disconnect_sapera = _TemcaGraphDLL.disconnectSapera

    #free_sapera = _TemcaGraphDLL.freeSapera

    #create_buffer = _TemcaGraphDLL.createBuffer

    #free_buffer = _TemcaGraphDLL.freeArray

    #close_sapera = _TemcaGraphDLL.closeSapera

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
            raise EnvironmentError ('Cannot access the camera, its either offline, not installed, or already in use')
        logging.info("TemcaGraph DLL initialized in %s seconds" % (time.clock()-t))

    def fini(self):
        TemcaGraphDLL.fini()

    def grab_frame(self, filename = "none"):
        TemcaGraphDLL.grab_frame(filename)

    def get_width(self):
        return TemcaGraphDLL.get_width()
        
    def get_height(self):
        return TemcaGraphDLL.get_height()

    def get_format(self):
        return TemcaGraphDLL.get_format()

    def get_pixel_depth(self):
        return TemcaGraphDLL.get_pixel_depth()

    def get_last_image(self):
        pass

    def statusCallback (status, stringResults):
        pass

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
    w = temcaGraph.get_width()
    h = temcaGraph.get_height()
    fmt = temcaGraph.get_format()
    pix_depth = temcaGraph.get_pixel_depth()

    im = np.zeros((w, h), dtype=np.uint32);

    frameCounter = 0

    for f in range(20):
        time.sleep(0.5)
        temcaGraph.grab_frame()

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
  
