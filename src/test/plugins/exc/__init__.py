
"""
This plugin is only for testing/development/demonstration purposes.

			It does not do anything generally useful.

It tests the framework's handling of plugins that raise exceptions,
and, incidentally, plugins that do not declare DEPENDENCIES at all.
"""

from os import environ
from os.path import exists,isfile,isdir

VERSION=0x00010000

def process( name, upstream_plugin_results ):

	assert isinstance( upstream_plugin_results, dict ) \
		and len(upstream_plugin_results) == 0

	# This demonstrates how one can parameterize plugin behavior using
	# environment variables.
	PARAM = environ.get( "BDQC_TEST_EXC", None )

	if isinstance(PARAM,str):
		raise RuntimeError( PARAM )

	# Otherwise just return a trivial dict categorizing the name
	# as a (F)ile, (D)irectory or (O)ther...
	if exists(name):
		return {"type":"F" if isfile(name) else ("D" if isdir(name) else "O" )}
	else: # ...or non-existent
		return None

