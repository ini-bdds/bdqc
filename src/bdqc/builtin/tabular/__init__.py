
"""
Its results are the basis for the first-round classification of
a file as either 1) ASCII text, 2) UTF-8 text, or 3) binary.
"""

import bdqc.builtin.compiled
import json

TARGET = "file"
VERSION=0x00010000
DEPENDENCIES = ['bdqc.builtin.extrinsic',]

def process( name, state ):
	"""
	C code simply emits a histogram and a transition matrix.
	TODO: Character histogram and a transition matrix should both be subject
	to significant "value-added" analysis here.
	"""
	if not state.get('bdqc.builtin.extrinsic',False):
		return None
	if state['bdqc.builtin.extrinsic']['readable'] != "yes":
		return { 'character_histogram':None,
			'transition_histogram':None,
	   		'tabledata':None }
	tabledata = None
	try:
		result = bdqc.builtin.compiled.tabular_scan( name )
		tabledata = json.loads( result )
	except Exception:
		pass
	return tabledata

if __name__=="__main__":
	import sys
	print( process( sys.argv[1], {'bdqc.builtin.extrinsic':{'readable':"yes"}} ) )

