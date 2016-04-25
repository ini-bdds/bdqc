
import pkgutil
import bdqc.analysis

class HTML(object):
	"""
	TODO: lots
	"""
	def __init__( self, source ):
		self.source = source

	def _render_incidence_matrix( self, fp ):
		"""
		Emit HTML5 markup representing an incidence matrix.
		"""
		assert hasattr(self.source,"anom_col") \
			and isinstance(self.source.anom_col,list) \
			and all([ isinstance(i,str) for i in self.source.anom_col])

		body = [ [ "c{}_{}".format(r,c)
			for c in range(len(self.source.anom_col)) ]
			for r in range(len(self.source.anom_row)) ]
		hl   = [ [ fi in self.source.column[k].outlier_indices()
			for k  in self.source.anom_col ]
			for fi in self.source.anom_row ]
		row_labels = [ self.source.files[fi] for fi in self.source.anom_row ]
		# The prelude
		print( '<table id="incidence_matrix">\n<caption></caption>\n', file=fp )
		# The header
		print( '<thead class="im">\n<tr>\n<th></th>\n', file=fp )
		for label in self.source.anom_col:
			print( '<th scope="col" class="im">{}</th>\n'.format(label), file=fp )
		print( '</tr>\n</thead>\n', file=fp )
		# The body
		print( '<tbody class="im">\n', file=fp)
		for ro in range(len(body)):
			print( '<tr>\n<th scope="row" class="im">{}</th>\n'.format( row_labels[ro] ), file=fp )
			row = body[ro]
			for co in range(len(row)):
				color = "red" if hl[ro][co] else "white"
				print( '<td class="im" id="{}" style="background:{}"></td>'.format( body[ro][co], color ), file=fp )
			print( '</tr>\n', file=fp)
		# The trailer
		print( ' </tbody>\n<tfoot class="im">\n</tfoot>\n</table>', file=fp )

	def render( self, fp ):
		assert hasattr(self.source,"status")
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
		<script src="https://cdnjs.cloudflare.com/ajax/libs/d3/3.5.6/d3.min.js"
			charset="utf-8"></script>
		<script type="application/x-javascript">
		""")

		# A pair of parallel arrays in which major index corresponds to
		# statistic
		# outliers=[[x00,x01,...],[x10,x11,x12],...];
		# density=[ [[x,y],[x,y],...], [[x,y],[x,y],...], ... ];
		N = len(self.source.anom_col)
		print( "var density=[" )
		for si in range(N):
			k = self.source.anom_col[si]
			print( "\t[", end="" )
			########
			print( "/*",k,"*/", sep="", end="" )
			if hasattr(self.source.column[k],"density"):
				coords = self.source.column[k].density
				print( "[{:+.3e},{:+.3e}]".format( *coords[0] ),end="" )
				for xy in coords[1:]:
					print( ",[{:+.3e},{:+.3e}]".format(*xy),end="" )
			else: # print a dummy list for now
				print( "[0,0]",end="" )
				for i in range(1,10):
					print( ",[{},0]".format( i ),end="" )
			########
			print( "]", end="" )
			if si+1 < N:
				print(",")
		print( "];" )

		print( "var outlier=[" )
		for si in range(N):
			k = self.source.anom_col[si]
			print( "\t[", end="" )
			########
			print( "/*",k,"*/", sep="", end="" )
			if hasattr(self.source.column[k],"density"):
				print( "{:+.3e}".format(self.source.column[k][0], end="" ) )
				for fi in self.source.anom_row[1:]:
					print( ",{:+.3e}".format(self.source.column[k][fi]), end="" )
			else:
				pass
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

		self._render_incidence_matrix( fp )

		####################################################################
		# ...allowing mouse hover to select plots for display
		####################################################################

		print( '<hr>\n' )

		print( """<div id="plots" style="position:relative;height:400px">
		<!--It appears that absolutely-positioned content inside a div results in 0
			computed size; explicitly giving the height fixed it. -->
		""" )

		####################################################################
		# Stacked plots
		####################################################################

		# Must maintain correspondence between:
		# 1. table cells in incidence_matrix, and
		# 2. img tags in the plot_div
		# ...for which there are three options:
		# 1. the same Python loop generates corresponding DOM nodes
		# 2. two loops enumerate corresponding DOM nodes in same order
		# 3. correspondence between DOM nodes is explicit by identifiers.
		# I'm using #3.

		for i in range(len(self.source.anom_col)):
			print(
				"""<svg id="s{STATINDEX}" width="320" height="240"
					style="position:absolute;top:0;left:0">
					<g id="g{STATINDEX}" transform="translate(0,240)"></g>
				</svg>
				""".format( STATINDEX=i ) )

		print( """</div>
		</body>
		</html>
		""" )


class Plaintext(object):
	"""
	TODO: lots
	"""

	def __init__( self, source ):
		self.source = source

	def _renderMissingValuesReport( self, fp ):
		print(
"""
The incidence matrix shows (file,statistic) pairs with present (+) or
missing (-) values.
These typically arise when different sets of plugins were run on different
files (or one or more plugins ran differently on different files).
Either of these cases implies some discrepancy between input files, but
in this case we cannot (usually) say which are the anomalous files.
""" )

	def _renderMultipleTypesReport( self, fp ):
		print(
"""
The incidence matrix shows (file,statistic) pairs for statistics for which
a single type could not be inferred. (The files in these pairs are those
with the minority type.)
This usually implies either software bugs or poorly designed plugins.
""" )

	def _renderAnomaliesReport( self, fp ):
		print(
"""
The incidence matrix shows (file,statistic) pairs for statistics which
exhibited either QUANTITATIVE OUTLIERS or otherwise NON-CONSTANT VALUES
(in the case of non-quantitative data). These are likely anomalies.
""" )

	@staticmethod
	def _render_im( im, fp ):
		print( "Incidence matrix:", file=fp )
		NR = len(im['body'])
		NC = len(im['cols'])
		assert NR == len(im['rows'])
		wid = max([len(r) for r in im['rows']])
		FMT = "{{0:>{}s}} {{1}}".format(wid)
		# Print headers
		# This will handle up to 9999 columns but only the 10's and 1's
		# values will be printed as column headers.
		CIX = [ "{:04d}".format(i+1) for i in range(0,NC) ]
		print( ' '*wid, ''.join([s[2] for s in CIX]) )
		print( ' '*wid, ''.join([s[3] for s in CIX]) )
		for rn in range(NR):
			print( FMT.format(
				im['rows'][rn],
				''.join([ '+' if f else '-' for f in im['body'][rn] ]) ),
				file=fp )
		print( "Column legend:", file=fp )
		for cn in range(NC):
			print( cn+1, im['cols'][cn], sep="\t", file=fp )

	def render( self, fp ):
		STATUS = self.source.status
		if STATUS == bdqc.analysis.STATUS_NOTHING_TO_SEE:
			print( "No anomalies detected.", file=fp )
			return

		im = self.source.incidence_matrix()
		Plaintext._render_im( im, fp )

		if STATUS == bdqc.analysis.STATUS_MISSING_VALUES:
			self._renderMissingValuesReport( fp )
		elif STATUS == bdqc.analysis.STATUS_MULTIPLE_TYPES:
			self._renderMultipleTypesReport( fp )
		else:
			assert STATUS == bdqc.analysis.STATUS_ANOMALIES
			self._renderAnomaliesReport( fp )

