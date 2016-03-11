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
from temca_graph import *
import msvcrt
from pytemca.stage.stage import RadishStage, DummyRadishStage
from pytemca.scope import scope
from pytemca.stage.smarAct.smaract_stage import SmarActStage

if __name__ == '__main__':

    logging.basicConfig(level=logging.INFO,
                format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    #while True:
    #    print(ord(msvcrt.getch()))r

    small = 1
    med = 512
    large = 1024

    keys = {'u': small,
            'd': -small,
            'U': med,
            'D': -med,
            }


    def process_keys():
        if msvcrt.kbhit():
            k = ord(msvcrt.getch())
            if k in keys:
                return keys[k]
            if k == 224:
                # handle arrow keys
                k = ord(msvcrt.getch())
                if k == 72:
                    # up arrow
                    return small;
                elif k == 80:
                    # down arrow
                    return -small
                elif k == 77:
                    # right arrow
                    return med
                elif k == 75:
                    # left arrow
                    return -med
                return None
            return k
        return None

    #stage = RadishStage("*", 8000, 8001, "http://10.128.26.51:8090")
    #stage = DummyRadishStage("*", 8000, 8001, "http://localhost:8090")
    #stage = SmarActStage()

    scope = scope.ScopeConnection(5)
    scope.beep()
    scope.htcond(80, 120, 3)

    #while True:
    #    k = process_keys()
    #    if k:
    #        print k
    #        if k == 27:
    #            exit()
    #        # scope_set_obj_focus(k)        

    #objFocus = scope.get_obj_focus()
    #scope.set_obj_focus(1)
    #objFocus = scope.get_obj_focus()




    