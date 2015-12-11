"""
Python wrapper for functionality exposed in the OpenCVGraph dll.

@author: jayb

"""
from ctypes import *
import logging
import time
import os
import numpy as np

#module = os.path.dirname(__file__)
#rel = "c_lib/pysapera.dll"

#dll_path = r"C:/dev/pytemca/pytemca/camera/c_lib/pysapera.dll"
dll_path = r"C:/dev/OpenCVGraph/x64/Debug/OpenCVGraphDLL.dll" # os.path.join(module, rel)

class OpenCVGraphDLL(object):
    """
    dll setup.  These are all the foreign functions we are going to be using
        from the dll, along with their arguments types and return values.
    """
    _OpenCVGraphDLL = WinDLL(dll_path)

    init_system = _OpenCVGraphDLL.initSystem

    #load_config_files = _OpenCVGraphDLL.loadConfigFiles

    #queue_frame = _OpenCVGraphDLL.queueFrame
    #queue_frame.restype = c_uint32

    #grab_frame = _OpenCVGraphDLL.grabFrame

    #acquire_images = _OpenCVGraphDLL.acquireImages

    #get_parameter = _OpenCVGraphDLL.getParameter
    #get_parameter.argtypes = (c_int32,)
    #get_parameter.restype = c_int32

    #get_width = _OpenCVGraphDLL.getWidth
    #get_width.restype = c_uint32

    #get_height = _OpenCVGraphDLL.getHeight
    #get_height.restype = c_uint32

    #get_format = _OpenCVGraphDLL.getFormat
    #get_format.restype = c_uint32

    #get_buffer_type = _OpenCVGraphDLL.getBufferType
    #get_buffer_type.restype = c_uint32

    #get_buffer_data_depth = _OpenCVGraphDLL.getBufferDataDepth
    #get_buffer_data_depth.restype = c_uint32

    #get_buffer_pixel_depth = _OpenCVGraphDLL.getBufferPixelDepth
    #get_buffer_pixel_depth.restype = c_uint32

    #set_parameter = _OpenCVGraphDLL.setParameter
    #set_parameter.argtypes = (c_int32, c_int32)
    #set_parameter.restype = c_int32

    #disconnect_sapera = _OpenCVGraphDLL.disconnectSapera

    #free_sapera = _OpenCVGraphDLL.freeSapera

    #create_buffer = _OpenCVGraphDLL.createBuffer

    #free_buffer = _OpenCVGraphDLL.freeArray

    #close_sapera = _OpenCVGraphDLL.closeSapera

    #get_status_text = _OpenCVGraphDLL.getStatusText
    #get_status_text.argtypes = (c_uint32,)
    #get_status_text.restype = c_char_p


class OpenCVGraph(object):
    """

    Args:

    """
    def __init__(self):
        t = time.clock()
        if not OpenCVGraphDLL.init_system():
            raise EnvironmentError ('Cannot access the camera, its either offline, not installed, or already in use')

        logging.info("OpenCVGraph DLL initialized in %s seconds" % (time.clock()-t))

        
if __name__ == '__main__':
     
    import cv2
    import numpy as np
    
    foo = cv2.VideoWriter_fourcc('M','J','P','G')
    print foo
    logging.basicConfig(level=logging.INFO,
                format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')

    openCVGraph = OpenCVGraph()

    frameCounter = 0

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
  
