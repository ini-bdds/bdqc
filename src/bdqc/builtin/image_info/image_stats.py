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
                
                # read image and instantiate ImageStat.Stat class
                img= Image.open(name)
                img_stats= ImageStat.Stat(img)
                
                # height, width, depth, matrix size (h * w * d)
                height, width= img.size
                channel_count= len(img.mode) # this is depth (d)
                size= height * width * channel_count
                
                # stats for each channel
                img_extrema = img_stats.extrema
                img_count = img_stats.count
                img_sum = img_stats.sum
                img_sum2 = img_stats.sum2
                img_mean = img_stats.mean
                img_rms = img_stats.rms
                img_median = img_stats.median
                img_var = img_stats.var
                img_stdev = img_stats.stddev
                
                # map channels to dictionary values                            
                chans= ['r', 'g', 'b']                
                
                channel_list= []
                channel_dict= {}
                
                for count, chan in enumerate(chans):
                    try:
                        channel_dict[chan]= dict(extrema= img_extrema[count], count= img_count[count], chan_sum= img_sum[count],
                                            chan_sum2= img_sum2[count], mean= img_mean[count], rms= img_rms[count],
                                            median= img_median[count], var= img_var[count], stdev= img_stdev[count])
                    except: # if channel does not exist, example black and whit photo
                        channel_dict[chan]= {}
                
                channel_list.append(channel_dict) # create a list of dicts, one dict for each channel
                
                channel_stats= {}
                channel_stats.update({'channels': channel_list}) # nest into a channels dict                      
                
            except:
                return None
            
        else:  # extension not supported
            return None
            warnings.warn(EXTENSION_WARNING)
            
        # add image attributes to inner dict
        channel_stats['height']= height
        channel_stats['width']= width
        channel_stats['depth']= channel_count
        channel_stats['size']= size
        
        image_data= {}
        image_data.update({'image': channel_stats}) # add everything to outer dict containing all image information
        
    

    return image_data

# Unit test
if __name__=="__main__":
    import sys
    if len(sys.argv) < 2:
        print("{} <filename>".format(sys.argv[0]), file=sys.stderr)
        sys.exit(-1)
    print(process(sys.argv[1], {'bdqc.builtin.extrinsic':{'readable':"yes"}}))    