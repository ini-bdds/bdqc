"""
	Generate an entirely self-contained HTML report summarizing anomalous
files along with graphical evidence in the form of density or other
appropriate plots.

	The HTML contains two primary parts:
1. an incidence matrix representing the map
		anomaly( file, plugin/statistic ) => {Yes,No}
2. a plot area that displays the "evidence" that support flagging a file as
	anomalous.

python3 ../test/simdata.py config.json | tee sample-scan-c.json | python3 report.py > ~/c.html 

TODO:
1.	Consider how layout must change to handle possibly very large incidence
	matrices.
2.	This file is currently run as a module. Change it to be callable from
	the bdqc.scan module.
"""

import sys
import json
import pkgutil

import bdqc.statistic
import bdqc.univariate
import bdqc.incidence


def _extract( datadict, stat:"(plugin,statname) tuple" ):
	"""
	Extract vectors of values from the input data.
	Recall: input data is (after the below conversion to parallel lists)
	a list of plugin-dicts of statistic-dicts. That is, associated with
	each file is:
	{
		"plugin1-name": {
			'stat1': value,
			'stat2': value,
			...
		},
		"plugin2-name": {
			'stat1': value,
			'stat2': value,
			...
		},
		...
	}
	Here we make no assumptions of completeness. Specifically:
	1. any given plugin might be missing from any given file, and
	2. any given statistic might be missing from any given plugin.
	No exceptions should be thrown here; anything missing becomes None.
	"""
	return [ (d[stat[0]].get(stat[1],None) if \
			d.get(stat[0],None) is not None else None) for d in datadict ]


def _inline(fname):
	"""
	Copy an external file line-by-line to stdout.
	"""
	with open( fname ) as fp:
		for line in fp.readlines():
			print( line, end="" )

############################################################################
# 1. See what we have...
#	Convert the dict-of-dict's into a pair of parallel lists, one with the
#   file names and the other with the data dicts associated with the files.
#   Concurrently, enumerate the set of scalar statistics available.

_fname = [] # arbitrary order of enumeration of the JSON dict
_fdata = [] # list of dicts mapping plugin-name => stat-name => value
_stats = set()
for name,data in json.load( sys.stdin ).items():
	_fname.append( name )
	_fdata.append( data )
	for pname,pdata in data.items():      # PluginNAME, PluginDATA
		for sname,sdata in pdata.items(): # StatisticNAME, StatisticDATA
			if (sdata is not None) and \
				bdqc.statistic.Descriptor.scalar_type( sdata ):
				_stats.add( (pname,sname) )

_stats = sorted( list(_stats) ) # now grouped by plugin if weren't already

############################################################################
# 2. Analyze scalar data vectors
# 	Perform univariate outlier analysis for each statistic.
# 	The the univariate.Analysis constructor should not throw any exceptions
# related to data. Exception should only be raised for system troubles out
# of our control. The data, good bad and ugly, should be summarized in the
# returned Analysis object.
# 	Most importantly, each Analysis object should have a list of pairs
# containing (file_offset,tmpfile_name) giving the offset (with respect
# to _fname) and 

_analysis = [ bdqc.univariate.Analysis( _extract( _fdata, s ) ) for s in _stats ]

_anomalous_stats = list(filter( lambda i:_analysis[i].has_outliers(), range(len(_analysis)) ))
# ...a list of indices of _stats that exhibited outliers.

_anomalous_files = sorted(list(set().union(*[ a.outlier_indices() for a in _analysis ])))
# ...a list of indices of _fdata that exhibited outliers.

############################################################################
# 3. Generate HTML

print( """<!DOCTYPE html>
<html>
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<title>Analysis Report</title>
<style type="text/css" media="screen">
""" )
print( pkgutil.get_data("bdqc","report.css").decode('utf-8') )
print( """</style>
<!-- script type="application/x-javascript" src="d3.min.js"></script -->
<script src="https://cdnjs.cloudflare.com/ajax/libs/d3/3.5.6/d3.min.js" charset="utf-8"></script>
<script type="application/x-javascript">
""")

# A pair of parallel arrays in which major index corresponds to
# statistic
# outliers=[[x00,x01,...],[x10,x11,x12],...];
# density=[ [[x,y],[x,y],...], [[x,y],[x,y],...], ... ];
N = len(_anomalous_stats)
print( "var density=[" )
for si in range(N):
	print( "\t[", end="" )
	########
	print( "/*{}:{}*/".format( *_stats[_anomalous_stats[si]] ), end="" )
	coords = _analysis[ _anomalous_stats[si] ].density
	print( "[{:+.3e},{:+.3e}]".format( *coords[0] ),end="" )
	for xy in coords[1:]:
		print( ",[{:+.3e},{:+.3e}]".format(*xy),end="" )
	########
	print( "]", end="" )
	if si+1 < N:
		print(",")
print( "];" )

print( "var outlier=[" )
for si in range(N):
	print( "\t[", end="" )
	########
	print( "/*{}:{}*/".format( *_stats[_anomalous_stats[si]] ), end="" )
	d = _extract( _fdata, _stats[_anomalous_stats[si]] )
	print( "{:+.3e}".format(d[_anomalous_files[0]]), end="" )
	for fi in _anomalous_files[1:]:
		print( ",{:+.3e}".format(d[fi]), end="" )
	########
	print( "]", end="" )
	if si+1 < N:
		print(",")
print( "];" )
print( pkgutil.get_data("bdqc","report-d3.js").decode('utf-8') )
print( """</script>
</head>
<body onload="initDocument()">
""" )

############################################################################
# The incidence matrix that allows mouse hover to select plots for display
############################################################################

_im = bdqc.incidence.Matrix(
	[ [ "c{}_{}".format(r,c) for c in range(len(_anomalous_stats)) ] for r in range(len(_anomalous_files)) ],
	hl=[ [ _analysis[_anomalous_stats[c]].is_outlier(_anomalous_files[r]) for c in range(len(_anomalous_stats)) ] for r in range(len(_anomalous_files)) ],
	row_labels=[ _fname[fi] for fi in _anomalous_files ],
	column_labels=[ "{}:{}".format(*_stats[si]) for si in _anomalous_stats ]
	)

_im.render_html( sys.stdout )

print( '<hr>\n' )

print( """<div id="plots" style="position:relative;height:400px">
<!--It appears that absolutely-positioned content inside a div results in 0
	computed size; explicitly giving the height fixed it. -->
""" )

############################################################################
# Stacked plots
############################################################################

_TEMPLATE_SVG="""<svg id="s{STATINDEX}" width="320" height="240" style="position:absolute;top:0;left:0">
	<g id="g{STATINDEX}" transform="translate(0,240)"></g>
</svg>
"""

# Must maintain correspondence between:
# 1. table cells in incidence_matrix, and
# 2. img tags in the plot_div
# ...for which there are three options:
# 1. the same Python loop generates corresponding DOM nodes
# 2. two loops enumerate corresponding DOM nodes in same order
# 3. correspondence between DOM nodes is explicit by identifiers.
# I'm using #3.

for i in range(len(_anomalous_stats)):
	print( _TEMPLATE_SVG.format( STATINDEX=i ) )

############################################################################
# Javascript that manages plot visibility
############################################################################

print( """</div>
</body>
</html>
""" )

