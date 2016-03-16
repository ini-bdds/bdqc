
"""
1. Load a set of file *names* by:
	a. recursively scanning directory root(s)
		i. to a given depth
		ii. for files matching a given pattern.
	b. reading manifest file(s)
2. Load a set of analysis plugins from well-known and/or
   specified location(s).
3. Resolves all declared dependencies among file analysis plugins.
4. For every file execute each plugin once in an appropriate order
	on that file

Plugins
	1. are of two types:
		a. file analysis -- concern themselves with ONE FILE
		b. accumulator analysis
	1. declare dependencies on other plugins (by plugin name).
	2. can read accumulated analysis state
	3. return either
		a. tuples of data to the executor, or
		b. indication that they did not run/were not applicable

Some plugins may not run on some files, so the data accumulation need
not be the same for all files, though we can formally impose a tabular
structure regardless: each file's accumulated data can be defined to
have one entry per analysis plugin, though that entry may be empty.
"""

import sys
import os.path
import os
import json
import time
import logging
import io

import bdqc.plugin
import bdqc.dir

ANALYSIS_EXTENSION = ".bdqc"
DEFAULT_PLUGIN_RCFILE = os.path.join(os.environ['HOME'],'.bdqc','plugins.txt')
_PLUGIN_VERSION = "VERSION"
_CACHE_VERSION  = "version"

def build_input( subjects, include=None, exclude=None, depth=None, preclude_recursion=True ):
	"""
	Create a list of absolute file paths from a variety of sources in
	the <subjects> list which may include:
	1. root directories to be recursively walked
	2. file names to include verbatim
	3. file naems of manifests which contain filenames to be included.
	"""
	accum = []
	for s in subjects:
		try:
			if s.startswith("@"):
				with open( s[1:] ) as fp:
					accum.extend( _read_manifest( fp ) )
			elif os.path.isdir( s ):
				accum.extend( bdqc.dir.walk( s, depth, include, exclude ) )
			else:
				accum.append( s )
		except RuntimeError as x:
			print( x, file=sys.stderr )
	# If user reruns bdqc.scan on a directory already containing .bdqc
	# without a sufficiently specific manifest and/or filters, recursive
	# analysis will occur that is almost certainly not wanted.
	if preclude_recursion:
		prefilter_count = len(accum)
		accum = list(filter( lambda f:not f.endswith(ANALYSIS_EXTENSION), accum ))
		if len(accum) < prefilter_count:
			logging.warning( "filtered {} *{} files".format(
				prefilter_count-len(accum), ANALYSIS_EXTENSION ) )
	return accum


def _format_time( s ):
	"""
	Convert a number of seconds into a human readable but compact string
	representation.
	"""
	# Compute days, hours, minutes, and seconds...
	d = s // (3600*24)
	s %= (3600*24)
	h = s // 3600
	s %= (3600)
	m = s // 60
	s %= 60
	# ...then a string containing "days" only if necessary.
	if d > 0:
		return "{} days + {:02d}:{:02d}".format( d, h, m )
	else:
		return "{:02d}:{:02d}:{:02d}".format( h, m, s )


