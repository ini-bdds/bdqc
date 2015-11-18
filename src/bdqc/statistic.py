
from math import isnan
import random
import re

class NonScalarData(RuntimeError):
	def __init__( self ):
		pass

class Descriptor(object):
	"""
	This class provides a means to describe the appropriate statistical
	treatment of data produced by plugins. Plugins SHOULD export data
	dictionaries in which each computed statistic is "tagged" by an instance
	of this class as:

	STATISTICS = {
		"MD5":Descriptor(IGNORABLE),
		"filesize":Descriptor(QUANTITATIVE,min=0),
		"readable":Descriptor(CATEGORICAL,cardinality=2,ordered=False)
	}

	This information helps guide the analysis phase.
	Currently, plugins are NOT REQUIRED to export data dictionaries,
	but this may change.

	Below defines three broad categories of statistic:
	1. quantitative
	2. ordinal
	3. categorical
	...and provides several qualifications to each of these, as appropriate.

	The most general/inclusive labels should be applied. For example,
	continuous data can always be *treated* as ordinal--that is, converted
	to ranks, but if data is inherently continuous, then it should be so
	labeled.

	These labels are intended to be descriptive, not prescriptive.

	These labels are *sometimes* essential because one can't definitively
	infer a statistical class from *type* (int,float,bool,string). This
	inference is straightforward for nominative (encoded as string labels)
	and overtly continuous data (encoded as floating-point data), but
	it can be impossible to infer the best statistical treatment in data
	represented as integers; it may be categorical, quantitative or ordinal,
	depending on factors such as cardinality, missing count, etc.
	Even floating-point data might encode ordinal or even categorical data
	even though it *almost* always implies continuous data.  Inferring the
	intended statistical interpretation itself becomes a statistical
	problem.

	These labels allow plugin authors to make explicit the intended
	interpretation of a statistic. In their absence BDQC will make
	reasonable assumptions.
	TODO: Describe all the conventions:
	1. data encoded as string labels is ALWAYS treated as nominal (UNordered
		categorical)
	2. data encoded as floating point is ALWAYS treated as quantitative.
	3. 
	"""

	"""
	Some values published by plugins may be unsuitable for statistical
	treatment. MD5 file hashes are probably the best example. These are
	essentially contentless values.
	"""
	IGN          = 0
	IGNORABLE    = 0

	"""
	QUANTITATIVE implies data is continuous, ordered, and magnitude is
	meaningful. That is, magnitude has some probably physical
	interpretation. The cardinality of the data approaches sample size unless
	data is missing. QUANTITATIVE data is necessarily numeric (represented
	as integers or floating-point numbers).
	"""
	QUA          = 1
	QUANTITATIVE = 1

	"""
	ORDINAL data canonically implies rank data, but it may involve floating-
	point representations since some methods of handling ties (in original
	data, e.g. averaging) yield non-integral values. The cardinality of the
	data approaches sample size unless data is missing or (the original
	data) contained large numbers of ties.
	Most importantly, the absolute value of a data point has no intrinsic
	meaning, and intervals are only meaningful as measures of relative
	separation (e.g. between ranks).
	"""
	ORD          = 2
	ORDINAL      = 2

	"""
	The primary distinguishing feature of categorical data is the
	cardinality of the value set: it is typicall "much" less than the
	sample size, though no universal definition exists.
	Categorical data may or may not be ordered.
	Boolean data is merely a special case of categorical with cardinality
	2, and it, too, may or may not be considered ordered (Y/N vs. Hi/Low).
	Unordered categorical data is "nominative/nominal".
	"""
	CAT          = 3
	CATEGORICAL  = 3

	STAT_CLASS_NAMES = ('IGN','QUA','ORD','CAT')

	TYPE = ('non-scalar','int','float','str','bool','complex')
	TY_NONSCALAR = 0
	TY_INT   = 1
	TY_FLOAT = 2
	TY_STR   = 3
	TY_BOOL  = 4
	TY_CMPX  = 5

	@staticmethod
	def scalar_type( obj ):
		"""
		This merely converts the Python-canonical type name of obj to
		an integer code for which non-zero values indicate the specific
		scalar type and 0 indicates any non-scalar type.
		This use of 0 makes this function serve as an "IS_scalar_type", too.
		(Note this is not the same as "statistical class"!  See plugin.py.)
		"""
		try:
			return Descriptor.TYPE.index( obj.__class__.__name__ )
		except ValueError:
			return 0

	@staticmethod
	def statClassCode( spec ):
		if isinstance(spec,int):
			if not (0 <= spec and spec < len(Descriptor.STAT_CLASS_NAMES)):
				raise RuntimeError( "invalid statclass code: " + str(spec) )
			return spec
		elif isinstance(spec,str):
			s = spec.lower()
			l = len(spec)
			if "ignore"[:l] == s:
				return Descriptor.IGN
			elif "quantitative"[:l] == s:
				return Descriptor.QUA
			elif "ordinal"[:l] == s:
				return Descriptor.ORD
			elif "categorical"[:l] == s:
				return Descriptor.CAT
			else:
				raise RuntimeError( "invalid statclass: " + spec )
		raise TypeError("expected int or string for statclass argument")

	NA = re.compile("[Nn]([Aa][Nn])?")

