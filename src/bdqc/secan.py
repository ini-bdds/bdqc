
"""
Given either
1. some root directories to scan for .bdqc cache files and/or
2. aggregate JSON file(s)

...scan all the JSON. In both cases (which can be arbitrarily interleaved)
scanning consumes one file's JSON at a time. Identify all features of
interest according to configuration and build tmp files containing vectors.

Concurrently maintain in memory the vector of filenames consumed and a map
from statistic names to the tmp files containing them.

Whenever a new statistic is identified its vector must be padded with
placeholders for all the already-scanned files that did not contain it.
Ideally, when a set of presumed-similar files are in fact similar, this
should never happen.

Heuristic analysis

Motivation
The heuristic analysis implemented here is based on an analogy between
meta-analysis of data and the process of differentiation in calculus.
Specifically, repeated differentiation of a some (e.g. polynomial) functions
leads eventually to a constant (and, then, to 0). Similarly, as one
progresses through progressively more indirect levels of data analysis--that
is, meta analyses, values can become constant when the underlying data is
a priori "similar enough."

For example, if one is analyzing files containing tables disregards the
content of (values in) columns and focuses on column count per table (an
example of metadata), one might reasonably expect all files to have the same
column count (if the files are indeed all "similar").

In the context of BDQC wherein the Stage 1 analysis runs a dynamically-
determined set of plugins, one might expect every file (if they're truly
"similar") to have received identical "treatment" by plugins in Stage 1.
This implies that no file is missing any statistic (JSON path) that is
present in one or more other files.

It makes sense to apply these kinds of heuristic to a meta-analysis of
practically any underlying data. Clearly, these are highly indirect tests.

Define an ad hoc hierarchy of heuristics:
1. universal
	a. no missing values (identical plugin-treatment)
	b. uniform type inference (inc. identical matrix dimensions)
		(If matrices' dimensions differ between files, then missing
		 values will be present, so (if matrices are traverse these)
		 states are redundant.)
2. constraint-based
	i. user-supplied XPath-inspired
	ii. plugin-specific/plugin-suggested (maybe xpath based?)
		e.g. bdqc.builtin.tabular might supply {"column_count const",
			"categorical columns have same level sets"}

Column selectors:
	path match, e.g.
		"/x/y/[p,q-r]/z,<type>
Constraints:
	column_cardinality <comparison> <int>
	value_cardinality <comparison> <int>
	value <comparison> <values>
	has_gaussian_outliers
"""

import sys
import os
import os.path
import json
import tempfile

import bdqc.dir

from bdqc.statistic import Descriptor

# Matrices may be traverse:
# 1. always
# 2. only if they have scalar component type
# 3. only if they have object component type
# 4. or never.
_ALWAYS_TRAVERSE = True
_EMPTY_MATRIX_TYPE = ((0,),None)

# These are the only additional constraints we put on plugin-generated JSON.
_MSG_BAD_ARRAY  = "Arrays must constitute matrices (N complete dimensions with uniform component type)"
_MSG_BAD_OBJECT = "Objects must not be empty"

if __debug__:
	_is_scalar = lambda v:\
		isinstance(v,int) or \
		isinstance(v,float) or \
		isinstance(v,str)

def _json_matrix_type( l ):
	"""
	Returns a pair (dimensions,element_type) where 
	1. dimensions is a tuple of integers giving each dimension's size, and
	2. element_type is an integer type code.
	For example, if f is a float in...

		[ [[f,f,f,f],[f,f,f,f]],
		  [[f,f,f,f],[f,f,f,f]],
		  [[f,f,f,f],[f,f,f,f]] ]

	...then the return is...

	( (3,2,4), Descriptor.TY_FLOAT )

	Note that TY_NONSCALAR is a legitimate element type, so...
		[ [ {...}, {...}, {...} ],
		  [ {...}, {...}, {...} ],
		  [ {...}, {...}, {...} ],
		  [ {...}, {...}, {...} ] ]

	...is ( (4,3), Descriptor.TY_NONSCALAR )

	Returns None if either:
	1. the matrix is not complete--there is inconsistency in dimensions, or
	2. the element type is not identical throughout.

	Warning: this function is recursive; mind your stack!
	"""
	if not ( isinstance(l,list) and len(l) > 0 ):
		return False
	if all([isinstance(elem,list) for elem in l]):
		types = [ _json_matrix_type(elem) for elem in l ]
		# None percolates up...
		if any([elem is None for elem in types]) or \
			len(frozenset(types)) > 1:
			return None
		else:
			return ( (len(l),)+types[0][0], types[0][1] )
	else: # l is a list that contains AT LEAST ONE non-list
		types = [ Descriptor.scalar_type(elem) for elem in l ]
		# If there are multiple types it's not a matrix at all (by our
		# restrictied definition in this code)
		if len(frozenset(types)) > 1:
			return None
		else:
			return ( (len(types),), types[0] ) 


