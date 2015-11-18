
class Matrix(object):
	"""
	Encapsulates generation of an HTML table representing an incidence
	matrix.
	Alternative input representations.
	Create an incidence matrix of {rows} X {columns} from either:
	1. explicit NxM table
	2. tagged lists
		name1: {'foo','baz',...}
		name2: {'bar','baz',...}
		name3: {'foo','bar',...}
	3.	incidence bitstrings with key
		name1: 0xa8b889c
		name2: 0xb988194
		...
		key [ 'foo','bar','baz','bozzle',... ]
	"""
	def __init__( self, body, row_major=True, **args ):
		self.body = body
		# Insure body is complete, not a ragged array.
		assert all([ len(r)==len(body[0]) for r in body[1:] ])
		if 'row_labels' in args:
			self.row_labels = args['row_labels']
		else:
			self.row_labels = [ str(i+1) for i in range(len(body)) ]
		if 'column_labels' in args:
			self.column_labels = args['column_labels']
		else:
			self.column_labels = [ str(i+1) for i in range(len(body[0])) ]
		if 'hl' in args:
			self.hl = args['hl']

	def render_html( self, fp ):
		"""
		Emit HTML5 markup representing an incidence matrix.
		"""
		# The prelude
		print( '<table id="incidence_matrix">\n<caption></caption>\n', file=fp )
		# The header
		print( '<thead class="im">\n<tr>\n<th></th>\n', file=fp )
		for label in self.column_labels:
			print( '<th scope="col" class="im">{}</th>\n'.format(label), file=fp )
		print( '</tr>\n</thead>\n', file=fp )
		# The body
		print( '<tbody class="im">\n', file=fp)
		for ro in range(len(self.body)):
			print( '<tr>\n<th scope="row" class="im">{}</th>\n'.format( self.row_labels[ro] ), file=fp )
			row = self.body[ro]
			for co in range(len(row)):
				color = "red" if self.hl[ro][co] else "white"
				print( '<td class="im" id="{}" style="background:{}"></td>'.format( self.body[ro][co], color ), file=fp )
			print( '</tr>\n', file=fp)
		# The trailer
		print( ' </tbody>\n<tfoot class="im">\n</tfoot>\n</table>', file=fp )

# Unit test
if __name__=="__main__":
	import random
	import sys

