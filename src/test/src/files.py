
"""
Generate controllably-random file/directory names, "controllably"
because we'll want to match on them
Number of internal components each of which is
	fixed
	sampled from a set
	entirely random string
"""

import json
import re
from random import randrange,choice
import os

#os.makedirs()

with open("config.json") as fp:
	CONFIG = json.load(fp)

def _randomAlpha( length=5 ):
	return ''.join([ chr(0x61+randrange(26)) for i in range( length )])

def stringFromTemplate( template, params ):
	PLACEHOLDER = "\[-?[0-9]+\]"
	fix = re.split( PLACEHOLDER, template )
	var = [ int(template[ slice(*m.span()) ][1:-1]) for m in re.finditer( PLACEHOLDER, template ) ]
	z = zip(fix,[ _randomAlpha(abs(v)) if v < 0 else choice(params[v]) for v in var ])
	s =  "".join( [ "".join(pair) for pair in z ] )
	# Following accounts for quirks of both re.split and zip.
	# zip stops at shortest list; re.split
	if len(fix) > len(var):
		assert len(fix) == len(var) + 1
		# ...shouldn't be too (two) much longer!
		s += fix[-1]
	return s

"""
Generate controllably-random directory trees
number of leaves
max depth
"""

"""
Generate controllably random content.
File count
"""

"""
"Pollute" the directory tree with image files in a controllably
random way.
"""
if __name__=="__main__":
	import sys
	print( stringFromTemplate( sys.argv[1], sys.argv[2].split(',') ) )