#	@staticmethod
#	def non_missing( val ):
#		"""
#		This function recognizes missing values of all three primary types
#		subject to the following strategy:
#		1. Floating-point NaN IS TREATED as a missing value placeholder.
#		2. A string value matching the 'NA' regular-expression (above)
#			IS TREATED
#
#		Identify valid values by relying on Python's built-in ability to
#		coerce int's and str's to float. A string that can't be so coerced
#		will generate a ValueError, and any other type (that does not define
#		the coercion) will generate a TypeError.
#		Pretty much any type is convertible to str, so as a last resort
#		check whether the object interpreted as a str matches a default
#		"missing value" regular expression (defined above).
#		"""
#		try:
#			return not isnan( float(val) )
#		except ValueError:
#			return not NA.match( str(val) )
#		except TypeError:
#			return not NA.match( str(val) )

	@staticmethod
	def _is_ordinal( data ):
		"""
		This implements a conservative definition of ordinal data:
		contiguous integers in [0,|sample|-1] or [1,|sample|].
		"""
		m = min(data)
		M = max(data)
		L = len(data)
		return (m == 0 or m == 1) and (M == L-1 or M == L )

	@staticmethod
	def infer_class( data ):
		"""
		Applies default rules to infer a statistical class from a vector
		of *arbitrary* data.
		Feasibility of inferring statistical class from type depends on its:
		1) representation
		2) sample size
		3) maybe number of NON-MISSING values

		Strategy:
		If a single (non-None/NA) type is present in the data:

		1) float: QUANTITATIVE
			Could be CATEGORICAL if cardinality is small enough relative
			to (non-missing) sample size.
		2) int:
			a. CATEGORICAL if cardinality is small enough relative to
				non-missing sample size,
			b. ORDINAL if contiguous and 0/1-based,
			c. QUANTITATIVE otherwise.
		3) string
			CATEGORICAL and nominative unless...
			We DEFINE {f(alse)?,no?} => 0, {t(rue)?,y(es)?} => 1 so that booleans
			are implicitly ordered?

		If multiple (non-None/NA) types are present:
		1. If any is float, all are coerced to float if possible.
		2. If ...
		"""
		# First, just eliminate None's; we can't do anything with them.
		values = list(filter(lambda v:v is not None, data ))
		if len(values) == 0:
			raise RuntimeError("Data is entirely missing; all values are \"None\"")
		# Identify the (necessarily-uniform) type...
		typevote = [0]*len(Descriptor.TYPE)
		for x in values:
			typevote[ Descriptor.scalar_type(x) ] += 1
		# If any "value" was non-scalar, it's game over...
		if typevote[Descriptor.TY_NONSCALAR] > 0:
			raise NonScalarData()
		# ...otherwise, how many distinct types does the data contain?
		# If more than 1 type is detected, it's game over.
		if sum([ int(v>0) for v in typevote ]) > 1:
			raise NotImplementedError("mixed-data type class inference")
		typecode = typevote.index( max(typevote) ) # ...all but one should be 0!
		# ...the value set...
		if typecode == Descriptor.TY_FLOAT:
			return Descriptor(sclass="qua",min=min(values),max=max(values))
		if typecode == Descriptor.TY_INT:
			value_set = frozenset( values )
			IS_LOW_CARDINALITY \
				= len(value_set) <= ( len(values) // 10 )
			if IS_LOW_CARDINALITY:
				return Descriptor(cardinality=len(value_set),ordered=True)
			elif Descriptor._is_ordinal(values):
				return Descriptor(sclass="ord",min=int(min(values)),max=int(max(values)))
			else:
				return Descriptor(sclass="qua",min=min(values),max=max(values))
		if typecode == Descriptor.TY_STR:
			return Descriptor(cardinality=len(frozenset(values)))
		if typecode == Descriptor.TY_BOOL:
			return Descriptor(cardinality=2, ordered=True)
		return None


	@staticmethod
	def random( allowdegen=False ):
		"""
		Generate a random instance of the Descriptor class.
		Note that this is a description of a statistic, not a random
		*value* of a Descriptor.
		"""
		sc = random.choice([ Descriptor.QUANTITATIVE, Descriptor.ORDINAL, Descriptor.CATEGORICAL ])
		if sc == Descriptor.CATEGORICAL:
			args = {'cardinality':random.randint(2,8),'ordered':random.choice([True,False])}
		elif sc == Descriptor.QUANTITATIVE:
			m = random.uniform(0,1)
			s = 10**random.randint(0,10)
			args = {'min':s*m,'max':s*random.uniform(m,1)}
		else:
			args = {'min':1,'max':random.randint(2,1000)}
		return Descriptor( **args )

	# TODO: Maybe add 'labels'/'labels' 
	KW = frozenset(['sclass','ordered','cardinality','labels','min','max'])

	def __init__( self, **args ):
		"""
		Make constructor smarter as follows:
		1. Specifying 'sclass' as one of the strings {'quantitative',
			'ordinal','categorical'} or a uniquely determining prefix
			thereof) allows explicit specification of the statistical
			class.
		2. Specifying 'cardinality', 'labels' or 'ordered' implies
			categorical type.
		3. Specifying BOTH bounds as INTEGERS implies ordinal.
		4. In all other cases, a quantitative statistic is assumed.
		"""
		if not Descriptor.KW.issuperset( frozenset(args.keys()) ):
			raise RuntimeError("bad keyword args: {}".format(
				frozenset(args.keys())-Descriptor.KW ) )
		####################################################################
		# First determine the intended statistical class from args
		if 'sclass' in args: # ...then it's explicit.
			self.stat_class = Descriptor.statClassCode( args['sclass'] )
		# ...otherwise, it's implicit from the supplied parameters and/or
		# their types.
		elif ('labels' in args) or \
			('cardinality' in args) or \
			('ordered' in args):
			self.stat_class = Descriptor.CAT
		elif isinstance(args.get('min',None),int) and \
			isinstance(args.get('max',None),int):
			self.stat_class = Descriptor.ORD
		else:
			self.stat_class = Descriptor.QUA
		####################################################################
		# Content of args depends on stat_class...
		if self.stat_class == Descriptor.CATEGORICAL:
			if 'labels' in args:
				a = args['labels']
				if not isinstance(a,list):
					raise RuntimeError("labels (\"{}\") should be a list".format(a))
				if len(a) == 0:
					raise RuntimeError("labels (\"{}\") is empty".format(a))
				self.cardinality = len(a)
			else:
				self.cardinality = int(args.get('cardinality',5))
			# Assumed it's nominative catgerical unless otherwise spec'ed!
			self.ordered = bool(args.get('ordered',False))
		else:
			# Either or both ends of a range may be specified for
		 	# Quantitative and Ordinal types.
			m = args.get('min',None)
			M = args.get('max',None)
			if not( (m is None) or (M is None) or (m <= M) ):
				raise ValueError("min({}) > max({})".format( m, M ))
			self.min = m
			self.max = M

	def __str__( self ):
		return repr(self)

	def __eq__( self, rhs ):
		"""
		Implements a test for strong equality.
		"""
		if self.stat_class != rhs.stat_class:
			return False
		if self.stat_class == Descriptor.CAT:
			return self.cardinality == rhs.cardinality and \
				self.ordered == rhs.ordered
		else:
			return self.min == rhs.min and self.max == rhs.max
		return True


	def __repr__( self ):
		"""
		"""
		if self.stat_class == Descriptor.CATEGORICAL:
			return "{}(sclass=\"{}\",cardinality={cardinality},ordered={ordered})".format(
				self.__class__.__name__, 
				Descriptor.STAT_CLASS_NAMES[self.stat_class],
				**self.__dict__ )
		else:
			# TODO 
			return "{}(sclass=\"{}\",min={min:},max={max:})".format(
				self.__class__.__name__,
				Descriptor.STAT_CLASS_NAMES[self.stat_class],
				**self.__dict__ )

	def is_boolean( self ):
		return getattr(self,"cardinality",0) == 2

	def is_categorical( self ):
		return self.stat_class == Descriptor.CAT

	def is_quantitative( self ):
		return self.stat_class == Descriptor.QUA

	def is_ordinal( self ):
		return self.stat_class == Descriptor.ORD

	def randomValues( self, missing_p=0.0, count=1 ):
		"""
		Generate a random vector of length <count> according to this
		instance's parameters.
		"""
		if count < 1:
			raise ValueError("count ({}) < 1 is senseless".format(count) )
		vec = []
		for i in range(count):
			if self.is_categorical():
				n = random.randint(0,self.cardinality-1)
				if self.ordered:
					value = n
				elif self.cardinality == 2:
					value = ['F','T'][n]
				else:
					value = chr(ord('A') + (n % 26))*((n//26)+1)
			elif self.stat_class == Descriptor.ORD:
				value = random.randint(
					0    if self.min is None else self.min,
					1e12 if self.max is None else self.max )
			else: # quantitative
				value = random.uniform(
					0    if self.min is None else self.min,
					1e12 if self.max is None else self.max )
			vec.append( value )
		# Now decimate values if missing values are allowed.
		# Note that missing_p is interpreted differently according to
		# whether the count of desired random values.
		if missing_p > 0.0:
			if count == 1:
				if random.random() < missing_p:
					vec[0] = None
			else:
				# Decimate up to missing_p by sampling WITH replacement.
				M = int( count * missing_p )
				while M > 0:
					M = M - 1
					vec[ random.choice(range(count)) ] = None
		# Don't needless return lists; make it a scalar when appropriate!
		return vec[0] if count == 1 else vec


if __name__=="__main__":
	import argparse

	_parser = argparse.ArgumentParser(
		description="Unit test for bdqc.plugin.Manager.",
		epilog="""\
		""")
	_parser.add_argument( "--missing", "-p",
		type=float,
		default=0.0,
		metavar="MISSING",
		help="Probability (or proportion) of missing values. [%(default).3f]")
	_parser.add_argument( "--count", "-c",
		type=int,
		default=1,
		metavar="COUNT",
		help="Generate %(metavar)s [%(default)i] random values.")
	_args = _parser.parse_args()

	s = Descriptor.random()
	print( s )
	print( repr(s) )
	print( s.randomValues( missing_p=_args.missing, count=_args.count) )

