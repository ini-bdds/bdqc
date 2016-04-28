
"""
This plugin will probably always be the first executed in any analysis.
By identifying such basic things as the readability of the filename (i.e.
the *existence* of the named file), it bootstraps the analysis of almost
every other conceivable plugin.
"""

import os
import os.path

TARGET  = "file"
VERSION = 0x00010100
#DEPENDENCIES = []

def process( name, state ):
	"""
	This plugin only examines attributes of the file visible "from the
	outside"--that is, without opening and reading any of its contents.
	readable { 'absent','notfile','perm','yes' }
	"""
	ext  = os.path.splitext( name )[1][1:] # skip over the '.'
	info = None
	if os.path.exists( name ):
		if os.path.isfile( name ):
			readable = 'yes' if os.access( name, os.R_OK ) else 'perm'
		else:
			readable = 'notfile'
		# stat always works (regardless of permissions) as long as the
		# the file is not missing. (In that case it raises FileNotFound.)
		info = os.stat( name )
	else:
		readable = 'missing'
	size  = info.st_size  if info else None
	# Time is incompatible with current heuristic approach.
	#mtime = info.st_mtime if info else None
	return {'readable':readable,'size':size,'ext':ext }

# Unit test
if __name__=="__main__":
	import sys
	if len(sys.argv) < 2:
		print( "{} <filename>".format( sys.argv[0] ), file=sys.stderr )
		sys.exit(-1)
	print( process( sys.argv[1], None ) )

