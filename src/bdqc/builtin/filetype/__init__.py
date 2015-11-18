
"""
Compression:
In a sense, compressed files have two types: the compression type and the
actual content type. The only conceivable motivation for analyzing the bytes
of a file in compressed form is entropy quantification. However, in all con-
ceivable cases, downstream analysis plugins will care *only* about the *un-
compressed* content. Detection early lets downstream plugins do the sensible
thing.
"""

import os
import os.path
import warnings
import importlib
import logging

TARGET = "file"
DEPENDENCIES = ['bdqc.builtin.extrinsic',]

_KNOWN_CODECS = (
	{	'canonical_ext':"gz",
		'sig':bytes((0x1F,0x8B )),
		'lib':"gzip"
	},
	{	'canonical_ext':"bz2",
		'sig':bytes((0x42,0x5A,0x68)),
		'lib':"bz2"
	},
	{	'canonical_ext':"xz",
		'sig':bytes((0xFD,0x37,0x7A,0x58,0x5A,0x00)),
		'lib':"lzma"
	}
)

_MISMATCH_TEMPLATE = "Compression indicated by {}'s extension does not match that indicated by its signature (\"{}\")"
_NO_CODEC_TEMPLATE = "No codec available for {}"

def _id_codec_by_sig( fp ):
	"""
	Infers codec type by examining the files initial bytes.
	GZIP: 1F 8B
	BZIP: 42 5A 68          ('B', 'Z', 'h')
	  XZ: FD 37 7A 58 5A 00 (FD, '7', 'z', 'X', 'Z', 0x00)
	This is intended to return a character "code" that
	corresponds to the more-or-less standard file extensions.
	"""
	pos = fp.tell()
	fp.seek(0, os.SEEK_SET )
	sig = fp.read(6)
	fp.seek(pos, os.SEEK_SET )
	for codec in _KNOWN_CODECS:
		if len(sig) >= len(codec['sig']) \
			and codec['sig'] == sig[:len(codec['sig'])]:
			return codec
	return None


def process( name, state ):
	"""
	If and only if the extrinsic builtin determined that name is readable,
	assess whether it's compressed, get the signature of the content, and
	attempt to classify it as "definitely binary" or "potentially text"
	using a DB of known file signatures.
	"""
	if not state.get('bdqc.builtin.extrinsic',False):
		return None
	codec = None
	_open = open
	sig   = None
	if state['bdqc.builtin.extrinsic']['readable'] == "yes":
		with open(name,"rb") as fp:
			codec = _id_codec_by_sig( fp )
		if codec: # ...implying it IS a compressed file.
			expected_ext = codec['canonical_ext']
			actual_ext = os.path.splitext( name )[1].lower()
			if not actual_ext.endswith( expected_ext ):
				logging.warn( _MISMATCH_TEMPLATE.format( name, codec ) )
			try:
				lib = importlib.import_module(codec['lib'])
				_open = lib.open
			except ImportError:
				logging.warn( _NO_CODEC_TEMPLATE.format( name ) )
				_open = None # ...we won't be able to read the actual sig
		if _open:
			with _open( name, "rb") as fp:
				sig = fp.read(8)
			# Convert sig from a bytes object (which the json module can't
			# serialize) to a list of byte values (or None) which it can.
			sig = list(sig) if len(sig) > 0 else None
		# TODO: Signature-based file infererence should attempt to
		# categorize file as "definitely binary" or "potentially text" and
		# further
		#	raster image
		#	executable
	return {'compression':codec['canonical_ext'] if codec else None, 'sig':sig }

# Unit test
if __name__=="__main__":
	import sys
	if len(sys.argv) < 2:
		print( "{} <filename>".format( sys.argv[0] ), file=sys.stderr )
		sys.exit(-1)
	print( process( sys.argv[1], {'readable':"yes"} ) )

