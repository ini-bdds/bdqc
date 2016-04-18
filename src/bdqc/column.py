
import tempfile
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
	1. exactly 1 non-quantitative value (though it may be numeric)
	2. quantitative--that is, floating-point--data
	Holds a vector of data in a tempfile if and only if at least two
	distinct values are pushed.
	Writing to (and even opening) the file is deferred until two distinct
	values are witnessed.
	"""
	def __init__( self, placeholder_count=0 ):
		"""
		This cache's initial state is equivalent to that which would result
		from <placeholder_count> invocations of self.__call__( None ).
		"""
		self.store = None
		self.last  = None
		self.count   = placeholder_count
		self.missing = placeholder_count
		# The counts in self.types are only valid after _flush.
		self.types = {'b':0,'i':0,'f':0,'s':0}
		# deferred attributes
		# self._data
		# self.value_histogram
		# self.aberrant
		# self.lb
		# self.ub
		# self.density

	def __del__( self ):
		if self.store:
			self.store.close()

	def __str__( self ):
		assert self.store is not None
		types = "b:{b},i:{i},f:{f},s:{s}".format( **self.types )
		return "{}/{}/{}".format( self.count, self.missing, types )

	def __len__( self ):
		return self.count

	def __getitem__( self, index ):
		assert isinstance(index,int)
		return self.data()[index]

	def __call__( self, value ):
		"""
		Just count identical values until a second distinct value is
		observed. When a second distinct value is observed, create the
		tempfile. Subsequently, all values are written directly to the
		file.
		"""
		self._incTypeCount(value)
		if self.store:
			self._write( value )
		elif self.count > 0:
			if self.last != value:
				self._flush()
				self._write( value )
		else:
			self.last = value
		self.count += 1

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

	def _write( self, value ):
		"""
		To insure that string values of "None" aren't confused with the
		None type, replace None by a more literal representation of None:
		an empty line.
		"""
		print( repr(value) if value is not None else "", file=self.store )

	def _flush( self ):
		"""
		"""
		assert self.store is None # because this should only be called once!
		self.store = tempfile.SpooledTemporaryFile( mode="w+" )
		for i in range(self.count):
			self._write( self.last )

	def data( self ):
		"""
		Idempotent--reload if and only if data hasn't been loaded.
		"""
		if not hasattr(self,"_data"):
			if self.store is None:
				# TODO: following can be optimized away with proper state maintenance
				self._data = [ self.last for i in range(self.count) ]
			else:
				assert self.store.tell() > 0
				self.store.seek(0)
				# Remember: None is represented as empty lines. See _write.
				self._data = [ eval(l.rstrip()) if len(l.rstrip()) > 0 else None
					for l in self.store.readlines() ]
				# We won't be using the tempfile again, so...
				self.store.close()
				self.store = None
		return self._data

	def _non_missing_len( self ):
		return self.count - self.missing

	def _make_value_histogram( self, min_card=0 ):
		"""
		Create dict mapping values to their cardinality within self.data().

		If min_card is non-zero, caller is only interested in whether or
		not the cardinality of distinct values is >= min_card. In this
		case, we may exit before finishing enumeration of the data and,
		thus, before finishing computation of the histogram.

		If min_card is zero, the histogram is fully computed and left in
		self.
		"""
		self.value_histogram = dict()
		for v in self.data():
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

	def add_missing_to_insure_count( self, expected_count ):
		"""
		This method insures that missing data is noted.
		If the last file processed by Matrix did not contain the
		statistic held by this cache, self.__call__ was not called and
		this cache's total count of statistics will be expected_count-1.
		"""
		if len(self) < expected_count:
			self( None )
			return True
		return False

	def is_numeric( self ):
		"""
		...if int and float items account for all non-missing items.
		Note that quantitative implies numeric, but numeric does NOT
		imply quantitative.
		"""
		return self._non_missing_len() == (self.types['i'] + self.types['f'])

	def is_quantitative( self ):
		"""
		If *any* cached values were floating-point, the entire column will
		be treated as floating-point. That is, integers will be coerced.
		This is to accomodate the possibility that floating-point data that
		happens to occasionally have integral values may be emitted by the
		plugins as integers.

		If all non-missing values were integral AND the cardinality of the
		values is sufficiently large the column will be treated as quanti-
		tative for the purpose of anomaly detection.
		"""
		
		if self.types['f'] > 0: # Presence of floats implies...
			return True  # ...quantitative, but...
		elif not self.is_numeric(): # ...complete absence of numbers...
			return False # ...precludes quantitative.
		else: # Otherwise, we need the cardinality of integral values.
			assert all([ v is None or isinstance(v,int)
				for v in self.data() ])
			cardinality = self._make_value_histogram( MAX_CATEGORICAL_INT_CARDINALITY+1 )
			return cardinality > MAX_CATEGORICAL_INT_CARDINALITY

	def is_missing_data( self ):
		"""
		Returns the count of missing data--that is, a count of values
		of None--in this cache.
		"""
		return self.missing > 0

	def is_uniquely_typed( self ):
		"""
		Indicates whether or this cache contains exactly one of boolean,
		string, or numeric data. Missing data is ignored.

		Note that integral and floating-point are treated as a single
		type for the purposes of this method. More specifically, if any
		floating-point values are present, integral values will be treated
		as floating-point.
		"""
		return self.store is None \
			or self.is_numeric() \
			or sum([ int(v > 0) for v in self.types.values()]) == 1

	def is_single_valued( self ):
		"""
		Determine whether or not this column is *effectively* single-valued.

		By "effectively" it is meant that EITHER:
		1. only a single value (of any type) was observed, OR
		2. all (non-missing) values were numeric and they were sufficiently
			centrally-distributed without outliers.

		This method computes as little as possible to arrive at an answer.
		However, if the data is quantitative, the answer actually requires
		determination of the presence of outliers, and thus the identities
		of the aberrant elements.
		"""
		if self.store is None: # self.last was never flushed because only...
			return True        # ...one value was ever seen. Case closed!
		if self.is_quantitative():
			# Filter missing values and force floating-point types.
			data = [ float(v) for v in filter( lambda x:x is not None, self.data()) ]
			array_data = array.array("d", data )
			lb,ub = bdqc.builtin.compiled.robust_bounds( array_data )
			self.aberrant = [ i for i in range(len(data)) if
				(data[i] is not None) 
				and (data[i] < lb or data[i] > ub) ]
			self.lb = lb
			self.ub = ub
			self.density = bdqc.builtin.compiled.gaussian_kde( array_data )
			return len(self.aberrant) == 0

		# Computation within is_quantitative can (but does not always) yield
		# a partial or complete cardinality assessment as a side effect...

		if not hasattr(self,"value_histogram"):
			self._make_value_histogram()
		else:
			# If it's *not* quantitative, but a histogram exists, then the
			# data *was* exhaustively enumerated...
			assert sum(self.value_histogram.values()) == self._non_missing_len()
			pass

		return len(self.value_histogram) == 1

	def aberrant_indices( self ):
		"""
		Return a list of the indices of aberrant elements.

		The "aberrant elements": 
		1. in quantitative data are the outliers.
		2. in non-quantitative data are whichever elements have the
			value of smallest cardinality (the minority).

		This method should only be called if this Vector is known to contain
		multiple values. (Otherwise, state assumed to exist won't!)

		In the case of categorical data, this method implements a broad
		definition of "aberrant": all files having any of the possibly
		multiple different values of minimum cardinality among all values
		are considered aberrant. Ideally, the column is binary valued and
		this definition simply returns the files (their indices) with the
		(unique) minority value.
		"""
		assert hasattr(self,"aberrant") \
			or hasattr(self,"value_histogram")
		# ...otherwise, if neither of these exist, we could just make the
		# tail of this method conditional on "not is_single_valued" which
		# would trigger their just-in-time generation...but I'm currently
		# assuming that has been called ealier, so one of these exists.
		if not hasattr(self,"aberrant"):
			min_cardinality = min( self.value_histogram.values() )
			minority_values = frozenset( filter(
				lambda k:self.value_histogram[k] == min_cardinality,
				self.value_histogram.keys() ) )
			self.aberrant = [ i for i in range(len(self.data())) 
				if self.data()[i] in minority_values ]
		return self.aberrant


if __name__=="__main__":
	pass

