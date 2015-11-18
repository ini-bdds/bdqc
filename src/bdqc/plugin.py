
import bdqc.depends
import importlib

class Manager(object):
	"""
	Manages the collection of plugins. Duh.
	In particular,
	1. loads the modules implementing the plugins and
	2. resolves (and loads) their dependencies
	3. determines an execution order (usually not unique) that respects
		the dependency structure
	4. Provides a mapping of plugin to [0,|plugins|) that reflects 
		execution order computed in #3.
	"""

	@staticmethod
	def _resolve( explicit_dependencies ):
		"""
		Imports explicitly-specified plugins, and resolves implicit
		dependencies (dependencies on unspecified plugins).
		"""
		modules = []
		missing  = set(explicit_dependencies)
		resolved = set() # monotonically grows
		# Every newly-loaded plugin potentially introduces new dependencies,
		# so several rounds of load+test-for-absence might be required.
		while missing:
			# Import whatever is (still) missing as a batch.
			imported = [ importlib.import_module(d) for d in missing ]
			# ...and add their names to the resolved list.
			resolved.update( [ m.__name__ for m in imported ] )
			# Do dependencies remain now?
			missing.clear()
			while imported:
				m = imported.pop()
				try:
					missing.update([ d for d in m.DEPENDENCIES if not d in resolved])
				except AttributeError:
					setattr(m,'DEPENDENCIES',[])
					# ...simplifies code elsewhere by allowing assumption
					# that DEPENDENCIES exists, though may be empty!
				modules.append( m )
		assert all([('DEPENDENCIES' in dir(m)) for m in modules ])
		return modules

	def __init__( self, plugin_names ):
		assert isinstance( plugin_names, list ) and \
			all([isinstance(m,str) for m in plugin_names ])
		module_list = Manager._resolve( plugin_names )
		dd = bdqc.depends.Dependencies(
			dict([ (m.__name__,set(m.DEPENDENCIES)) for m in module_list]) )
		ordered_names = dd.tsort()
		# Create a TEMPORARY dict so that we can access modules by name...
		module_dict = dict(zip([m.__name__ for m in module_list],module_list))
		# ...and a PERMANENT mapping of module names to (<module>,<ordinal>)
		self.module_dict = {}
		for i in range(len(ordered_names)):
			name = ordered_names[i]
			self.module_dict[name] = (module_dict[name],i)
		# Finally, convert tsort's string list output into a module list.
		self.ordered_modules \
			= tuple(map(lambda name:module_dict[name],ordered_names))

	def __str__( self ):
		return '\n'.join([ m.__name__ for m in self.ordered_modules])

	def __len__( self ):
		return len(self.ordered_modules)

	def __getitem__( self, key ):
		"""
		This method facilitates mapping modules to their execution order
		AND mapping execution position to module names.
		The direction of mapping (and type of return) depends on the type
		of the argument.
		"""
		if isinstance(key, int):
			return self.ordered_modules[ key ]
		elif isinstance(key,str):
			return self.module_dict[ key ][1]
		elif getattr( key, '__name__', None ):
			name = getattr( key, '__name__' )
			return self.module_dict[ name ][1]
		else:
			raise TypeError("expected int, str or module; received "+str(type(key)))

	def __iter__( self ):
		"""
		This just returns a list of modules IN EXECUTION ORDER--that is,
		the order is very special, not arbitrary.
		"""
		return iter(self.ordered_modules)


if __name__=="__main__":
	import argparse

	_parser = argparse.ArgumentParser(
		description="Unit test for bdqc.plugin.Manager.",
		epilog="""\
		""")
	_parser.add_argument( "--plugins", "-m",
		default=None,
		metavar="PLUGINS",
		help="File listing analysis plugins to execute.[%(default)s]")
	_args = _parser.parse_args()

	if _args.plugins:
		with open( _args.plugins ) as fp:
			plugins = [ l.rstrip() for l in fp.readlines() ]
		m = Manager( plugins )
		print( str(m) )