class Cache(object):
	"""
	Run-length encodes objects
	"""
	def __init__( self, placeholder_count ):
		self.store = tempfile.SpooledTemporaryFile(mode="w+")
		self.count = placeholder_count
		self.total = self.count
		self.last  = None
		self.flushes = 0
		self.types_flushed = {'b':0,'i':0,'f':0,'z':0,'s':0,'t':0}
		self.missing = 0

	def __str__( self ):
		types = "b:{b},i:{i},f:{f},z:{z},s:{s},t:{t}".format( **self.types_flushed )
		return "{}/{}/{}".format( self.total, self.missing, types )

	def _flush( self ):
		"""
		Emit a line of:
		<count>\t<type>\t<value>
		...into the file backing this Cache.
		<type> is one of {'b','i','f','z','s','t','N'} for
		'bool', 'int', 'float', 'str', 'set', 'tuple', 'None', respectively.
		(Tuples hold the dimensions of matrices, and sets are emitted
		 whenever a 1D matrix of strings is encountered.)
		"""
		assert self.count > 0
		# Update relevant count statistics...
		if self.last is None:
			self.missing += self.count
			ty = "-" # Not really a type, just for output below!
		else:
			# Determine a type code for what we're about to flush.
			ty = type(self.last).__name__
			if ty.startswith('s'): # need to distinguish str's and set's!
				ty = 'z' if isinstance(self.last,str) else 's' # set
			else:
				ty = ty[0]
			self.types_flushed[ty] += self.count # Key MUST always pre-exist!
		self.total += self.count
		self.flushes += 1
		# ...flush...
		print( self.count, ty, repr(self.last), sep="\t", file=self.store )
		# ...and reset.
		self.count = 0

	def __call__( self, value ):
		"""
		Just increment the count of consecutive identical values if value is
		the same as self.last.
		"""
		if self.count > 0:
			if self.last != value:
				self._flush()
				self.last = value
		else:
			self.last = value
		self.count += 1

	def fileptr( self ):
		if self.count > 0:
			self._flush()
		return self.store

	def missing_data( self ):
		"""
		Returns a boolean indication of whether or not this cache was
		missing any data (relative to other caches in the analysis run).
		"""
		return self.missing > 0

	def uniquely_typed( self ):
		"""
		Indicates whether or not exactly one type has a non-zero count
		in types_flushed.
		Note that the state of missing data has no impact on whether or
		not the cache is uniquely typed; missing data is not a "type" and
		is accounted separately.
		"""
		return sum([ int(v > 0) for v in self.types_flushed.values()]) == 1


