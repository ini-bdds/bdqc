
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

import re

class Selector(object):
	"""
	Encapsulates a selector for one component of a data path. The selector
	is either a compiled regular expression object or a list of ranges.
	TODO: The conditional behavior in multiple methods suggests use of
	polymorphism might be preferable. Maybe override base class __new__
	to return appropriate subclass(?)
	"""
	@staticmethod
	def pmatch( selseq, subject ):
		"""
		"""
		assert isinstance( selseq, list )
		subseq = subject.split('/') if isinstance(subject,str) else subject
		try:
			for i in range(len(selseq)):
				sel = selseq[i]
				assert isinstance( sel, Selector )
				if not sel.match( subseq[i] ):
					return False
		except IndexError:
			return False
		return True

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
		parts = rngexp.split(',')
		try:
			for p in parts:
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
				self.spec = Selector._parse_range( spec[1:-1] )
			if not self.spec:
				self.spec = re.compile( spec )
				# Let re.error propagate up.

	def __str__( self ):
		if self.is_wild():
			return '*'
		elif isinstance( self.spec, list ):
			return str(self.spec)
		else:
			return '/'+self.spec.pattern+'/'

	def _in_range( self, value ):
		"""
		Indicates whether or not the <value> is in this Selector's
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


if __name__ == "__main__":
	import sys
	Xpath = [ Selector(p) for p in sys.argv[1].split('/') ]
	for p in Xpath:
		print( p )
	while len(sys.argv) > 2:
		subject = sys.argv.pop(2)
		print( "{} {}".format( subject, Selector.pmatch( Xpath, subject ) ) )

