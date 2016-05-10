
from bdqc.statistic import Descriptor

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


def flatten( data:"JSON data represented as Python containers", is_terminal = lambda l,m:False ):
	"""
	Traverse a JSON object (encoded in Python containers), flattening it to
	yield a list of tuples of "atomic" data elements encoded as:
		( path, value, matrix-descriptor )
	...where the matrix-descriptor is optional.

	Scalars (ints, floats, booleans, and strings) are always atomic.
	JSON Objects (encoded as Python dicts) are NEVER considered atomic.
	JSON Arrays (encoded as Python lists), could in some circumstances
	be considered atomic. In particular, lists that are interpretable as
	matrices or sets could be considered atomic, so the is_terminal callback
	is provided so that the caller can make this decision.

	The is_terminal callback is given the list and a descriptor of its
	structure similar to Matlab matrix descriptors.

	The root element (data) is always going to be a JSON Object (Python
	dict) mapping plugin names to the data each plugin produced.
	"""
	assert data and isinstance( data, dict ) # a non-empty dict
	# Using iteration rather than recursion.
	path = []
	stack = []
	accum = []
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
						if is_terminal( v, mt ):
							accum.append( ( pa, v, mt ) )
						else:
							push = ( k, i )
					else:
						assert v is None or _is_scalar(v)
						accum.append( ( pa, v ) )
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
						if is_terminal( v, mt ):
							accum.append( ( pa, v, mt ) )
							i += 1
						else:
							push = ( str(i), i+1 )
					else:
						assert v is None or _is_scalar(v)
						accum.append( ( pa, v ) )
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
	return accum


if __name__=="__main__":

	import sys
	import json

	while len(sys.argv) > 1:
		with open( sys.argv.pop(1) ) as fp:
			data = flatten( json.load( fp ) )
			for t in data:
				print( *t, sep="\t" )

