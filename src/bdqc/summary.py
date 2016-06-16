
import pkgutil

class Summary(object):

	def __init__( self, status, msg, body, rows, cols ):
		self.status = status
		self.msg  = msg
		self.body = body
		self.rows = rows
		self.cols = cols

	def _data( self ):
		return '<script src="testdata.js" charset="utf-8"></script>'

	def render_html( self, fp ):
		"""
		Merge the 3 parts of the HTML page,
		1. the template HTML
		2. CSS stylesheet
		3. D3-based Javascript rendering code
		...along with the data to be rendered.
		The parts are kept separate to facilitate development--it's usually
		easier to edit the 3 file types independently, but we're aiming to
		provide the user with a *single* (nearly) self-contained HTML file
		for the report. If they want to copy the report elsewhere they
		should not have to copy multiple files.
		D3 will remain an external dependency, invisible to the user as
		long as they have a network connection.
		"""
		template = pkgutil.get_data("bdqc","template.html").decode("utf-8")
		css = pkgutil.get_data("bdqc","template.css").decode("utf-8")
		js = '<script>' + pkgutil.get_data("bdqc","render.js").decode("utf-8") + '</script>'
		report = template.replace(
				'@import url(template.css);\n',css).replace(
				'<script src="render.js" charset="utf-8"></script>\n',js).replace(
				'<script src="testdata.js" charset="utf-8"></script>\n',self._data() )
		print( report, file=fp )

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
	if len(sys.argv) > 1 and sys.argv[1] == "html":
		s.render_html( sys.stdout )
	else:
		s.render_text( sys.stdout )