class MetaAnalysis(object):
	"""
	Contains a configuration consisting of
	1. heuristics to be applied
	2. paths to complex multi-level plugins
	...which guide accumulation of data from a combination of aggregate JSON
	files and directory scanning.
	Heuristics to be applied drive extraction: any 1st level statistic of a
	type for which a heuristic has been specified is extracted. In addition,
	deeper statistics.
	First-level univariate statistics for which a heuristic exists are
	automatically extracted. Other statistics more deeply nested in cached
	or aggregated analyses are extracted only if mentioned by name.
	"""
	def __init__( self ):
		"""
		"""
		# Initialize the cache map
		self.cache = {}
		self.files = []

	def __call__( self, filename, analysis:"JSON data represented as Python objects"=None ):
		"""
		Process all the data associated with a single primary file.

		If <filename>'s analysis was aggregated into a larger JSON file,
		then <analysis> will be provided, having been parsed out by the
		caller. Otherwise, we need to load the analysis from <filename>
		which should end in ".bdqc". More specifically, analysis will be
		None when this is called from a directory walker
		"""
		# Load the JSON data from name if it wasn't provided...
		if analysis is None:
			if not (
				filename.endswith(".bdqc") or
				filename.endswith(".json") ):
				raise RuntimeError("expected \".bdqc\" cache file, received "+filename )
			with open( filename ) as fp:
				analysis = json.loads( fp )
			filename = filename[:-5] # snip off the suffix.
		# ...then analyze it.
		# _visit calls self.accum for each non-traversable element in the
		# JSON content. (See elsewhere for what "non-traversable" means.)
		self._visit( analysis )
		self.files.append( filename )

	def _accept( self, statname ):
		"""
		Test the statname against path based filters.
		"""
		return True # for now

	def _addstat( self, statname, value, meta=None ):
		"""
		Actually write a datum to the appropriate cache.
		Called by _visit every time it reaches a leaf node (necessarily
		a scalar) or a JSON Array (which might represent an arbitrary
		dimensional matrix or a set).
		The latter case allows:
		1. emission of matrix metadata 
		2. emission of potential sets as values
		...which are mutually exclusive. As long as these possibilities
		*are* mutually exclusive we don't need any additional path
		differentiators--that is, we can just use the JSON path.

		Returns:
			Boolean value for which True means "descend into the matrix."
		(A True is only meaningful if the node is, in fact, a JSON Array).
		"""
		descend = (meta is not None) and (_ALWAYS_TRAVERSE or meta[1] == 0)
		# Insure a cache exists for the statistic...
		try:
			cache = self.cache[ statname ]
		except KeyError:
			# ...IF AND ONLY IF it passes path-based filters that define
			# statistics of interest.
			if not self._accept( statname ):
				return descend # ...without caching anything. 
			cache = Cache(len(self.files))
			self.cache[ statname ] = cache
		# ...then cache the value(s)
		if meta:
			# If it's a vector (1D matrix) of strings and...
			if len(meta[0]) == 1 and meta[1] == Descriptor.TY_STR:
				S = set(value)
				# ...the cardinality of the set of values equals the count
				# of values, then encode it as a set.
				if len(S) == meta[0][0]:
					cache( S )
					descend = False
					# ...because the list was just treated as a *value*.
				else:
					cache( meta )
			else:
				# It's either multi-dimensional or it's element type
				# is Numeric or non-scalar.
				cache( meta )
		else:
			assert value is None or _is_scalar( value )
			cache( value )
			descend = False
		return descend

	def _visit( self, data ):
		"""
		Traverse a JSON object, identifying:
		1. scalars (numeric or strings)
		2. matrices
		Matrices are DEFINED as nested Arrays of a single-type (which may itself
		be an Object). Scalar matrices are nested Arrays of a scalar type.
		Nested arrays in which the leaves are not of uniform type are not
		considered matrices.

		Traversal of matrices may be made conditional on the component type.

		One-dimensional matrices are, of course, vectors, and when their content
		is String they may be interpreted as sets.

		The root element (data) is always going to be a JSON Object (Python
		dict) mapping plugin names to the data each plugin produced.
		"""
		assert data and isinstance( data, dict ) # a non-empty dict
		# Using iteration rather than recursion.
		path = []
		stack = []
		node = data
		i = iter(node.items())
		while node:
			push = None
			# Move forward in the current compound value (object or array)
			if isinstance(node,dict): # representing a JSON Object
				try:
					k,v = next(i)
					if isinstance(v,dict):
						if not len(v) > 0:
							raise RuntimeError(_MSG_BAD_OBJECT)
						push = ( k, i )   # NO callback.
					else: # It's a scalar or list, each of which *always*
						# ...trigger a callback.
						pa = '/'.join( path+[k,] )
						if isinstance(v,list):
							# Matrices MAY be empty, but the element type
							# of an empty matrix cannot be determined.
							mt = _json_matrix_type(v) if len(v) > 0 else _EMPTY_MATRIX_TYPE
							if mt is None:
								raise RuntimeError(_MSG_BAD_ARRAY)
							
							# If there are nested Objects (or we're traversing
							# all), descend...
							if self._addstat( pa, v, mt ) and len(v) > 0:
								push = ( k, i )
						else:
							assert v is None or _is_scalar(v)
							self._addstat( pa, v )
				except StopIteration:
					node = None
			else: # node represents a JSON Array
				assert isinstance(node,list) and isinstance(i,int)
				if i < len(node):
					v = node[i]
					if isinstance(v,dict):
						if not len(v) > 0:
							raise RuntimeError(_MSG_BAD_OBJECT)
						push = ( str(i), i+1 ) # NO callback.
					else: # It's a scalar or list, each of which *always*
						# ...trigger a callback.
						pa = '/'.join( path+[str(i),] )
						if isinstance(v,list):
							# Matrices MAY be empty, but the element type
							# of an empty matrix cannot be determined.
							mt = _json_matrix_type(v) if len(v) > 0 else _EMPTY_MATRIX_TYPE
							if mt is None:
								raise RuntimeError(_MSG_BAD_ARRAY)
							
							# If there are nested Objects (or we're traversing
							# all), descend...
							if self._addstat( pa, v, mt ) and len(v) > 0:
								push = ( str(i), i+1 )
							else:
								i += 1
						else:
							assert v is None or _is_scalar(v)
							self._addstat( pa, v )
							i += 1
				else:
					node = None
			# If v is not an empty compound value, a scalar, or a matrix--in
			# other words, if v is further traversable, push the current node
			# onto the stack and start traversing v. (Depth-first traversal.)
			if push:
				assert (isinstance(v,list) or isinstance(v,dict)) and len(v) > 0
				path.append( push[0] )
				stack.append( (node,push[1]) )
				i = iter(v.items()) if isinstance(v,dict) else 0
				node = v
			# If we've exhausted the current node, pop something off the stack
			# to resume traversing. (Clearly, exhaustion is mutually-exclusive
			# of encountering a new tranversable. Thus, the elif...)
			elif (node is None) and stack:
				path.pop(-1)
				node,i = stack.pop(-1)
		# end _visit

	def analyze( self ):
		"""
		This reloads (as needed) cached vectors and analyzes in the following
		sense:
		1. Verify no missing data exists. Missing data suggests structural
			differences existed between the files and further comparison
			is probably unwarranted--that is, at least some of the files
			*are* fundamentally different.
		2. Each column is unambiguously typed. That is, each column contains
			elements that are unamibuously one of the following:
			A. scalars
				i. floating-point values
				ii. integer values
				iii. string values
			B. sets of strings
			C. arbitrary dimension matrix of one scalar type
		3. Configured heuristics are applied to all columns. In particular,
			columns that are "matched" by rule selectors are tested by the
			associated rules.
		"""
		missing_data = [ v.missing_data() for v in self.cache.values() ]
		if any(missing_data):
			# Determine whether the columns missing data are the majority
			N = sum([ int(b) for b in missing_data ])
			print( "a minority {}/{} of statistics have {} data:".format(
				N,
				len(self.cache),
				"missing" if N < len(self.cache)/2.0 else "excess" ) )
			for k in sorted(self.cache.keys()):
				if self.cache[k].missing_data():
					print( k, str(self.cache[k]) )
			return # early, and don't bother with further analysis.
		if not all([ v.uniquely_typed() for v in self.cache.values() ]):
			print( "non-uniform types" )
			for k in sorted(self.cache.keys()):
				if not self.cache[k].uniquely_typed():
					print( k, str(self.cache[k]) )

	def dump( self, prefix="." ):
		n = 0
		for k in sorted(self.cache.keys()):
			ifp = self.cache[k].fileptr()
			ifp.seek(0)
			with open( os.path.join( prefix, "{:04d}.txt".format(n) ), "w" ) as ofp:
				print( "#", k, file=ofp )
				line = ifp.readline()
				while len(line) > 0:
					ofp.write( line )
					line = ifp.readline()
				print( "#", str(self.cache[k]), sep="", file=ofp )
			n += 1

