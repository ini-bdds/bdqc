
import pkgutil

class Summary(object):

	def __init__( self, status, msg, body, rows, cols ):
		self.status = status
		self.msg  = msg
		self.body = body
		self.rows = rows
		self.cols = cols


	def render_html( self, fp ):
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

	def render_text( self, fp ):
		"""
		Render summary, including an incidence matrix, as ASCII.
		"""
		print( "Status:", self.msg, file=fp )
		print( "Incidence matrix:", file=fp )
		NR = len(self.body)
		NC = len(self.cols)
		assert NR == len(self.rows)
		wid = max([len(r) for r in self.rows])
		FMT = "{{0:>{}s}} {{1}}".format(wid)
		# Print headers
		# This will handle up to 9999 columns but only the 10's and 1's
		# values will be printed as column headers.
		CIX = [ "{:04d}".format(i+1) for i in range(0,NC) ]
		print( ' '*wid, ''.join([s[2] for s in CIX]), file=fp )
		print( ' '*wid, ''.join([s[3] for s in CIX]), file=fp )
		for rn in range(NR):
			print( FMT.format(
				self.rows[rn],
				''.join([ '+' if f else '-' for f in self.body[rn] ]) ),
				file=fp )
		print( "Column legend:", file=fp )
		for cn in range(NC):
			print( cn+1, self.cols[cn], sep="\t", file=fp )

if __name__=="__main__":
	import sys
	import bdqc.analysis
	s = Summary(
		bdqc.analysis.STATUS_VALUE_OUTLIERS,
		"whatever",
		[ [ 0, 1, 0 ],
		  [ 0, 0, 1 ],
		  [ 1, 0, 0 ],
		  [ 0, 1, 1 ] ],
		["r1","r2","r3","r4"],
		["c1","c2","c3"] )
	s.render_text( sys.stdout )	

