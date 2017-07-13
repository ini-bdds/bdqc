"""
This plugin examines and records height, width, depth, and matrix size of supported image files.
"""

import os
import os.path
from PIL import ImageStat, Image
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
        
        ext = os.path.splitext(name)[1][1:] # skip over the '.'
        
        if ext.lower() in PERMITTED_EXTENSIONS:
            
            # collect remaining attributes
            try:
                
                # stats for each channel
                img= Image.open(name)
                img_stats= ImageStat.Stat(img)
                img_extrema = img_stats.extrema
                img_count = img_stats.count
                img_sum = img_stats.sum
                img_sum2 = img_stats.sum2
                img_mean = img_stats.mean
                img_rms = img_stats.rms
                img_median = img_stats.median
                img_var = img_stats.var
                img_stdev = img_stats.stddev
                
                # height, width, depth, matrix size (h * w * d)
                height, width= img.size
                depth= len(img.mode)
                size= height * width * depth
            except:
                return None
            
        else:  # extension not supported
            return None
            warnings.warn(EXTENSION_WARNING)
            
               
        attributes= dict(extension= ext, extrema= img_extrema, count= img_count, img_sum= img_sum, 
                        sum2= img_sum2, mean= img_mean, rms= img_rms, median= img_median, var= img_var,
                        stdev= img_stdev, height= height, width= width, depth= depth, size= size)

    return attributes

# Unit test
if __name__=="__main__":
    import sys
    if len(sys.argv) < 2:
        print("{} <filename>".format(sys.argv[0]), file=sys.stderr)
        sys.exit(-1)
    print(process(sys.argv[1], {'bdqc.builtin.extrinsic':{'readable':"yes"}}))    