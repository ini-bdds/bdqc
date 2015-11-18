import argparse
import sys
import json
import os.path

_parser = argparse.ArgumentParser(
	description="Slice one statistic out of (JSON-formatted) analysis output." )

#_parser.add_argument( "--plugins", "-m",
#	default=os.path.join(os.environ['HOME'],'.bdqc','plugins.txt'),
#	help="File listing analysis plugins to execute.\nDefault: %(default)s")
_parser.add_argument( "json_file",
	help="""\
	A JSON file containing the results of file analysis.""" )
_parser.add_argument( "plugin",
	help="""\
	Which plugin's analysis you want to dump.""" )
_parser.add_argument( "analysis",
	help="""\
	Which analysis/statistic you want to dump from the given plugin.""" )
_args = _parser.parse_args()

#if any([x is None for x in (_args.json_file, _args.plugin, _args.analysis)]):
#	print( "No arguments are optional", file=sys.stderr )
#	sys.exit(-1)

if not os.path.isfile( _args.json_file ):
	print( "{} is not a file".format( _args.json_file ), file=sys.stderr )
	sys.exit(-1)

try:
	with open( _args.json_file ) as fp:
		data = json.load( fp )
except Exception as e:
	print( e )
	sys.exit(-1)

try:
	for kv in data.items():
		print( kv[0], kv[1][ _args.plugin ][ _args.analysis ], sep="\t" )
except KeyError as e:
	print( e )

