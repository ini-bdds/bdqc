
"""
Generates and writes data that simulates that produced by plugin analysis,
a 3-level JSON file of the form:

{
	"filename1":{
		"plugin1":{
			"stat1":...,
			"stat2":...,
			...
		},
		"plugin2":{
			"stat1":...,
			"stat2":...,
			...
		},
		...
	},
	"filename2":{
		...
	},
	...
}

More specifically, this script generates *controllably* random data subject
to a configuration also specified as JSON.

It should generate data to test the 5 canonical conditions:
0.	no amomalies
1.	incomparable files (different plugins ran)
2.	ambiguous types
3.	missing data outliers
4.	value outliers
	4a. quantitative
	4b.	qualitative

ok,incomparable,ambiguous,missing,quantitative,qualitative

1. qualitative (outliers)
	requires 1 NON-quantitative column
2. quantitative (outliers)
	requires 1 quantitative column
3. missing (outliers)
	requires at least one
4. ambiguous
	requires a column with one of
		(bool,int)
		(bool,float)
		(bool,string)
		(int,float)		special case: NOT AMBIGUOUS
		(int,string)
		(float,string)
5. incomparable
	requires at least two statistics
	DECIMATE

Randomizing:
number of plugins									[1-5]
number of input files								[1-1000]
per plugin:
	number of statistics							[1-5]
	types {boolean,int,float,string} of statistics	[randomly]
"""
import sys
import json

from random import random,randrange,choice,randint,normalvariate
from os import walk
from os.path import join,isdir
from copy import deepcopy

def _randomAlpha( length=5 ):
	return ''.join([ chr(0x61+randrange(26)) for i in range( 1+randrange(length) )])

def _randomName( length=5 ):
	"""
	Returns a random string of printable ASCII characters free of
	whitespace, but otherwise entirely random.
	"""
	return ''.join([ chr(0x21+randrange(95)) for i in range( 1+randrange(length) )])

def _randomStatistic( **args ):
	"""
	Quantitative
		float, int
	Categorical
		int,string,bool
	"""
	return choice(['Qf','Qi','Ci','Cs','Cb'])
	#return type(choice([True,1,1.0,"s"]))


class ScalarProxy(object):
	"""
	This class serves as a proxy for scalar data in the context of a
	Python representation of an (arbitrarily complex) JSON object.
	Specifically, it  holds an array of data one element of which is
	consumed each time the object is called. Thus (in combination with
	a JSONEncoder subclss) it allows a JSON object (tree) to be
	repeatedly serialized with different (precomputed) values for all
	the scalar (non-Object, non-Array) elements of the tree on each
	serialization.
	"""
	def __init__( self, sclass=None ):
		self.sclass = sclass

	def __call__( self ):
		value = self.queue[self.index]
		self.index += 1
		return value

	def __str__( self ):
		return self.sclass

	def initQueue( self, q ):
		self.queue = q
		self.index = 0

class ProxyEncoder(json.JSONEncoder):
	def default( self, obj ):
		if isinstance(obj,ScalarProxy):
			return obj()
		else:
			return json.JSONEncoder.default(self,obj)


def _randomTree_r( depth, prob, **args ):
	assert len(prob) == 2 and all([ 0 <= p and p < 1.0 for p in prob ])
	tree = dict()
	N = randint(2,3) # disallow objects with <2 keys
	while len(tree) < N:
		if depth < 2 and random() < prob[0]:
			subt = _randomTree_r( depth+1, (prob[0]/2.0,0.50), **args )
			if random() < prob[1]:
				item = subt
			else:
				item = []
				for x in range(randint(1,4)):
					item.append( deepcopy(subt) )
		else:
			item = ScalarProxy( _randomStatistic( **args ) )
		tree[_randomAlpha()] = item
	return tree


def _walk_immutable( data, debug=False ):
	"""
	Gather all the Proxies randomTree_r inserted to hold scalars.
	This is only feasible if deepcopy was used for array creation since,
	otherwise, Proxies will be multiply referenced.
	Moreover, randomTree_r cannot (in a straightforward way) maintain an
	accumulator for Proxies; it would have to accumulate after every deep
	copy.
	"""
	scalars = []
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
				if isinstance( v, ScalarProxy ):
					scalars.append(v)
					if debug:
						print( '/'.join( path+[k,] ), v, sep="\t" )
				else:
					assert isinstance(v,list) or isinstance(v,dict)
					push = ( k, i )
			except StopIteration:
				node = None
		else:
			assert isinstance(node,list) and isinstance(i,int)
			if i < len(node):
				v = node[i]
				if isinstance( v, ScalarProxy ):
					scalars.append(v)
					if debug:
						print( '/'.join( path+[str(i),] ), v, sep="\t" )
					i += 1
				else:
					assert isinstance(v,list) or isinstance(v,dict)
					push = ( str(i), i+1 )
			else:
				node = None
		# If v is further traversable, push the current node
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
	return scalars


