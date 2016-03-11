"""
Test approaches for optimizing focus.

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
import unittest
from temca_graph import *
import msvcrt
from pytemca.stage.stage import RadishStage, DummyRadishStage
from pytemca.stage.smarAct.smaract_stage import SmarActStage
from pytemca.scope.scope_device import ScopeDevice, ScopeConnection

# -------------------------------------------------------------
# helper functions
# -------------------------------------------------------------
def isWithin(minV, maxV, v):
    return v >= minV and v <= maxV

def frange(start, stop, step):
    i = start
    if step > 0:
        while i < stop:
            yield i
            i += step
    else:
        while i > stop:
            yield i
            i += step


def create_move_list (sx, sy, min_x, max_x, min_y, max_y, count):
    ''' create a list of X,Y moves of length count with random distances between min and max from sx, sy. '''
    dx = max_x - min_x
    dy = max_y - min_y
    r = np.random.rand(count, 2)
    r = (r - 0.5) * 2
    r[:,0] = (r[:,0] * dx) + sx + min_x
    r[:,1] = (r[:,1] * dy) + sy + min_y
    return r

# -------------------------------------------------------------
# TestSmarActStage tests
# -------------------------------------------------------------
class TestSmarActStage(unittest.TestCase):

    @classmethod                            
    def setUpClass(cls):
        cls.stage = SmarActStage()
        #self.stage = RadishStage("*", 8000, 8001, "http://10.128.26.51:8090")
        #self.stage = DummyRadishStage("*", 8000, 8001, "http://localhost:8090")

        cls.x_move_accuracy_limit = .000025 #mm
        cls.y_move_accuracy_limit = .000025 #mm
        cls.x_min = -21 #mm
        cls.x_max = 25.5 #mm
        cls.y_min = -2 #mm
        cls.y_max = 1.4 #mm

        if cls is TestSmarActStageWithCamera:
            # make the capture graph
            cls.temcaGraph = TemcaGraph()
            cls.temcaGraph.open(dummyCamera = False) 

            # wait for graph to complete initialization
            cls.temcaGraph.eventInitCompleted.wait(cls.temcaGraph.wait_time)
            cls.temcaGraph.set_mode('preview')

    @classmethod                            
    def tearDownClass(cls):
        if cls is TestSmarActStageWithCamera:
            # make the capture graph
            cls.temcaGraph.close()
            cls.temcaGraph.eventFiniCompleted.wait(cls.temcaGraph.wait_time)

    def setUp(self):
        #self.x_min, self.x_max =
        #self.stage.get_position_limit(self.stage.x_index)
        #self.y_min, self.y_max =
        #self.stage.get_position_limit(self.stage.y_index)
        self.startTime = time.time()

    def tearDown(self):
        t = time.time() - self.startTime
        print "%s: %.3f" % (self.id(), t)

    # -------------------------------------------------------------
    # tests
    # -------------------------------------------------------------
    def test_calibrate(self):
        ''' calibration must be called before FindReference '''
        self.stage.calibrate()
        while (self.stage.is_moving()):
            pass

    def test_reset(self):
        ''' test repositioning smarAct to physical 0,0 '''
        self.stage.reset()
        while (self.stage.is_moving()):
            pass
        pass

    def test_go_home(self):
        ''' go to (0, 0) established by Reference position '''
        self.stage.set_pos(0, 0)
        while (self.stage.is_moving()):
            pass
        x, y = self.stage.get_pos()
        self.assertTrue(isWithin(-self.x_move_accuracy_limit, self.x_move_accuracy_limit, x))
        self.assertTrue(isWithin(-self.x_move_accuracy_limit, self.x_move_accuracy_limit, y))

    def test_go_limits(self):
        ''' move entire +/- range for each sensor '''
        self.stage.set_pos(0, 0)
        while (self.stage.is_moving()):
            pass
        for xV in frange(self.x_min, self.x_max, 0.25): #mm
            self.stage.set_pos(xV, 0)
            while (self.stage.is_moving()):
                pass
            x, y = self.stage.get_pos()
        self.assertTrue(isWithin(xV - self.x_move_accuracy_limit, xV + self.x_move_accuracy_limit, x))

        for yV in frange(self.y_min, self.y_max, 0.25): #mm
            self.stage.set_pos(0, yV)
            while (self.stage.is_moving()):
                pass
            x, y = self.stage.get_pos()
        self.assertTrue(isWithin(yV - self.y_move_accuracy_limit, yV + self.y_move_accuracy_limit, y))

    def test_is_moving(self):
        ''' check behavior of is_moving status '''
        move_delta = 0.1

        logging.info ('\t+dx in mm\t time ')
        moves = frange(0, self.x_max, move_delta) #mm
        for p in moves:
            self.stage.set_pos(0, 0)
            while (self.stage.is_moving()):
                pass

            tStart = time.clock()
            self.stage.set_pos(p, 0)
            while (self.stage.is_moving()):
                pass
            dt = time.clock() - tStart
            logging.info ('\t{0}\t{1}'.format(str(p), str(dt)))
               
        logging.info ('\t-dx in mm\t time ')
        moves = frange(0, self.x_min, -move_delta) #mm
        for p in moves:
            self.stage.set_pos(0, 0)
            while (self.stage.is_moving()):
                pass

            tStart = time.clock()
            self.stage.set_pos(p, 0)
            while (self.stage.is_moving()):
                pass
            dt = time.clock() - tStart
            logging.info ('\t{0}\t{1}'.format(str(p), str(dt)))

        logging.info ('\t+dy in mm\t time ')
        moves = frange(0, self.y_max, move_delta) #mm
        for p in moves:
            self.stage.set_pos(0, 0)
            while (self.stage.is_moving()):
                pass

            tStart = time.clock()
            self.stage.set_pos(0,p)
            while (self.stage.is_moving()):
                pass
            dt = time.clock() - tStart
            logging.info ('\t{0}\t{1}'.format(str(p), str(dt)))
            
        logging.info ('\t-dy in mm\t time ')
        moves = frange(0, self.y_min, -move_delta) #mm
        for p in moves:
            self.stage.set_pos(0, 0)
            while (self.stage.is_moving()):
                pass

            tStart = time.clock()
            self.stage.set_pos(0,p)
            while (self.stage.is_moving()):
                pass
            dt = time.clock() - tStart
            logging.info ('\t{0}\t{1}'.format(str(p), str(dt)))
            
                         
class TestSmarActStageWithCamera(TestSmarActStage):

    def grab(self, filename):
        self.temcaGraph.wait_start_of_frame()
        self.temcaGraph.grab_frame(filename, 0,0) # filename doesn't matter in preview
        self.temcaGraph.wait_all_capture_events()

    def close(self):
        self.temcaGraph.close()
        self.temcaGraph.wait_graph_event(self.temcaGraph.eventFiniCompleted)

    def test_autoexposure(self):
        '''  blah '''
        self.temcaGraph.optimize_exposure()

    def test_focus(self):
        ''' from a starting point increment focus values and grab sample images '''
        
        self.temcaGraph.set_mode("temca")

        # fine mode: small step 32, big step 128
        # coarse mode: small step 128, big step 512
        focus_steps_plus_minus = 16  
        #focus_steps_plus_minus = 2  
        focus_step_size = 8

        self.scope = ScopeConnection(5)
        self.scope.beep()
        overall_max = [0, 0, 0, 0, ""] # max_val, fft_size, start_freq, focus_setting, filename
        fft_sizes = [512, 1024, 2048]
        dir_root = r'j:\junk\focus_test'

        for fft_size in fft_sizes:
            start_freqs = range (0, fft_size-1, fft_size / 10)
            freq_max = [0, 0, 0, 0, ""] # max_val, fft_size, start_freq, focus_setting, filename
            for freq in start_freqs:
                self.temcaGraph.set_fft_size(fft_size, freq, fft_size-1)
                directory = dir_root + '\\' + str(fft_size) + '_' + str(freq)
                try:
                    os.mkdir(directory)
                except OSError:
                    pass
                logging.info ('--------------------' + 'FFT_Size: ' + str(fft_size) + ' start_freq: ' + str(freq) + ' --------------------')
                logging.info ('\tfile\tfocus\tastig')
                start_focus = 0 # self.scope.get_obj_focus()
                fileroot = directory + r"\focus"
                iCount = 0

                self.scope.set_obj_focus(- focus_step_size * focus_steps_plus_minus)

                focus_max = [0, 0, 0, 0, ""] # max_val, fft_size, start_freq, focus_setting, filename
                for focus in range(focus_steps_plus_minus * 2):
                    filename = fileroot + str(iCount) + ".tif"
                    self.grab(filename)
                    fi = self.temcaGraph.get_focus_info()
                    if (fi['focus_score'] > focus_max[0]):
                        focus_max[0] = fi['focus_score']
                        focus_max[1] = fft_size
                        focus_max[2] = freq
                        focus_max[3] = focus
                        focus_max[4] = filename
                    if focus_max[0] > freq_max[0]:
                        freq_max = focus_max
                    if freq_max[0] > overall_max[0]:
                        overall_max = freq_max
                    s = '\t{0}\t{1}\t{2}\t{3}'.format(filename, 'focus', str(fi['focus_score']) , str(fi['astig_score']))
                    print s
                    logging.info (s)
                    self.scope.set_obj_focus(focus_step_size)
                    time.sleep(.05)
                    iCount = iCount + 1

                self.scope.set_obj_focus(- focus_step_size * focus_steps_plus_minus)
                logging.info (overall_max)
    
        

def suite():
    tests_stage = [
        #'test_calibrate', 
        #'test_reset', 
        'test_go_home',
        #'test_go_limits',
        #'test_is_moving'
        ]

    tests_video = [
        'test_autoexposure',
        #'test_focus',
        ]

    #return unittest.TestSuite(map(TestSmarActStage, tests_stage))
    return unittest.TestSuite(map(TestSmarActStageWithCamera, tests_video))


if __name__ == '__main__':
    
    logging.basicConfig(filename='test_focus.txt', level=logging.INFO,
                format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')

    suite = suite()
    unittest.TextTestRunner(verbosity=0).run(suite)

    

