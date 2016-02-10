
#include <Python.h>
#include <stdbool.h>

#include "tabular/tabular.h"
#include "stats/quantile.h"
#include "stats/density.h"

//static PyObject *CharCompError;

static void dump_buffer( const Py_buffer *v, FILE *fp ) {
	fprintf( fp,
		".buf = %p\n"
		".obj = %p\n"
		".len = %ld\n"
		".itemsize = %ld\n"
		".readonly = %d\n"
		".ndim = %d\n"
		".format = %s\n"
		".shape = %p\n"
		".strides = %p\n"
		".suboffsets = %p\n"
		".internal = %p\n",
	v->buf,
	v->obj,
	v->len,
	v->itemsize,
	v->readonly,
	v->ndim,
	v->format,
	v->shape,
	v->strides,
	v->suboffsets,
	v->internal );
}

// TODO: Revisit following extern decls; here only so I don't have to
// figure out how to get distutils to handle private headers.
extern FILE *fopenx( const char *, const char * );
extern int fclosex( FILE * );

extern double medcouple_naive( double *values, int n );

// forward decl
static PyObject * _tabular_scan( PyObject *self, PyObject *args);
static PyObject * _robust_bounds( PyObject *self, PyObject *args);
static PyObject * _gaussian_kde( PyObject *self, PyObject *args);


static PyMethodDef CharCompMethods[] = {
	{"tabular_scan",  _tabular_scan, METH_VARARGS,
	"Analyzes a file that is a priori assumed:\n"
	"1. to contain only UTF-8 text, and\n"
	"2. to contain a data table or matrix.\n"
	"Analysis terminates as soon as a byte sequence is encountered that is\n"
	"not interpretable as UTF8-encoded text (or a file I/O error prevents\n"
	"completion). If the analysis does not detect a table/matrix structure,\n"
	"results are limited to histograms of character classes and pairs\n"
	"described below.\n"
	"If a table/matrix is detected, its content is analyzed as described\n"
	"below.\n"
	"\n"
	"CHARACTER CONTENT ANALYSIS\n"
	"==========================\n"
	"If the file is text, then at a minimum two histograms are returned:\n"
	"- One summarizes the character content of the file in terms of 6\n"
	"  character categories:\n"
	"  1. LF (line feeds, ASCII 0x0A)\n"
	"  2. CR (carriage returns, ASCII 0x0D)\n"
	"  3. non-line-terminating, ASCII characters (UTF8/1)\n"
	"  4. non-line-terminating, two-byte characters (UTF8/2)\n"
	"  5. non-line-terminating, UTF8/3 characters\n"
	"  6. non-line-terminating, UTF8/4 characters\n"
	"- The other histogram counts transitions between LF, CR, and any\n"
	"  non-line-terminating characters and is arranged as a 3x3 matrix.\n"
	"The resulting count data completely determines the line termination\n"
	"convention in a file as well as verifying the convention is followed\n"
	"consistently throughout the entire file.\n"
	"\n"
	"TABLE/MATRIX ANALYSIS (TODO)\n"
	"============================\n"
	"\n"
	"IMPLEMENTATION NOTES\n"
	"====================\n"
	"This function combines capabilities which arguably ought to be\n"
	"factored between multiple functions. They are integrated for the\n"
	"sake of performance--specifically, in order to infer as much as\n"
	"possible about a file in *one pass*.\n",
	},
	{"robust_bounds", _robust_bounds, METH_VARARGS,
	"This function identifies the bounds of non-outlier data using the\n"
	"medcouple.\n",
	},
	{"gaussian_kde",  _gaussian_kde, METH_VARARGS,
	"Produce a series of points describing a density function.\n",
	},
	{NULL, NULL, 0, NULL}        /* Sentinel */
};

static struct PyModuleDef _moduledef = {
	PyModuleDef_HEAD_INIT,
	"compiled",	/* This becomes the __name__ attribute of the loaded module. */
				/* It need not be identical to the 1st arg to */
				/* distutils.core.Extension, though it currently is. */
	"This module serves as a container for ALL compiled code within the BDQC\n"
	"framework. The intent is provide a single place to house all the C code,\n"
	"so this module necessarily contains an ad hoc mixture of methods.\n"
	"It is not tied to a single class, plugin or Python module.\n",
	-1,       /* size of per-interpreter state of the module,
				or -1 if the module keeps state in global variables. */
	CharCompMethods
};


