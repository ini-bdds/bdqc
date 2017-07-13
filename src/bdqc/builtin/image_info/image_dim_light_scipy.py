"""
This plugin examines and records height, width, depth, and matrix size of supported image files.
"""

import os
import os.path
import scipy.ndimage
import warnings

TARGET  = "file"
VERSION = 0x00010100
DEPENDENCIES = ['bdqc.builtin.extrinsic',]
PERMITTED_EXTENSIONS = ['jpg', 'bmp', 'tiff', 'png', 'sr', 'ras', 'dib', 'jp2', 'pbm', 'pgm', 
             'ppm', 'tif', 'jpeg', 'jpe']
EXTENSION_WARNING = "Extension Not Supported Return = None"

def process(name, state):    
    

    """
    This plugin only examines attributes of the file visible "from the
    outside"--that is, without opening and reading any of its contents.
    readable { 'absent','notfile','perm','yes' }
    """    
    
    if not state.get('bdqc.builtin.extrinsic',False):
        return None
        
    if state['bdqc.builtin.extrinsic']['readable'] == "yes":
        
        #import pdb;pdb.set_trace()
        ext = os.path.splitext(name)[1][1:] # skip over the '.'
        
        
        if ext.lower() in PERMITTED_EXTENSIONS:
            
            # collect remaining attributes
            try:
                height, width, depth = scipy.ndimage.imread(name).shape
                size= height * width * depth
            except:
                return None
            
        else:  # extension not supported
            return None
            warnings.warn(EXTENSION_WARNING)
            
               
        attributes= dict(extension= ext, height= height, width= width, depth= depth, size= size)

    return attributes

# Unit test
if __name__=="__main__":
    import sys
    if len(sys.argv) < 2:
        print("{} <filename>".format(sys.argv[0]), file=sys.stderr)
        sys.exit(-1)
    print(process(sys.argv[1], {'bdqc.builtin.extrinsic':{'readable':"yes"}}))   