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
import numpy as np
from pytemca.image.imageproc import fit_sin
from numpy.ctypeslib import ndpointer

if sys.flags.debug:
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
        # 3: Sync step completed 
        # 4: Async step completed
        # 5: Processing finished (except Async graphs)
        # 6: Shutdown finished
        ("info_code", c_int),
        # value indicates which sync or async step completed
        ("error_string", c_char * 256)
        ]

STATUSCALLBACKFUNC = CFUNCTYPE(c_int, POINTER(StatusCallbackInfo))  # returns c_int

class CameraInfo(Structure):
    '''
    Information about the current camera in use.
    '''
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
    '''
    Information about focus quality.
    '''
    _fields_ = [
        ("focus_score", c_float),
        ("astig_score", c_float),
        ("astig_angle", c_float),
        ("astig_profile", c_float * 360)
        ]

class QCInfo(Structure):
    '''
    Information about image quality.
    '''
    _fields_ = [
        ("min_value", c_int),
        ("max_value", c_int),
        ("mean_value", c_int),
        ("histogram", c_int * 256),
        ]

class ROIInfo(Structure):
    '''
    Information about the selected ROI used for stitching.
    '''
    _fields_ = [
        ("gridX", c_int),
        ("gridY", c_int),
        ]

class MatcherInfo(Structure):
    '''
    Match parameters from the Matcher.
    '''
    _fields_ = [
        ("dX", c_float),
        ("dY", c_float),
        ("distance", c_float),
        ("rotation", c_float),
        ("good_matches", c_int),
        ]


class TemcaGraphDLL(object):
    """
    Hooks onto the C++ DLL.  These are all the foreign functions we are going to be using
        from the dll, along with their arguments types and return values.
    """
    _TemcaGraphDLL = WinDLL(dll_path)

    open = _TemcaGraphDLL.temca_open
    open.argtypes = [c_int, c_char_p, STATUSCALLBACKFUNC]
    open.restype = c_uint32

    close = _TemcaGraphDLL.temca_close
    close.argtype = [None]
    close.restype = c_uint32

    set_mode = _TemcaGraphDLL.setMode
    set_mode.argtypes = [c_char_p]
    set_mode.restype = c_uint32

    get_camera_info = _TemcaGraphDLL.getCameraInfo
    get_camera_info.restype = CameraInfo

    get_focus_info = _TemcaGraphDLL.getFocusInfo
    get_focus_info.restype = FocusInfo

    set_fft_size = _TemcaGraphDLL.setFFTSize
    set_fft_size.argtypes = [c_int, c_int, c_int]
    set_fft_size.restype = None

    get_qc_info = _TemcaGraphDLL.getQCInfo
    get_qc_info.restype = QCInfo

    grab_frame = _TemcaGraphDLL.grabFrame
    grab_frame.argtypes = [c_char_p, c_int, c_int]
    grab_frame.restype = None
    
    get_last_frame = _TemcaGraphDLL.getLastFrame
    get_last_frame.argtypes = [ndpointer(c_uint16, flags="C_CONTIGUOUS")]
    get_last_frame.restype = None

    get_preview_frame = _TemcaGraphDLL.getPreviewFrame
    get_preview_frame.argtypes = [ndpointer(c_uint8, flags="C_CONTIGUOUS")]
    get_preview_frame.restype = None

    set_parameter = _TemcaGraphDLL.setParameter
    set_parameter.argtypes = [c_char_p, c_int]
    set_parameter.restype = None

    get_parameter = _TemcaGraphDLL.getParameter
    get_parameter.argtypes = [c_char_p]
    get_parameter.restype = c_uint32

    get_status = _TemcaGraphDLL.getStatus
    get_status.restype = StatusCallbackInfo

    setRoiInfo = _TemcaGraphDLL.setROI
    setRoiInfo.restype = None
    setRoiInfo.argtypes = [ POINTER( ROIInfo) ]

    grab_matcher_template = _TemcaGraphDLL.grabMatcherTemplate
    grab_matcher_template.restype = None
    grab_matcher_template.argtypes = [c_int, c_int, c_int, c_int]

    get_matcher_info = _TemcaGraphDLL.getMatcherInfo
    get_matcher_info.restype = MatcherInfo
    get_matcher_info.argtypes = None

