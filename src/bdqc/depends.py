
"""
This module essentially implements the topological sort
algorithm (Kahn (1962):

L <= Empty list that will contain the sorted elements
S <= Set of all nodes with no incoming edges
while S is non-empty do
    remove a node n from S
    add n to tail of L
    for each node m with an edge e from n to m do
        remove edge e from the graph
        if m has no other incoming edges then
            insert m into S
if graph has edges then
    return error (graph has at least one cycle)
else 
    return L (a topologically sorted order)


////////////////////////////////////////////////////////////////////////////
                   It strives for *clarity*, not speed.
////////////////////////////////////////////////////////////////////////////
"""

# I want to be able to test objects with isinstance(v,Iterable), but
# collections.abc was only introduced in 3.3.
#from collections.abc import Iterable

# An item has no dependencies if:
# 1. it doesn't even appear as a key, or
# 2. it is a key with an empty value set.

# 0. Remove all independent nodes.
#    As long as there are no cycles this will render some remaining
#    nodes newly-independent.
# 1. Identify all nodes 
# 
# Find an item with no dependencies

class Dependencies(object):

	def __init__( self, spec ):
		"""
		spec could take a variety of forms:
		A dictionary:
		    dict { k1; iterable( deps ), k2: iterable( deps ) }
		A iterable set of pairs
		iterable ( (k1,(deps)), (k2,(deps)), (k3,(deps)) )
		Others?
		This constructor's primary purpose is translating the variety of
		input possibilities into a dictionary of set-valued keys.
		Currently the input must be a dictionary of iterables.
		TODO: Eventually handle more input possibilities.
		"""
		# Convert whatever caller has provided into a dict
		# of set-values keys; for now throw up if argument is not
		# iterablerovided.
		if not isinstance(spec,dict) or \
			not all([ getattr(v,'__len__',False) for v in spec.values() ]):
			raise NotImplemented("need a dict of iterable values")
			# Note I'm using existence of the length attribute as an
			# indicator of iterability...not necessarily ideal(?)
		self.dd = dict( ((k,set(v)) for k,v in spec.items()) )


	def __bool__( self ):
		"""
		False when its wrapped dict is false.
		"""
		return bool(self.dd)


	def nodes( self ):
		"""
		Generates the Python set of all nodes in self.
		"""
		n = set()
		for k,v in self.dd.items():
			n.add(k)
			n.update(v)
		return n


	def is_indie( self, n ):
		"""
		Determines whether n is independent (has no dependencies) in self.
		The second clause handles all cases:
		bool(None) == False, bool([]) == False, etc.
		"""
		return (not n in self.dd) or (not self.dd[n])


	def cull_independent_nodes( self ):
		"""
		Produce a new Dependencies object by removing all independent nodes
		from self, and return the pair ( indie_nodes, Dependencies ).
		"""
		indie = frozenset( filter( lambda n:self.is_indie(n),self.nodes()) )
		keys = filter( lambda k: not k in indie, self.dd.keys() )
		return (
			indie,
			Dependencies( dict(( (k,self.dd[k]-indie) for k in keys )) ) )


	def tsort( self ):
		"""
		Recursively remove independent nodes from Dependencies (generating
		a sequence of progressively smaller Dependencies) until either the
		resulting Dependencies object at some iteration is empty (success),
		or the resulting independent nodes at some stage is empty (failure).
		"""
		topo_order = []
		deps = self
		while deps:
			indie, deps = deps.cull_independent_nodes()
			if not indie:
				raise RuntimeError("Graph is not a DAG; a cycle exists.")
			topo_order.extend( indie )
		return topo_order


if __name__ == "__main__":
	import sys
	fp = sys.stdin
	line = fp.readline()
	deps = dict()
	while len(line) > 0:
		f = line.rstrip().split()
		try:
			deps[f[0]].update( set(f[1:]) )
		except KeyError:
			deps[f[0]] = set(f[1:])
		line = fp.readline()
	for x in Dependencies( deps ).tsort():
		print( x )

# Following are test data intended to be stripped out and sent to the
# immediately preceding debug code using...
# grep '^##' depends.py | cut -b 3- | python3 -O tsort.py
##main parse_options
##main tail_file
##main tail_forever
##tail_file pretty_name
##tail_file write_header
##tail_file tail
##tail_forever recheck
##tail_forever pretty_name
##tail_forever write_header
##tail_forever dump_remainder
##tail tail_lines
##tail tail_bytes
##tail_lines start_lines
##tail_lines dump_remainder
##tail_lines file_lines
##tail_lines pipe_lines
##tail_bytes xlseek
##tail_bytes start_bytes
##tail_bytes dump_remainder
##tail_bytes pipe_bytes
##file_lines dump_remainder
##recheck pretty_name
