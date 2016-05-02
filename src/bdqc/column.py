
import array

import bdqc.builtin.compiled

# TODO: Investigate what makes sense for this.
# It relies on the performance of the median couple.
MAX_CATEGORICAL_INT_CARDINALITY = 26

class Vector(object):
	"""
	An accumulator for column data acquired incrementally.
	In the context that motivated this class, most columns are expected to
	contain either:
	1. exactly 1 non-quantitative (though possibly still numeric) value
	2. quantitative--that is, floating-point--data
	"""
	def __init__( self, placeholder_count=0 ):
		"""
		This cache's initial state is equivalent to that which would result
		from <placeholder_count> invocations of self.push( None ).
		"""
		self.last  = None
		self.count   = placeholder_count
		self.missing = placeholder_count
		# The counts in self.types are only valid after _flush.
		self.types = {'b':0,'i':0,'f':0,'s':0}
		# Deferred attributes:
		# self.data  = []
		# self.value_histogram
		# self.outlier
		# self.lb
		# self.ub
		# self.density

	def __str__( self ):
		types = "b:{b},i:{i},f:{f},s:{s}".format( **self.types )
		return "len {}, list-len {}, missing {}, types {}".format(
				self.count,
				len(self.data) if hasattr(self,"data") else 0,
				self.missing,
				types )

	def __len__( self ):
		return self.count

	def __getitem__( self, index ):
		assert isinstance(index,int) # not supporting slices
		if not ( index < len(self) ):
			raise IndexError()
		return self.data[index] if hasattr(self,"data") else self.last

	def _incTypeCount( self, value ):
		if value is None:
			self.missing += 1
		else:
			if not ( isinstance(value,float) \
				or isinstance(value,int) \
				or isinstance(value,str) \
				or isinstance(value,bool) ):
				raise TypeError( "unexpected type: " + type(value).__name__ )
			self.types[ type(value).__name__[0] ] += 1

	def _non_missing_len( self ):
		return self.count - self.missing

	def _is_numeric( self ):
		"""
		...if int and float items account for all non-missing items.
		Note that in the context of this code "quantitative" implies
		"numeric", but "numeric" does NOT imply "quantitative".
		"""
		return self._non_missing_len() == (self.types['i'] + self.types['f'])

	def _is_quantitative( self ):
		"""
		If *any* cached values were floating-point, the entire column will
		be treated as floating-point. That is, integers will be coerced.
		This is to accomodate the possibility that floating-point data that
		happens to occasionally have integral values may be "look like"
		and be interpreted as integral data by Python.

		If all non-missing values were integral AND the cardinality of the
		values is sufficiently large (to preclude interpretation as
		categorical data) the column will be treated as quantitative for the
		purpose of outlier detection.
		"""
		if self.types['f'] > 0:      # Presence of floats implies...
			return True              # ...quantitative, and...
		elif not self._is_numeric(): # ...complete absence of numbers...
			return False             # ...precludes quantitative,...
		else: # ...otherwise, we need the cardinality of integral values.
			assert all([ v is None or isinstance(v,int) for v in iter(self) ])
			cardinality = self._make_value_histogram( MAX_CATEGORICAL_INT_CARDINALITY+1 )
			return cardinality > MAX_CATEGORICAL_INT_CARDINALITY

	def _make_value_histogram( self, min_card=0 ):
		"""
		Create dict mapping values to their cardinality in this Vector.

		If min_card is non-zero, caller is only interested in whether or
		not the cardinality of distinct values is >= min_card. In this
		case, we may exit before finishing enumeration of the data and,
		thus, before finishing computation of the histogram.

		If min_card is zero, the histogram is fully computed and left in
		self.
		"""
		self.value_histogram = dict()
		for v in iter(self):
			if v is not None:
				try:
					self.value_histogram[ v ] += 1
				except KeyError:
					self.value_histogram[ v ]  = 1
			if min_card > 0 and len(self.value_histogram) >= min_card:
				# Since the histogram is incomplete, if we need it later
				# we'll have to recomputed it. So don't leave it around.
				delattr(self,"value_histogram")
				return min_card
		return len(self.value_histogram)

	def push( self, value ):
		"""
		Counts identical values until a second distinct value is observed.
		When a second distinct value is observed, create the list.
		Subsequently, all values are appended directly to the list.
		"""
		self._incTypeCount(value)
		if hasattr(self,"data"):
			self.data.append( value )
		elif self.count > 0:
			# Note that Python will promote int to float as needed so
			# observation of 1.0 and int(1) will NOT trigger flush.
			if self.last != value: # need to "flush"
				self.data = [ self.last, ]*len(self)
				self.data.append( value )
		else:
			self.last = value
		self.count += 1

	def pad_to_count( self, expected_count ):
		"""
		This method insures that missing data is accounted for.
		"""
		if len(self) < expected_count:
			self.push( None )
			return True
		return False

	def is_missing_data( self ):
		return self.missing > 0

	def is_uniquely_typed( self ):
		"""
		Indicates whether or this cache contains exactly one of boolean,
		string, or numeric data. Missing data is ignored.

		Note that integral and floating-point are treated as a single type
		for the purposes of this method, so this method treats that as a (last resort) special case.
		More specifically, if any
		floating-point values are present, integral values will be treated
		as floating-point.
		"""
		return not hasattr(self,"data") \
			or sum([ int(v > 0) for v in self.types.values()]) == 1 \
			or self._is_numeric()

	def is_single_valued( self ):
		"""
		Determine whether or not this column is *effectively* single-valued.

		By "effectively" we mean that EITHER:
		1. only a single value (of any type) was observed, OR
		2. all (non-missing) values were numeric and they were sufficiently
			centrally-distributed without outliers.

		This method computes as little as possible to arrive at an answer.
		However, if the data is quantitative, the answer actually requires
		determination of the presence, and thus the identities, of outliers.
		"""
		if not hasattr(self,"data"): # A list was never created because...
			return True    # ...only one value was ever seen. Case closed!
		if self._is_quantitative():
			# Filter missing values and force floating-point types.
			data = [ float(v) for v in filter( lambda x:x is not None, iter(self)) ]
			array_data = array.array("d", data )
			lb,ub = bdqc.builtin.compiled.robust_bounds( array_data )
			self.outlier = [ i for i in range(len(data)) if
				(data[i] is not None) 
				and (data[i] < lb or data[i] > ub) ]
			self.lb = lb
			self.ub = ub
			self.density = bdqc.builtin.compiled.gaussian_kde( array_data )
			return len(self.outlier) == 0

		# Computation within _is_quantitative can (but does not always) yield
		# a partial or complete cardinality assessment as a side effect...

		if not hasattr(self,"value_histogram"):
			self._make_value_histogram()
		else:
			# If it's *not* quantitative, but a histogram exists, then the
			# data *was* exhaustively enumerated...
			assert sum(self.value_histogram.values()) == self._non_missing_len()
			pass

		return len(self.value_histogram) == 1

	def missing_indices( self ):
		"""
		Return a list of the indices of elements with missing data (None).
		Incidentally, this list is guaranteed to be sorted.
		I'm tuple'ing it so that it can be hashed, and multiple columns'
		lists easily compared for equality.
		"""
		return tuple([ i for i in range(len(self)) if self[i] is None ])

	def present_indices( self ):
		"""
		Return a list of the indices of elements with NON-MISSING data.
		This is the complement of the set returned by missing_indices.
		"""
		return tuple([ i for i in range(len(self)) if self[i] is not None ])

	def minor_type_indices( self ):
		"""
		Return a list of the indices of elements with types in the minority.
		"""
		MINOR_TYPE_COUNT = min( self.types.values() )
		MINOR_TYPES = frozenset([ k for k in self.types.keys()
			if self.types[k] == MINOR_TYPE_COUNT ])
		return tuple([ i for i in range(len(self)) if type(self[i]).__name__[0] in MINOR_TYPES ])

	def outlier_indices( self ):
		"""
		Return a list of the indices of outlier elements.

		The "outlier elements": 
		1. in quantitative data are the outliers.
		2. in non-quantitative data are whichever elements have the
			value of smallest cardinality (the minority).

		This method should only be called if this Vector is known to contain
		multiple values. (Otherwise, state assumed to exist won't!)

		In the case of categorical data, this method implements a broad
		definition of "outlier": all files having any of the possibly
		multiple different values of minimum cardinality among all values
		are considered outlier. Ideally, the column is binary valued and
		this definition simply returns the files (their indices) with the
		(unique) minority value.
		"""
		assert hasattr(self,"outlier") \
			or hasattr(self,"value_histogram")
		# ...otherwise, if neither of these exist, we could just make the
		# tail of this method conditional on "not is_single_valued" which
		# would trigger their just-in-time generation...but I'm currently
		# assuming that has been called ealier, so one of these exists.
		if not hasattr(self,"outlier"):
			min_cardinality = min( self.value_histogram.values() )
			minority_values = frozenset( filter(
				lambda k:self.value_histogram[k] == min_cardinality,
				self.value_histogram.keys() ) )
			self.outlier = tuple([ i for i in range(len(self))
				if self[i] in minority_values ])
		return self.outlier


# Unit test
if __name__=="__main__":
	import sys
	vec = Vector( int(sys.argv[1]) if len(sys.argv) > 1 else 0 )
	# Read input...
	line = sys.stdin.readline()
	while len(line) > 0:
		if line.startswith("\n"):
			vec.push( None )
		else:
			vec.push( eval(line.rstrip()) )
		line = sys.stdin.readline()
	# ...then evaluate in the same way analysis.Matrix will.
	print( vec )
	if vec.is_missing_data():
		print( "missing data" )
	elif not vec.is_uniquely_typed():
		print( "ambiguous type" )
	elif vec.is_single_valued():
		print( "single-valued" )
	else: # "outliers" exist. Name them!
		for i in vec.outlier_indices():
			print( i, vec[i] )

