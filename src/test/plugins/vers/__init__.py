
"""
This plugin is only for testing/development/demonstration purposes.

			It does not do anything generally useful.

It exports a VERSION string specified by an environment variable
to test handling of version-dependent plugin reruns.
"""

from os import environ

VERSION=eval(environ.get("BDQC_TEST_VERS",'0x00010000'))

def process( name, upstream_plugin_results ):

	assert isinstance( upstream_plugin_results, dict ) \
		and len(upstream_plugin_results) == 0

	return {"VER":VERSION }

