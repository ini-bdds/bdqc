
"""
This plugin is only for testing/development/demonstration purposes.
It does not do anything.
"""

import json

TARGET = "file"
VERSION=0x00010000
DEPENDENCIES = ['bdqc.builtin.extrinsic',]

def process( filename, state ):
	pass

# A plugin may optionally contain class definitions implementing heuristics.
	
class DummyHeuristic(object):
	"""
	Describes the heuristic.
	"""
	def __init__( self, arg=None ):
		assert arg is None \
			or isinstance(arg,str) \
			or isinstance(arg,dict)
		print( "{}.__init__({})".format( self.__class__, json.dumps(arg) ) )

	def __call__( self ):
		print( "DummyHeuristic.__init__()" )

# If (and only if) a plugin provides its own heuristics it must include a
# dictionary named HEURISTIC that maps heuristic names to the classes.

HEURISTIC = {
	'heuristic':DummyHeuristic
}

