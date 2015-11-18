"""
1. Identify (by index) outliers in a 1-column file of quantitative values
	using (for now) an external executable, and
2. Create a series of density plots each with a rug of the data points
	with a single outlier highlighted.
"""

import array
import bdqc.statistic
import bdqc.builtin.compiled


class Analysis(object):

	def __init__( self, data, **args ):
		"""
		Returns a tuple of (index,plotname) tuples.
		The plotnames refer to temporary files. The plots are all identical
		except that each in some way highlights one datapoint corresponding
		to the associated filename.
		"""
		values = list( filter( lambda x:x is not None, data ))
		desc = args.get('desc',bdqc.statistic.Descriptor.infer_class( values ))
		if desc.is_quantitative():
			array_data = array.array("d", values)
			lb,ub = bdqc.builtin.compiled.robust_bounds( array_data )
			self.outliers = [ i for i in range(len(data)) if data[i] < lb or data[i] > ub ]
			self.lb = lb
			self.ub = ub
			if len(self.outliers) > 0:
				self.density = bdqc.builtin.compiled.gaussian_kde( array_data )
			# else, density won't be needed.
			#for p in self.density:
			#	print( "{:.3f}\t{:.3f}".format(*p) )
		elif desc.is_categorical():
			self.outliers = []
		elif desc.is_ordinal():
			self.outliers = []
		else:
			raise RuntimeError("unhandled statistical class"+desc)
		# self.outliers must be initialized

	def __del__( self ):
		pass

	def has_outliers( self ):
		return len(self.outliers) > 0

	def outlier_indices( self ):
		"""
		Returns the 0-based indices of all the datapoints that were
		identified as outliers.
		"""
		return self.outliers

	def is_outlier( self, fnum ):
		return fnum in self.outliers

if __name__=="__main__":
	pass