static PyObject *
_tabular_scan(PyObject *self, PyObject *args) {

	FILE *fp = NULL;
	const char *filename = NULL;
	PyObject *json = NULL;

	if( ! PyArg_ParseTuple(args, "s", &filename ) )
	    return NULL;

	fp = fopenx( filename, "r" );
	if( fp ) {

		struct table_description results;
		int failed = 0;
	
		memset( &results, 0, sizeof(results) );
		failed = tabular_scan( fp, &results );
		fclosex( fp );

		/**
		  * We can produce JSON results for any termination status
		  * except file I/O errors.
		  */

		if( ! failed ) {

			FILE *json_fp = tmpfile();
			if( json_fp ) {
				void *buf;
				size_t len;
				tabular_as_json( &results, json_fp );
				len = ftell( json_fp );
				buf = calloc( len + 1, sizeof(char) );
				if( buf ) {
					rewind( json_fp );
					if( fread( buf, sizeof(char), len, json_fp ) == len )
						json = PyUnicode_FromString( buf );
					free( buf ); // TODO: verify safe!
				}
				fclose( json_fp );
			}

			if( json == NULL )
				PyErr_SetString( PyExc_RuntimeError,
					"failed encoding table analysis as JSON" );

		} else {

			switch( results.status ) {

			case E_UNINITIALIZED_OUTPUT:
				PyErr_SetString( PyExc_RuntimeError,
					"tabular_scan received uninitialized output struct" );
				break;

			case E_FILE_IO:
				PyErr_SetFromErrnoWithFilename( PyExc_IOError, filename );
				break;

			default:
				PyErr_SetString( PyExc_RuntimeError,
					"unhandled error (unfinished code?) in " __FILE__ );
			}
		}
	
		tabular_free( &results );

	} else
		PyErr_SetFromErrnoWithFilename( PyExc_IOError, filename );

	return json;
}


/**
  * whisk <- 1.5*IQR(x)*if( mc < 0 ) {
  * 	c( exp(-3.0*mc), exp(+4.0*mc) )
  * } else {
  * 	c( exp(-4.0*mc), exp(+3.0*mc) )
  * }
  * fence <- c( quantile(x)['25%'] - whisk[1], quantile(x)['75%'] + whisk[2] );
  */
static PyObject *
_robust_bounds( PyObject *self, PyObject *args) {

	double lb = 0, ub = 0, mc = 0;
	Py_buffer view;
	PyObject *array;

	memset( &view, 0, sizeof(view));

	if( ! PyArg_ParseTuple( args, "O", &array ) ) {
		return NULL;
	}

	//fprintf( stdout, "array.ob_refcnt = %ld\n", array->ob_refcnt );

	if( PyObject_GetBuffer( array, &view, PyBUF_SIMPLE ) == 0 ) {

		double Q1, Q3, SIQR /* Scaled IQR */;
		int n;
		n = view.len / view.itemsize;
		Q1 = quantile( view.buf, n, 0.250 );
		Q3 = quantile( view.buf, n, 0.750 );
		mc = medcouple_naive( view.buf, n );

		PyBuffer_Release( &view );

		SIQR = (1.5 * (Q3-Q1));
		if( mc < 0.0 ) {
			lb = Q1 - SIQR*exp(-3.0*mc);
			ub = Q3 + SIQR*exp(+4.0*mc);
		} else {
			lb = Q1 - SIQR*exp(-4.0*mc);
			ub = Q3 + SIQR*exp(+3.0*mc);
		}
	}

	return PyTuple_Pack( 2, PyFloat_FromDouble(lb), PyFloat_FromDouble(ub) );
}


static PyObject *
_gaussian_kde( PyObject *self, PyObject *args) {

	Py_buffer view;
	PyObject *array;
	PyObject *points = NULL;
	memset( &view, 0, sizeof(view));

	if( ! PyArg_ParseTuple( args, "O", &array ) ) {
		return NULL;
	}

	if( PyObject_GetBuffer( array, &view, PyBUF_SIMPLE ) == 0 ) {
		const int n = view.len / view.itemsize;
#define N_PTS (512)
		double *xd = calloc( 2* N_PTS, sizeof(double) );
		double *yd = xd + N_PTS;
		if( xd && yd ) {
			points = PyTuple_New( N_PTS );
			if( points ) {
				gkde( view.buf, n, xd, yd, N_PTS );
				for(int i = 0; i < N_PTS; i++ ) {
					PyObject *pair
						= PyTuple_Pack( 2,
							PyFloat_FromDouble(xd[i]),
							PyFloat_FromDouble(yd[i]) );
					PyTuple_SetItem( points, i, pair );
				}
			}
			free( xd );
		}
		PyBuffer_Release( &view );
	}

	return points;
}


/**
  * From python-3.3.2-docs-html/extending/extending.html#a-simple-example:
  * "The initialization function must be named PyInit_name(), where name is
  * the name of the module, and should be the only non-static item defined
  * in the module file."
  */

PyMODINIT_FUNC PyInit_compiled(void) {

	PyObject *m
		= PyModule_Create( &_moduledef );
	if (m == NULL)
	    return NULL;
/*
	CharCompError = PyErr_NewException("text.error", NULL, NULL);
	Py_INCREF(CharCompError);
	PyModule_AddObject(m, "error", CharCompError);
*/
	return m;
}

