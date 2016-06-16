
"""
This plugin is only for testing/development/demonstration purposes.

			It does not do anything generally useful.

It emits a variety of stats subject to a variety of conditions strictly
to facilitate setup of a variety of test cases.
"""

from os import environ
from hashlib import md5

VERSION=0x00010000

DEPENDENCIES = ['bdqc.builtin.extrinsic',]

def process( filename, state ):

	assert isinstance( state, dict )

	# This demonstrates how one can parameterize plugin behavior using
	# environment variables.
	THRESHOLD = environ.get("BDQC_TEST_LINES",0)

	if state['bdqc.builtin.extrinsic']['size'] > THRESHOLD:
		with open(filename) as fp:
			data = []
			for l in [ line.rstrip() for line in fp.readlines() ]:
				h = md5()
				h.update( bytes(l,"utf8") )
				data.append( {'len':len(l), h.name:h.hexdigest() } )
			return {"line-count":len(data), "lines":data }
	else:
		return None