class TemcaGraph(object):
    '''
    Python class which wraps the C++ TemcaGraphDLL and provides the linkage between Python and the C++ OpenCVGraph world.
    The Python events which are triggered by C++ callbacks are::

        eventInitCompleted - all graphs have finished building
        eventStartNewFrame - ready for client to issue a frame grab request
        eventCaptureCompleted - exposure completed
        eventCapturePostProcessingCompleted - xfer to CUDA, upshift, Bright/Dark correction finished
        eventSyncProcessingCompleted - Synchronous processing has finished
        eventAsyncProcessingCompleted - Asynchronous processing has finished (may overlap next exposure)
        eventFiniCompleted - graph has finished shutting down

    '''
    def __init__(self,):
        '''
        Many additional class variables are defined in the open() function
        '''
        self.aborting = False
        self.eventInitCompleted = threading.Event() # Event signalling that initialization is complete.
        self.eventStartNewFrame = threading.Event()
        self.eventCaptureCompleted = threading.Event()
        self.eventCapturePostProcessingCompleted = threading.Event()
        self.eventSyncProcessingCompleted = threading.Event()
        self.eventAsyncProcessingCompleted = threading.Event()
        self.eventFiniCompleted = threading.Event()
        # all events after eventStartNewFrame, and before eventFiniCompleted
        self.eventsAllCaptureLoop = [self.eventCaptureCompleted, 
                                     self.eventCapturePostProcessingCompleted,
                                     self.eventSyncProcessingCompleted,
                                     self.eventAsyncProcessingCompleted]

        self.threadLock = threading.Lock()
        self.preview_decimation_factor = 4
        self.wait_time = 10    # in seconds.  If we reach this limit, its an error

    def wait_graph_event (self, event):
        '''
        Waits for the specified event to signal indicating a change in the graph state,
        and then clears the event.
        '''
        self.threadLock.acquire()
        event.wait(self.wait_time)
        event.clear()
        self.threadLock.release()

    def wait_all_capture_events(self):
        for e in self.eventsAllCaptureLoop:
            self.wait_graph_event(e)

    def wait_start_of_frame(self):
        '''
        Wait for the event which indicates the graph is ready to start a new frame.
        '''
        self.wait_graph_event(self.eventStartNewFrame)

    def open(self, dummyCamera = False, dummyPath = None, callback=None):
        ''' 
        Open up the Temca C++ DLL.
        If dummyCamera is True, create a dummy TEMCA image source using...
        either a real camera, image, directory, or movie according to dummyPath which MUST be specified
        as no default path is provided.  If dummyPath is an integer string, then an OpenCV camera will be used
        corresponding to that index.
        '''
        if callback == None:
            callback = self.statusCallback
        # prevent the callback from being garbage collected !!!
        self.callback = STATUSCALLBACKFUNC(callback)

        self.dummyPath = dummyPath

        t = time.clock()
        if not TemcaGraphDLL.open(dummyCamera, self.dummyPath, self.callback):
            raise EnvironmentError('Cannot open TemcaGraphDLL. Possiblities: camera, is offline, not installed, or already in use')
        logging.info("TemcaGraph DLL initialized in %s seconds" % (time.clock() - t))

        self.eventInitCompleted.wait()

        # get info about frame dimensions
        fi = self.get_camera_info()
        self.image_width = fi['width']
        self.image_height = fi['height']
        self.pixel_depth = fi['pixel_depth']    # 16 ALWAYS
        self.camera_bpp = fi['camera_bpp']      # 12 for Ximea (upshift to full 16 bpp)
        self.camera_model = fi['camera_model']
        self.camera_id = fi['camera_id']

        # if this is changed dynamically, reallocate preview frames
        self.set_parameter('preview_decimation_factor', self.preview_decimation_factor)

    def close(self):
        ''' 
        Close down all graphs.
        '''
        TemcaGraphDLL.close()

    def set_mode(self, graphType):
        '''
        Sets the overall mode of operation for the Temca graph.  
        Each mode activates a subset of the overall graph.::

            graphType                         SYNC        ASYNC
            -----------------------------------------------------
            temca           : ximea, postCap, QC          Stitch
                                              Focus
                                              FileWriter 
            raw             : ximea, postCap, FileWriter
            preview         : ximea, postCap, QC
                                              Focus 

        '''
        return TemcaGraphDLL.set_mode(graphType)

    def set_parameter(self, parameter, value):
        '''
        General purpose way to set random parameters on the graph. 
        'value' must be an int.  Valid parameters are::

            'exposure' for Ximea, this is in microseconds
            'gain' for Ximea, this is in dB * 1000 
            'preview_decimation_factor' (2, 4, 8, ...)

        '''
        TemcaGraphDLL.set_parameter(parameter, value)

    def get_parameter(self, parameter):
        '''
        General purpose way to get random parameters on the graph.
        Return value is an int.  Valid parameters are given under set_parameter.
        '''
        return TemcaGraphDLL.get_parameter(parameter)
    
    def get_camera_info(self):
        ''' 
        Returns a dictionary with details of the capture format including width, height, bytes per pixel, and the camera model and serial number.
        '''
        info = TemcaGraphDLL.get_camera_info()
        return {'width' : info.width, 'height' : info.height, 
                'pixel_depth' : info.pixel_depth, 'camera_bpp' : info.camera_bpp, 
                'camera_model' : info.camera_model, 'camera_id' : info.camera_id}
    
    def get_focus_info(self):
        ''' returns focus and astigmatism values, some calculated in CUDA, some in python '''

        info = TemcaGraphDLL.get_focus_info()
        astig_amp, astig_angle, offset, wave = fit_sin(info.astig_profile)
        astig_score = astig_amp/np.ptp(info.astig_profile)

        array_type = c_float*len(info.astig_profile)
        astig_profile_pointer = cast(info.astig_profile, POINTER(array_type))
        astig_numpy = np.frombuffer(astig_profile_pointer.contents, dtype=np.float32)

        # return the profile?
        return {'focus_score': info.focus_score, 'astig_score': astig_score, 'astig_angle' : astig_angle,
                'astig_profile' : astig_numpy,}

    def set_fft_size(self, dimension, start_freq, end_freq):
        ''' Set the dimension of the FFT (which must be a power of 2) and the start and end frequency for focus/astig measurement.
            Both start and end frequencies must be less than dimension.
        '''
        TemcaGraphDLL.set_fft_size(dimension, start_freq, end_freq);

        
    def get_qc_info(self):
        ''' Get the min, max, mean, and histogram from the last image acquired. '''
        info = TemcaGraphDLL.get_qc_info()

        array_type = c_int*len(info.histogram)
        hist_profile_pointer = cast(info.histogram, POINTER(array_type))
        hist_numpy = np.frombuffer(hist_profile_pointer.contents, dtype=np.int32)

        return {'min':info.min_value, 'max': info.max_value, 'mean':info.mean_value, 'histogram':hist_numpy}

    def grab_matcher_template(self, x, y, width, height):
        ''' Set the ROI to use as the template on the next image acquired. ''' 
        TemcaGraphDLL.grab_matcher_template(x, y, width, height)

    def get_matcher_info(self):
        ''' Return Match status from the matcher.  If "good_matches" is 0, then the match operation failed'''
        info = TemcaGraphDLL.get_matcher_info()
        return {'dX': info.dX, 'dY': info.dY, 'distance': info.distance, 'rotation': info.rotation, 'good_matches': info.good_matches} 

    def get_status(self):
        return TemcaGraphDLL.get_status()

    def grab_frame(self, filename = "none", roiX = 0, roiY = 0):
        ''' 
        Trigger capture of a frame.  This function does not wait for completion of anything.
        '''
        TemcaGraphDLL.grab_frame(filename, roiX, roiY)

    def grab_frame_wait_completion(self, filename = "none", roiX = 0, roiY = 0):
        ''' 
        Trigger capture of a frame.  This function waits for completion of all graphs.
        '''
        self.wait_start_of_frame()
        self.grab_frame(filename, roiX, roiY) # filename doesn't matter in preview, nor does roi
        self.wait_all_capture_events()

    def allocate_frame(self):
        '''
        Allocate memory as a numpy array to hold a complete frame (16bpp grayscale).
        '''
        return np.zeros(shape=(self.image_width,self.image_height), dtype= np.uint16)

    def allocate_preview_frame(self):
        '''
        Allocate memory as a numpy array to hold a preview frame (8bpp grayscale).
        '''
        return np.zeros(shape=(self.image_width/self.preview_decimation_factor,self.image_height/self.preview_decimation_factor), dtype= np.uint8)

    def get_last_frame(self, img):
        ''' 
        Get a copy of the last frame captured as an ndarray (16bpp grayscale).  
        This must be called only after eventCapturePostProcessingCompleted has signaled and before the next frame is acquired.
        '''
        assert (img.shape == (self.image_width, self.image_height) and (img.dtype.type == np.uint16))
        TemcaGraphDLL.get_last_frame(img)

    def get_preview_frame(self, img):
        ''' 
        Get a copy of the preview image as an ndarray (8bpp grayscale).  
        This must be called only after eventCapturePostProcessingCompleted has signaled and before the next frame is acquired.
        '''
        assert (img.shape == (self.image_width/self.preview_decimation_factor, self.image_height/self.preview_decimation_factor) and (img.dtype.type == np.uint8))
        TemcaGraphDLL.get_preview_frame(img)

    def optimize_exposure(self):
        '''
        Search for optimal exposure value using binary search.
        '''
        min_high_value = 61000
        max_high_value = 63000
        exposure_step = 100000 #uS

        self.set_mode('preview')
        exp = self.get_parameter('exposure')

        def _searchDirection():
            ''' return 0 = just right, 1 go up, -1 go down '''
            self.grab_frame_wait_completion()
            info = self.get_qc_info()
            m = info['max']
            if m > min_high_value and m < max_high_value:
                return 0 # just right
            elif m >= max_high_value:
                return +1 # too high
            else:
                return -1 # too low

        #overshoot top end
        dir = _searchDirection()
        while dir < 0:
            exp = exp + exposure_step
            self.set_parameter('exposure', exp)
            dir = _searchDirection()
        if dir == 0:
            return;
        exp_top = exp

        #overshoot bottom end
        while dir > 0:
            exp = exp - exposure_step
            self.set_parameter('exposure', exp)
            dir = _searchDirection()
        if dir == 0:
            return;
        exp_bottom = exp
        
        # binary search, starting from bottom
        exposure_step = exp_top - exp_bottom
        while dir != 0 and exposure_step >= 2:
            exposure_step = exposure_step / 2
            if dir < 0:
                exp += exposure_step
            else:
                exp -= exposure_step
            self.set_parameter('exposure', exp)
            dir = _searchDirection()

    
    def set_roi_info (self, roiInfo):
        '''
        Set the dimensions of the ROI.  This information is used for stitching.
        '''
        TemcaGraphDLL.setRoiInfo (roiInfo)

    def statusCallback (self, statusInfo):
        ''' 
        Called by the C++ Temca graph runner whenever status changes.
        These values correspond to the Python events activated. ::

            -1 : fatal error
             0: finished init (startup)
             1: starting new frame
             2: finished frame capture  (ie. time to move the stage)
             3: capture post processing finished (preview ready)
             4: Sync step completed 
             5: Async step completed
             6: Shutdown finished

        '''
        retValue = True
        status = statusInfo.contents.status
        info = statusInfo.contents.info_code
        #logging.info ('callback status: ' + str(status) + ', info: ' + str(info))
        tid = threading.currentThread()
        if (status == -1):
            self.aborting = True
            error_string = statusInfo.contents.error_string
            logging.info ('callback error is' + error_string)
            retValue = False
        elif status == 0:
            # finished initialization of all graphs
            self.eventInitCompleted.set()
        elif status == 1:
            # ready to start the next frame (start of the loop)
            self.eventStartNewFrame.set()
        elif status == 2:
            # capture completed
            # (move the stage now)
            self.eventCaptureCompleted.set()
        elif status == 3:
            # post processing finished (*16, bright dark, spatial correction, preview ready)
            self.eventCapturePostProcessingCompleted.set()
        elif status == 4:
            # all synchronous processing for the frame is complete
            self.eventSyncProcessingCompleted.set()
        elif status == 5:
            # all asynchronous processing for the frame is complete
            self.eventAsyncProcessingCompleted.set()
        elif status == 6:
            # graph is finished all processing. Close app.
            self.eventFiniCompleted.set()
        return retValue