class Executor(object):
	"""
	Manages the execution of a set of file-analysis plugins on a set of
	files ("subjects"). In particular, this:
	1. accumulates the results
	2. provides dependent plugins with the results of their dependencies
	"""

	def __init__( self, plugin_mgr, subjects, **kwargs ):
		assert isinstance( subjects, list ) and \
			all([isinstance(s,str) for s in subjects ])
		assert isinstance( plugin_mgr, bdqc.plugin.Manager )
		self.subjects = subjects
		self.plugin_mgr = plugin_mgr
		self.results = {}
		self.adjacent = kwargs.get( "cache",   True )
		self.clobber  = kwargs.get( "clobber", True )
		self.dryrun   = kwargs.get( "dryrun",  False ) 

	def __del__( self ):
		pass

	def __str__( self ):
		raise NotImplementedError

	def _should_run( self, plugin, cache, ran ):
		"""
		Determines whether or not a specific plugin should be run based on
		global state and the status (existence and version) of earlier
		results from the same plugin in the cache.
		"""
		assert isinstance( ran, set )
		# Note that does NOT check self.dryrun because the whole point of
		# dry runs is simulation of what would happen in a real run.
		# The dryrun flag must be handled by caller!
		if self.clobber:
			return (True, "clobbering")
		elif len(cache) == 0:
			return (True, "no earlier results exist" )
		elif plugin.__name__ not in cache:
			return (True, "no earlier results for {} exist".format( plugin.__name__ ))
		elif len(frozenset(plugin.DEPENDENCIES).intersection(ran)) > 0:
			deps = ','.join( frozenset(plugin.DEPENDENCIES).intersection(ran) )
			return (True, "one or more of this plugin's dependencies ({}) were run".format(deps) )
		else:
			# This plugin has been run earlier on this subject, but the
			# plugin itself might have been updated. Check for a version.
			oldres_version = cache[ plugin.__name__ ].get(_CACHE_VERSION, None)
			plugin_version = getattr( plugin,_PLUGIN_VERSION, None )
			if plugin_version is None or oldres_version is None:
				return ( False, "assuming earlier results are still valid (Results and/or current plugin have no version number.)" )
			# Because version is added by Executor (not plugin) we *should*
			# be able to trust here that it's the proper type, so...
			assert isinstance(oldres_version,int) \
				and isinstance(plugin_version,int)
			run = plugin_version > oldres_version
			why = "plugin version ({}) is {}greater than earlier results' version ({})".format(
				plugin_version, "" if run else "not ", oldres_version )
			return ( run, why )

	def run( self,
			accumulate:"file destination for aggregate results"=None,
			progress:"file to which progress updates are written"=None ):
		"""
		Returns a count of missing files.
		TODO: Eventually this should incorporate the multiprocessing module.
		"""
		missing = 0
		# 0. Validate arguments

		ARG_ERROR = "\"{}\" argument must be an open file or None"
		if accumulate is not None and not isinstance( accumulate, io.TextIOBase ):
			raise TypeError( ARG_ERROR.format( "accumulate" ) )
		if progress is not None and not isinstance( progress, io.TextIOBase ):
			raise TypeError( ARG_ERROR.format( "progress" ) )

		if accumulate:
			print( "{", file=accumulate )

		completed_subjects = 0
		ran = set()
		START_TIME = time.clock()
		for s in self.subjects: # for each file...

			SUBJECT_EXISTS = os.path.isfile( s )
			ran.clear()

			# 1. If the data and cache both exist and the data is newer than
			#    the cache, everything in the cache is assumed invalid.
		 	#    Otherwise, because the cache is written in its entirety (at
			#    bottom), it must be read in first. It may happen that only
		 	#    parts of the cache are updated.

			cache_file = s + ANALYSIS_EXTENSION
			if not self.clobber \
				and SUBJECT_EXISTS \
				and os.path.isfile( cache_file ) \
				and os.stat( s ).st_mtime < os.stat( cache_file ).st_mtime:
				try:
					with open(cache_file) as fp:
						cache = json.load( fp )
				except ValueError:
					logging.error( "content of {} is invalid JSON".format(cache_file) )
					# This is also thrown when cache_file is a present,
					# but empty file; handling the ValueError exception is
					# more general...includes malformed content.
					cache = {}
			else:
				cache = {}
			# An empty cache dict insures _should_run will return True.

			# 2. Apply each plugin to s.

			if SUBJECT_EXISTS or self.dryrun:

				for p in iter(self.plugin_mgr):

					do_run = self._should_run( p, cache, ran )

					if self.dryrun:
						# ALWAYS print on stdout; that's the point of dry run!
						print( "{}({}): {} because {}".format( p.__name__, s,
							"run" if do_run[0] else "skip",
							do_run[1] ) )
						if do_run[0]:
							ran.add( p.__name__ ) # *would* have been run!

					elif do_run[0]:

						# Plugins may only access DECLARED dependencies.
						# Currently this excludes implicit dependencies.
						# TODO: This strictness could be optionally relaxed.
						declared_reqs = dict( zip(
								p.DEPENDENCIES,
								[ cache[d] for d in p.DEPENDENCIES ] ) )
						try:
							d = p.process( s, declared_reqs )

							# Automatically append plugin's version to results.
							# converting results to dict if they're not already.
							if hasattr( p, _PLUGIN_VERSION ):
								if not isinstance(d,dict):
									d = {"value":d,}
								d[_CACHE_VERSION] = int( getattr( p, _PLUGIN_VERSION ) )

							# Add this plugin's results to subject's cache.
							cache[ p.__name__ ] = d
							ran.add( p.__name__ )

						except Exception as X:
							logging.error( "{} while processing {} with {}".format( X, s, p.__name__ ) )
							if not getattr(self,"ignore_exceptions",False):
								raise
					else:
						logging.info( "skipping {}({}): {}".format( p.__name__, s, do_run[1] ) )
				else:
					pass # just tagging the end of the for loop.
			else: # not SUBJECT_EXISTS
				logging.warning( "{} is missing or is not a file".format( s ) )
				missing += 1

			# 3. Store locally and/or accumulate cache of s.

			if not self.dryrun:
				results = json.dumps( cache, sort_keys=True, indent=4 )
				if self.adjacent: # store JSON results adjacent to subject
					with open( cache_file, "w" ) as fp:
						print( results, file=fp )
				if accumulate:
					if completed_subjects > 0:
						print( ",", file=accumulate )	
					print( '"{}":'.format(s), results, file=accumulate )

			# 4. Update the expected time.

			completed_subjects += 1
			if progress:
				rem_s = ( len(self.subjects) - completed_subjects ) \
					* ( ( time.clock() - START_TIME ) / completed_subjects )
				if rem_s > 0:
					time_string = _format_time( int(rem_s) )
					prog_report = "{}/{} files. time remaining: {}".format(
						completed_subjects,
						len(self.subjects),
						time_string )
					#self.prog_len = max(len(prog_report),self.prog_len)
					print( prog_report, end="\r" if progress.isatty() else "\n" )

		if accumulate:
			print( "}", file=accumulate )
		return missing


