
import sys
import importlib
import json

def loadConfiguration( fpath ):
	"""
	Loads an "across-files" analysis configuration in JSON format
	from a file, dynamically importing modules as required.
	Returns a dict mapping path-selectors to heuristic instantiations.
	"""
	config = {'*':[] } # Preload the config with the global selector.
	with open(fpath) as fp:
		j = json.load( fp )
	for hspec,pathconf in j.items():
		# Each plugin may have zero or more path filters and,
		# each path filter may optionally be associated with a unique heuristic
		# configuration. In any event, each heuristic spec must be
		# associated with a JSON Object, String, or null.
		if not ( isinstance(pathconf,dict) \
			or isinstance(pathconf,str) \
			or pathconf is None ):
			raise RuntimeError("expected JSON Object, String, or null associated with " + hspec )

		# Heuristic identifiers consist of provider:selector where
		# <provider> is the name of the Python module and <selector>
		# is an OPTIONAL selector used when a module provides more than one.
		if hspec.find(':') >= 0:
			provider,selector = hspec.split(':')
		else:
			provider,selector = hspec,None

		# Try to load the Python module...
		try:
			module = importlib.import_module( provider )
		except ImportError as x:
			raise # Let it percolate up.

		# ...and confirm it defines a heuristic dict.
		if not ( hasattr(module,'HEURISTIC') \
				and isinstance( module.HEURISTIC, dict ) ):
			raise RuntimeError( provider + " does not contain a dict name HEURISTIC" )

		# Select the specified heuristic class or just use the first.
		heuristic_class = \
			module.HEURISTIC[ selector ] \
			if selector \
			else next( iter(module.HEURISTIC.values()) )

		# Create as many instantiations as necessary.
		if isinstance( pathconf, dict ):
			# heuristic_class implements a parameterized heuristic, so
			# pathconf contains one or more configurations each associated
			# with a distinct path filter.
			for path,conf in pathconf.items():
				h = heuristic_class( conf )
				try:
					config[ path ].append( h )
				except KeyError:
					config[ path ] = [ h, ]
			# Note an empty dict is not an error; no heuristic is created.
		else:
			# heuristic_class is either unparameterized or doesn't require
			# parameters, so the string is nothing but a path filter.
			h = heuristic_class()
			if isinstance( pathconf, str ):
				try:
					config[ pathconf ].append( h )
				except KeyError:
					config[ pathconf ] = [ h, ]
			else:
				assert pathconf is None # ...insured above.
				config['*'].append( h )
	return config

############################################################################
# A unit test of sorts...

if __name__=="__main__":
	for k,v in loadConfiguration( sys.argv[1] ).items():
		print( k, v )