def _walk_mutable( data ):
	"""
	Replace the None's randomTree_r inserted as placeholders for scalars
	with Proxies.
	"""
	path = []
	stack = []
	node = data
	i = 0
	while node:
		# Move forward in the current compound value (object or array)
		if i < len(node):
			k = i if isinstance(node,list) else sorted(node.keys())[i]
			v = node[k]
			if v is None:
				node[k] = v = ScalarProxy()
				print( '/'.join( path+[str(k),] ), v, sep="\t" )
				i += 1
			else:
				assert not isinstance(v,ScalarProxy)
				assert isinstance(v,list) or isinstance(v,dict)
				path.append( str(k) )
				stack.append( (node,i+1) )
				node,i = v,0
		elif stack:
			path.pop(-1)
			node,i = stack.pop(-1)
		else:
			break
	# end _walk_mutable

def genfilenames( root ):
	names = []
	for p,dl,fl in walk( root ):
		for f in fl:
			# Don't involve hidden files!
			if not f.startswith("."):
				names.append( join( p, f ) )
	return names

def main( args ):
	# 1. Create a set of filenames either by traversing a directory
	#    or generating random paths
	F = genfilenames(args.files) if isdir(args.files) else [ _randomAlpha(10) for i in range(int(args.files)) ]
	N = len(F)
	# 2. Create a random JSON object (hierarchy of data) like that
	#    which might be produced by BDQC from running an arbitrary
	#    set of plugins on input files.
	#    The 1st level is plugin-name, and descendent level(s) are
	#    statistics.
	J = dict()
	if args.plugins.isdigit():
		for i in range(int(args.plugins)):
			J[_randomAlpha(8)] = _randomTree_r( 0, (0.5,0.5) )
	else:
		for p in args.plugins.split(','):
			J[ p ] = _randomTree_r( 0, (0.5,0.5) )
	# Walk the tree collecting the ScalarProxies it contains.
	P = _walk_immutable( J )
	# Insure there are no multiply-referenced Proxies.
	assert len(frozenset([id(p) for p in P])) == len(P)
	# 3. Initialize the Proxies with "good" (anomaly-free) data
	for p in P:
		rng = range(N)
		if   p.sclass == "Qf":
			data = [ normalvariate(0,1.0) for i in rng ]
		elif p.sclass == "Qi":
			v = round(normalvariate(32,4)) 
			data = [ v for i in rng ]
		else:
			if   p.sclass == "Cs":
				K = _randomAlpha( 10 )
			elif p.sclass == "Ci":
				K = randint( 0, 10 )
			else:
				assert p.sclass == "Cb"
				K = choice([True,False])
			data = [ K for i in rng ]
		p.initQueue( data )
	# 4. TODO: Muck it up in the way specified by cmdline args.
	# 5. Emit it...either as a single JSON file or as lots of *.bdqc files.
	print("{")
	for i in range(N):
		print( '"{}":{}{}'.format(
				F[i],
				json.dumps( J, cls=ProxyEncoder, indent=4 ),
				',' if i+1 < N else '' ) )
	print("}")

if __name__ == "__main__":
	from argparse import ArgumentParser
	_parser = ArgumentParser(
		description="Slice one statistic out of (JSON-formatted) analysis output." )

	_parser.add_argument( "--plugins", "-p",
		default="2",
		type=str, # ...to accomodate the "plugin1,plugin2,..." case.
		help="""Either a count of plugins which will be randomly named
		or a comma-separated list of names to use for simulated plugins.
		(default: %(default)s)""" )
#_parser.add_argument( "--max_stats",
#		default=8,
#		help="Maximum number of statistics any one plugin produces" )
#	_parser.add_argument( "--missing-plugins", "-P",
#		type=int,
#		default=0,
#		help="Number of missing analyses (plugins). (Default: 0)" )
	_parser.add_argument( "--sim", "-s",
		type=str,
		default="",
		help="""Simulate a particular anomaly in the, otherwise normal,
		data. The argument should be a string containing digit-prefixed
		letters specifying the type(s) of anomalies to inject into the
		data. For example 1q2c
		(q)uantitative anomaly,
		(c)ategorical anomaly,
		(m)issing data anomaly,
		(i)ncomparable
		(a)mbiguous types

		The default is no anomalies.""" )
	_parser.add_argument( "--files", "-f",
		type=str,
		default="26",
		help="""Either a directory name to scan for files the names of
		which will be used in simulated output, or a count of filenames
		to generate randomly.
		(default: %(default)s)""" )
	#_parser.add_argument( "config",
		#	nargs=1,
		#help=""" """ )
	main( _parser.parse_args() )