def _main( args ):

	a = MetaAnalysis()

	for s in args.sources:
		if os.path.isdir( s ):
			# Look for "*.bdqc" files under "s/" each of which contains
			# *one* file's analysis as a single JSON object.
			# dir.walk calls a visitor with the filename
			bdqc.dir.walk( s, args.depth, args.include, args.exclude, a )
		elif os.path.isfile( s ):
			# s is assumed to contain a ("pre") aggregated collection of analyses
			# of multiple files.
			with open(s) as fp:
				for filename,content in json.load( fp ).items():
					a( filename, content )
		else:
			raise RuntimeError( "{} is neither file nor directory".format(s) )
	a.dump( 'tmp' )
	a.analyze()


if __name__=="__main__":
	import argparse

	_parser = argparse.ArgumentParser(
		description="A framework for \"Big Data\" QC/validation.",
		epilog="""The command line interface is one of two ways to use this
		framework. Within your own Python scripts you can create, configure
		and programmatically run a bdqc.Executor.""")
#	_parser.add_argument( "--plugins", "-p",
#		default=None, metavar="PLUGINLIST",
#		help="""Plugins to execute EITHER in addition to OR instead of the
#		defaults listed in {}. The argument is a comma-separated
#		list of names. Each name must be EITHER the fully-qualified name of
#		a Python3 package implementing a plugin, OR the name of a file which
#		itself contains a list of plugins, one per line.
#		If %(metavar)s is prefixed with "+", the listed plugins AUGMENT
#		the defaults; otherwise the listed plugins REPLACE the defaults.
#		Note that a given plugin's dependencies will ALWAYS be added to
#		whatever you specify.
#		""".format( DEFAULT_PLUGIN_RCFILE ) )

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

	_parser.add_argument( "sources", nargs="+",
		help="""Files, directories and/or manifests to analyze. All three
		classes of input may be freely intermixed with the caveat that the
		names of manifests (files listing other files) should be prefixed
		by \"@\" on the command line.""" )

	_args = _parser.parse_args()

	# If inclusion or exclusion patterns were specified, just verify they're
	# valid regular expressions here. The result of compilation is discarded
	# now. This is just an opportunity for Python to report a regex format
	# exception before entering directory recursion.

	if _args.include:
		re.compile( _args.include )
	if _args.exclude:
		re.compile( _args.exclude )

	_main( _args )

