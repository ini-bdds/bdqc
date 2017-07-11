"""
This plugin examines and records attributes of image files
"""

import os
import os.path
import cv2 # TODO change to specific function set
import numpy as np
import pandas as pd # TODO probably won't use, re-eval

TARGET  = "file"
VERSION = 0x00010100 # TODO check on this version number, align with project convention
DEPENDENCIES = ['bdqc.builtin.extrinsic',]
PERMITTED_EXTENSIONS = ['jpg', 'bmp', 'tiff', 'png', 'sr', 'ras', 'dib', 'jp2', 'pbm', 'pgm', 
             'ppm', 'tif', 'jpeg', 'jpe']

def process(name, state):    
    
    # TODO update to reflect reality
    """
    This plugin only examines attributes of the file visible "from the
    outside"--that is, without opening and reading any of its contents.
    readable { 'absent','notfile','perm','yes' }
    """    
    
    #if not state.get('bdqc.builtin.extrinsic',False):
     #   return None
        
    if state['bdqc.builtin.extrinsic']['readable'] == "yes":
        
        ext = os.path.splitext(name)[1][1:] # skip over the '.'
        
        if ext.lower() in PERMITTED_EXTENSIONS:
            
            # collect remaining attributes
            try:
                img= cv2.imread(name)
                height, width, depth= img.shape
                size= np.size(img)
            except: # TODO issue warning or raise, possibly log 
                return None
            
        else:  # extension not supported
            return None # TODO issue warning and possibly log
               
        attributes= dict(extension= ext, height= height, width= width, depth= depth, size= size)

    return attributes

# Unit test
if __name__=="__main__":
    import sys
    if len(sys.argv) < 2:
        print("{} <filename>".format(sys.argv[0]), file=sys.stderr)
        sys.exit(-1)
    print(process(sys.argv[1], {'bdqc.builtin.extrinsic':{'readable':"yes"}}))    