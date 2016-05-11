
"""
Plugins' JSON-format summaries are flattened in BDQC's 2nd stage analysis
into a matrix in which columns are named by the *path* through compound
JSON data types to a statistic.

This module contains utilities for selecting paths similar to XPath in XML.

Syntax definition of a path selector:

wildcard          := '*'
regex             := any Python-legal regular expression
range-expr-part   := <integer> ( '-' <integer )?
range-expr        := '[' range-expr-part ( ',' range-expr-part )* ']'
selector          := { wildcard | range-expr | regex }
selector-sequence := selector ( '/' selector )*

Examples:
	*
	foo/*
	foo/[3-9,5]/baz
	foo/[3-9,5]/baz/*
	foo/[3-9,5]/baz/[3]/*
"""

from os.path import isfile
import re

class _Component(object):
	"""
	Encapsulates a selector for one component of a data path. The selector
	is either a compiled regular expression object or a list of ranges.
	TODO: The conditional behavior in multiple methods suggests use of
	polymorphism might be preferable. Maybe override base class __new__
	to return appropriate subclass(?)
	"""

	@staticmethod
	def _parse_range( rngexp ):
		"""
		Speculatively try to parse rngexp assuming it is a range expression
		(defined below). Failure because of non-integer subexpression(s) is
		not an error; rather it merely invalidates the assumption, and we
		return None. Failure for any other reason is an exception.

		range_exp := '[' rng(,rng)* ']'
		rng := <int> ( '-' <int> )?
		"""
		assert isinstance( rngexp, str ) 
		ranges = []
		part = rngexp.split(',')
		try:
			for p in part:
				r = tuple([ int(v) for v in p.split('-') ])
				ranges.append( r )
				if len(r) != 1 and (len(r) != 2 or r[0] > r[1]):
					raise RuntimeError("bad range component:"+p)
		except ValueError:
			# We assume if it's not interpretable as a sequence of integer
			# ranges then it's not *intended* to be one.
			if __debug__:
				print( p )
			return None
		return ranges

	def __init__( self, spec ):
		assert isinstance( spec, str ) and len(spec) > 0
		self.spec = None # implies wildcard
		if spec != '*':
			if spec.startswith('[') and spec.endswith(']'):
				self.spec = _Component._parse_range( spec[1:-1] )
			if not self.spec:
				self.spec = re.compile( spec )
				# Let re.error propagate up.

	def __str__( self ):
		if self.is_wild():
			return '*'
		elif isinstance( self.spec, list ):
			return "[{}]".format( ','.join(
				[ "{}-{}".format(*el) if len(el) == 2 else str(el[0]) for el in self.spec]) )
		else:
			return self.spec.pattern

	def _in_range( self, value ):
		"""
		Indicates whether or not the <value> is in this _Component's
		numeric range.
		"""
		assert isinstance(value,int)
		assert isinstance(self.spec,list)
		for rng in self.spec:
			if len(rng) == 1:
				if value == rng[0]:
					return True
			else:
				if rng[0] <= value and value <= rng[1]:
					return True
		return False

	def is_wild( self ):
		return self.spec is None

	def match( self, s:"a path component" ):
		"""
		<s> should not contain any path component separator.
		"""
		assert s.find('/') < 0
		if self.is_wild():
			return True
		elif isinstance(self.spec,list):
			try:
				return self._in_range( int(s) )
			except ValueError:
				return False
		else: # a regular expression match
			m = self.spec.match( s )
			return m and m.group() == s


class Selector(object):
	"""
	Encapsulates a list of statpath._Components and the means to
	match them.
	"""

	def __init__( self, path ):
		self.part = [ _Component(p) for p in path.split('/') ]

	def __str__( self ):
		return '/'.join([ str(part) for part in self.part ])

	def __call__( self, subject ):
		"""
		"""
		subseq = subject.split('/') if isinstance(subject,str) else subject
		try:
			for i in range(len(self.part)):
				if not self.part[i].match( subseq[i] ):
					return False
		except IndexError:
			return False
		return True

	def __getitem__( self, index ):
		assert isinstance(index,int)
		return self.part[index]


def selectors( source:"a literal string, filename, or list of strings" ):
	"""
	Create a list of Selector from a source. The source is assumed to
	be either:
	1. a list of strings each specifying a Selector
	2. the name of a file containing a list of selectors, one per line.
	3. a literal string containing a semi-colon-separated list of selector
	   specs
	This is just a convenience function.
	"""
	slist = None
	if isinstance(source,list):
		slist = [ Selector(s) for s in source ]
	elif isfile( source ):
		with open( source ) as fp:
			slist = [ Selector(line.rstrip()) for line in fp.readlines() ]
	else:
	  	assert isinstance(source,str)
		slist = [ Selector(substring) for substring in source.split(';') ]
	return slist


if __name__ == "__main__":
	import sys
	if len(sys.argv) < 2:
		print( sys.argv[0], "<pattern>", "<path1> [ <path2> ... ]" )
		sys.exit(0)
	sel = Selector( sys.argv[1] )
	print( "reconstructed:", sel )
	for part in sel:
		print( part )
	while len(sys.argv) > 2:
		subject = sys.argv.pop(2)
		print( "{} {}".format( subject, sel( subject ) ) )