def main( args ):

	# Build lists of plugins...

	plugins = []
	# ...maybe including defaults...
	if ( args.plugins == None or args.plugins[0] == '+' ) and \
			os.path.isfile( DEFAULT_PLUGIN_RCFILE ):
		with open( DEFAULT_PLUGIN_RCFILE ) as fp:
			plugins.extend( [ l.rstrip() for l in fp.readlines() ] )
	# ...and maybe including additional.
	if args.plugins:
		for plugname in args.plugins.split(','):
			if os.path.isfile( plugname ): # ...if it's a plugin manifest
				with open( plugname ) as fp:
					plugins.extend( [ l.rstrip() for l in fp.readlines() ] )
			else:
				plugins.append( plugname )
	mgr = bdqc.plugin.Manager( plugins )

	# ...and subjects from command line args.

	subjects = build_input( args.subjects, args.include, args.exclude, args.depth )

	if args.dryrun:
		print( "These plugins..." )
		i = 0
		for p in iter(mgr):
			i += 1
			print( "{}. {} (from {})".format( i, p.__name__, p.__path__ ) )
		print( "...would be executed as follows..." )

	# Finally, create and run an Executor.

	_exec = Executor( mgr, subjects,
			dryrun = args.dryrun,
			clobber = args.clobber,
			adjacent = (not args.no_adjacent) )

	if args.dryrun:
		missing = _exec.run() # ...no need for progress indicator or an accumulator.
	else:
		prog_fp = _open_output_file( args.progress ) \
			if args.progress else None
		accum_fp = _open_output_file( args.accum ) \
			if args.accum else None

		missing = _exec.run( accum_fp, prog_fp )

		if accum_fp:
			accum_fp.close()
		if prog_fp:
			prog_fp.close()
		# TODO: Analysis( args.accum, m.plugin_mgr )
	if missing > 0:
		logging.warning( "{} file(s) were missing".format( missing ) )


############################################################################
# Command line args
# Code below here is only relevant when Executor is run as a script:
# python3 -m bdqc.scan <arguments>
# ...as opposed to
# e = bdqc.scan.Executor( plugins, subjects )