if __name__ == '__main__':
    
    import cv2

    logging.basicConfig(level=logging.INFO,
                format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')

    # Open the DLL which runs all TEMCA graphs
    
    #os.environ["PATH"] += os.pathsep
    
    temcaGraph = TemcaGraph() 
    temcaGraph.open(dummyCamera = True) 

    showRawImage = True
    showPreviewImage = True

    if showRawImage or showPreviewImage:
        import numpy as np
        if showRawImage:
            imgRaw = temcaGraph.allocate_frame()
        if showPreviewImage:
            imgPreview = temcaGraph.allocate_preview_frame()  # 8bpp and decimated

    # wait for graph to complete initialization
    temcaGraph.eventInitCompleted.wait(temcaGraph.wait_time)

    #temcaGraph.optimize_exposure()
    temcaGraph.set_mode('preview')

    #for j in range(10):
    #    temcaGraph.grab_frame_wait_completion()
    #    sys.stdout.write('.')
    #    info = temcaGraph.get_qc_info()

    #for mode in ['temca', 'preview', 'raw']:
    #for mode in ['temca']:
    for mode in ['preview']:
        print
        print mode
        temcaGraph.set_mode(mode)
        frameCounter = 0

        # set ROI grid size (for stitching only)
        roiInfo = ROIInfo()
        roiInfo.gridX = 5
        roiInfo.gridY = 5
        temcaGraph.set_roi_info (roiInfo)

        for y in range(roiInfo.gridY):
            for x in range (roiInfo.gridX):
                if temcaGraph.aborting:
                    break

                temcaGraph.wait_start_of_frame()
                temcaGraph.grab_frame('j:/junk/pyframe' + str(frameCounter) + '.tif', x, y) # filename doesn't matter in preview
                sys.stdout.write('.')
                temcaGraph.wait_graph_event(temcaGraph.eventCaptureCompleted)

                # move the stage here

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

                qcInfo = temcaGraph.get_qc_info()
                #histogram = qcInfo['histogram']
                focusInfo = temcaGraph.get_focus_info()
                #print qcInfo

                frameCounter += 1

    
    temcaGraph.close()
    temcaGraph.wait_graph_event(temcaGraph.eventFiniCompleted)
    



 
