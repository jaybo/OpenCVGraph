"""
Manually move the stage using arrow keys.
Measure the dx, dy offsets via the matcher.

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

#recalibrate
#test: is_moving

#general is_moving test

#get physical limits API

#test: home limits home

#do we need a software stop to prevent hitting physical stuff?

#test: settling time

#	for x, y, xy:
#		for 10 mS increments measure settling time via focus


#test: low vibration mode

#test: hold positon

#test: mini endurance

#test: verify all 16 girds are fully visible



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

    def grab(self):
        self.temcaGraph.wait_start_of_frame()
        self.temcaGraph.grab_frame('j:/junk/junk.tif', 0,0) # filename doesn't matter in preview
        self.temcaGraph.wait_graph_event(self.temcaGraph.eventAsyncProcessingCompleted)

    def close(self):
        self.temcaGraph.close()
        self.temcaGraph.wait_graph_event(self.temcaGraph.eventFiniCompleted)

    def test_focus(self):
        ''' loop through x, y stage positions.  Make random moves (within a range), and then collect focus measurements '''
        steps = 10
        moves_per_step = 3
        time_min = .0
        time_max = .100
        dx = self.x_max - self.x_min
        dy = self.y_max - self.y_min
        step_min = .1
        step_max = .2

        sx, sy = self.stage.get_pos()

        #for sx in frange (self.x_min, self.x_max, dx / steps):
        #    for sy in frange (self.y_min, self.y_max, dy / steps):
        logging.info ('\tsx\tsy\tdt\tfocus\tastig')

        moves = create_move_list(sx, sy, step_min, step_max, step_min, step_max, moves_per_step)
        for move in moves:
            for dt in frange(time_min, time_max, .010): # 10mS steps
                x, y = move
                # move to start position
                self.stage.set_pos(sx, sy)
                while (self.stage.is_moving()):
                    pass
                self.grab()

                # do the move, don't wait for move to complete
                self.stage.set_pos(x, y)
                time.sleep(dt)
                self.grab()
                fi = self.temcaGraph.get_focus_info()
                logging.info ('\t{0}\t{1}\t{2}\t{3}\t{4}'.format(str(sx), str(sy), str(dt), str(fi['focus_score']) , str(fi['astig_score'])))
                while (self.stage.is_moving()):
                    pass
        # back home
        self.stage.set_pos(sx, sy)
        while (self.stage.is_moving()):
            pass
        

def suite():
    tests_stage = [
        #'test_calibrate', 
        'test_reset', 
        'test_go_home',
        #'test_go_limits',
        #'test_is_moving'
        ]

    tests_video = ['test_focus',]

    return unittest.TestSuite(map(TestSmarActStage, tests_stage))
    #return unittest.TestSuite(map(TestSmarActStageWithCamera, tests_video))


if __name__ == '__main__':
    
    logging.basicConfig(filename='test_stage.txt', level=logging.INFO,
                format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')

    suite = suite()
    unittest.TextTestRunner(verbosity=0).run(suite)

    

