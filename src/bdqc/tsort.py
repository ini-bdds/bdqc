
"""
Topological sort algorithm (Kahn (1962)).
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

My implementation below is motivated by function call dependencies, so
"an edge e from n to m" means "m depends on/calls n" and is represented
below as <n> in deps[<m>].
"""


def tsort( deps ):
	"""
	deps is a dict of direct dependencies represented as sets.
	If <a> depends on <b> (e.g. if function <a> "calls" function),
	then <b> is in the set edge[<a>].
	In other words every <key> in deps HAS dependencies, and those
	items in the value sets of deps that do not also appear as keys
	have no dependencies.
	This is equivalent in other terminology to "an edge from <b> to <a>.
	"""
	assert isinstance(deps,dict)
	assert all([ isinstance(v,set) for k,v in deps.items() ])
	order = []
	indep = set() # items with no dependencies
	# Find all items in the sets of dependencies that are NOT keys.
	# (In the following while loop, these would be keys with empty
	# value sets.)
	for dd in deps.values():
		for d in dd:
			if not d in deps: # d is not a key
				indep.add( d )
	while indep:
		n = indep.pop()
		order.append(n)
		# Take <n> out of all dependency lists.
		for k,v in deps.items():
			v.discard(n)
		# Any key that now has an empty set of values is an item with no
		# (further) dependencies. (It's dependencies have already been
		# incorporated into the order.)
		newly_indep = set([k for k,v in deps.items() if not v])
		# Add these items to the independent set...
		indep.update( newly_indep )
		# ...and remove them as keys from the deps dict.
		while newly_indep:
			k = newly_indep.pop()
			del deps[k]
	if deps:
		raise RuntimeError("Graph is not a DAG; a cycle exists.")
	return order

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
	for x in tsort( deps ):
		print( x )

# Following are test data intended to be stripped out and sent to the
# immediately preceding debug code using...
# grep '^##' tsort.py | cut -b 3- | python3 -O tsort.py
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
