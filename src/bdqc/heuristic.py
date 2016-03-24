
import sys
import importlib
import json

def loadConfiguration( fpath ):
	"""
	Loads an "across-files" analysis configuration in JSON format
	from a file, dynamically importing modules as required.
	"""
	h = []
	with open(fpath) as fp:
		j = json.load( fp )
	for k,v in j.items():
		if isinstance(v,dict):
			pass
		elif isinstance(v,str):
			pass
		elif v is None:
			pass
		else:
			raise RuntimeError("expected JSON Object, String, or null at " + k )
		# Heuristic identifiers consist of module:heuristic where <module> is
		# the name of the Python module and heuristic is an optional selector
		# used when a module provides more than one heuristic.
		if k.find(':') >= 0:
			mname,hname = k.split(':')
		else:
			mname,hname = k,None
		m = None
		try:
			m = importlib.import_module( mname )
		except ImportError:
			pass
		if m is not None and hasattr(m,'HEURISTIC'):
			o = m.HEURISTIC[hname]( v )
			print( o.__doc__.strip() )

if __name__=="__main__":
	loadConfiguration( sys.argv[1] )