if __name__=="__main__":
	import argparse
	import re

	def _open_output_file( name ):
		"""
		Open the appropriate file according to name, handling
		the special values "stdout"/"-"
		"""
		if len(name) > 0:
			if name == "stdout" or name == "-":
				return sys.stdout
			else:
				return open( name, "w" )
		else:
			return None

	def _read_manifest( fp ):
		"""
		Read a list of filenames from fp. No validation or
		existence checks are desired at this point.
		"""
		accum = []
		line = fp.readline()
		while len(line) > 1:
			accum.append( line.rstrip() )
			line = fp.readline()
		return accum

	_parser = argparse.ArgumentParser(
		description="A framework for \"Big Data\" QC/validation.",
		epilog="""The command line interface is one of two ways to use this
		framework. Within your own Python scripts you can create, configure
		and programmatically run a bdqc.Executor.""")
	_parser.add_argument( "--plugins", "-p",
		default=None, metavar="PLUGINLIST",
		help="""Plugins to execute EITHER in addition to OR instead of the
		defaults listed in {}. The argument is a comma-separated
		list of names. Each name must be EITHER the fully-qualified name of
		a Python3 package implementing a plugin, OR the name of a file which
		itself contains a list of plugins, one per line.
		If %(metavar)s is prefixed with "+", the listed plugins AUGMENT
		the defaults; otherwise the listed plugins REPLACE the defaults.
		Note that a given plugin's dependencies will ALWAYS be added to
		whatever you specify.
		""".format( DEFAULT_PLUGIN_RCFILE ) )

	# Directory recursion options

	_parser.add_argument( "--depth", "-d",
		default=None, type=int,
		help="Maximum depth of recursion when scanning directories.")
	_parser.add_argument( "--include", "-I",
		default=None,
		help="""When recursing through directories, only files matching the
		<include> pattern are included in the analysis. The pattern should
		be a valid Python regular expression and usually should be enclosed
		in single quotes to protect it from interpretation by the shell.""")
	_parser.add_argument( "--exclude", "-E",
		default=None,
		help="""When recursing through directories, any files matching the
		<exclude> pattern are excluded from the analysis. The comments
		regarding the <include> pattern also apply here.""")

	# Output control

	_parser.add_argument( "--no-adjacent",
		action='store_true', default=False,
		help="""Don't store {} analysis files alongside the subject files""".format(ANALYSIS_EXTENSION) )
	_parser.add_argument( "--dryrun", "-D",
		action='store_true',
		help="""Don't actually run. Just describe what would be done.""")
	_parser.add_argument('-C', '--clobber',
		action='store_true', default=False,
		help="""Skip all optimizations intended to minimize work;
		rerun everything (default:%(default)s).""" )
	_parser.add_argument('-A', '--accum',
		type=str, default="",
		help="""The name of a file in which to accumulate results.
		This is required for final analysis.""" )
	_parser.add_argument('-P', '--progress',
		type=str, default="",
		help="""Name of file for progress updates (or "stdout").""")
	_parser.add_argument('-l', '--logfile',
		type=str, default="",
		help="""Name of file for logging. Logging is not performed
		unless this option is specified.""")
	_parser.add_argument('-L', '--loglevel',
		type=str, default="ERROR",
		help="""One of {\"critical\", \"error\", \"warning\", \"info\",
		\"debug\"} (default:%(default)s).""")

	_parser.add_argument( "subjects", nargs="+",
		help="""Files, directories and/or manifests to analyze. All three
		classes of input may be freely intermixed with the caveat that the
		names of manifests (files listing other files) should be prefixed
		by \"@\" on the command line.""" )

	_args = _parser.parse_args()

	# Set up logging before ANYTHING else.

	numeric_level = getattr( logging, _args.loglevel.upper(), None )
	if not isinstance( numeric_level, int):
		raise ValueError('Invalid log level: %s' % _args.loglevel )
	logging.basicConfig( filename=_args.logfile, level=numeric_level, filemode="w" )

	# If inclusion or exclusion patterns were specified, just verify they're
	# valid regular expressions here. The result of compilation is discarded
	# now. This is just an opportunity for Python to report a regex format
	# exception before entering directory recursion.

	if _args.include:
		re.compile( _args.include )
	if _args.exclude:
		re.compile( _args.exclude )

	main( _args )

