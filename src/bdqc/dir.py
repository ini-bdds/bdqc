

from os import listdir
from os.path import join,isdir
from re import match

def walk( root, depth=None, include=None, exclude=None ):
	"""
	A slightly more capable directory tree walker than that provided
	by os.walk. This provides for limiting depth of recursion and
	filtering filenames with a regular expression.
	"""
	files = []
	stack = [ root, ]
	while stack and ( not depth or len(stack) <= depth ):
		d = stack.pop()
		subs = []
		for e in listdir( d ):
			c = join( d, e )
			if isdir( c ):
				subs.append( c )
			elif ((not include) or match( include, c)) \
				 and ((not exclude) or not match( exclude, c)):
				files.append( c )
		stack.extend( subs )
	return files

if __name__=="__main__":
	import sys
	for f in walk(sys.argv[1],
			None if len(sys.argv) < 3 else int(sys.argv[2]),
			None if len(sys.argv) < 4 else     sys.argv[3] ):
		print( f )

