
Table of Contents
#################

- Installation_
- Overview_

	- `What is it?`_
	- `What is it for?`_
	- `What does it do?`_
- `How does it work?`_
- Plugins_


_`Installation`
###############

The BDQC framework has no requirements other than Python 3.3.2 or later.
The GCC toolchain is required for installation as some of its
components are C code that must be compiled.

After extracting the archive...

.. code-block:: shell

	python3 setup.py install

...installs the framework, after which...

.. code-block:: shell

	python3 -m bdqc.scan <directory>

...will analyze all files in <directory>, and

.. code-block:: shell

	python3 -m bdqc.scan --help
	
...provides further help.
The contents of the online help is not repeated in this document.


_`Overview`
###########

_`What is it?`
==============

BDQC is a Python3_ software framework and executable module.
Although it provides built-in capabilities that make it useful "out of the
box", being a "framework" means that users (knowledgeable in Python
programming) can extend its capabilities, and it is *intended* to
be so extended.

_`What is it for?`
==================

BDQC identifies anomalous files among large collections of files which are
*a priori* assumed to be "similar."
It was motivated by the realization that when faced with many thousands of
individual files it can be challenging to even confirm they all contain
approximately what they should.

It is intended to:

1. validate primary input data to pipelines
2. validate output (or intermediate stages of) data processing pipelines
3. discover potentially "interesting" outliers

These use cases merely highlight different sources of anomalies in data.
In the first, anomalies might be due to faulty data handling or acquisition
(e.g. sloppy manual procedures or faulty sensors). In the second, anomalies
might appear in data due to bugs in pipeline software or runtime failures
(e.g. power outages, network unavailablity, etc.). Finally, anomalies that
can't be discounted as being due to technical problems might actually be
"interesting" observations to be followed up in research.

Although it was developed in the context of genomics research, it is 
expressly not tied to a specific knowledge domain. It can be customized
as much as desired (via the plugin mechanism) for specific knowledge domains.

Importantly, *files* are its fundamental unit of operation.
This means that a file must constitute a meaningful unit of
information--one sample's data, for example--in any
application of BDQC.

_`What does it do?`
===================

BDQC analyzes a collection of files in two stages.
First, it analyzes each file individually and produces a summary of the
file's content (`within-file analysis <Within-file analysis_>`_).
Second, the aggregated file summaries are analyzed heuristically
(`between-file analysis <Between-file analysis_>`_) to identify possible anomalies.

The two stages of operation can be run independently.

BDQC can be run from the command line, and command line arguments control
which files are analyzed,
how files are summarized,
how the summaries are aggregated and finally analyzed.
All command line arguments are optional; the framework will carry out
default actions. See command line help.

Alternatively, the bdqc.Executor Python class can be incorporated directly
into third party Python code. This allows it to be incorporated into
pipelines.

Design goals
============

The BDQC framework was developed with several explicit goals in mind:

1. Identify an "anomalous" file among a large collection of *similar* files of *arbitrary* type with as little guidance from the user as possible, ideally none.  In other words, it should be useful "out of the box" with almost no learning curve.
2. "Simple things should be simple; complex things should be possible" [#]_ Although basic use should involve almost no learning curve, it should be possible to extend it with arbitrarily complex (and possibly domain-specific) analysis capabilities.
3. Plugins should be simple (for a competent Python programmer) to develop, and the system must be robust to faults in plugins.

_`How does it work?`
####################

This section describes in more detail how BDQC works internally.
This and following sections are required reading for anyone
wanting to develop their own plugins.

The most important fact to understand about BDQC is that
*plugins*, **not the** *framework*, **carry out all analysis of input files.**
The BDQC framework merely orchestrates the execution of `plugins <Plugins_>`_.
(The BDQC *package* includes several "built-in" plugins which insure
it is useful "out of the box." Though they are built-in, they are
nonetheless plugins.)

A plugin is simply a Python module that is installable like any Python module.
Plugins provide two kinds of analysis: file analysis and heuristic analysis.
A single plugin can provide:
	1. zero or one file analyzer and
	2. zero or more heuristic analyzers.

A *file analyzer* is just a function that can read a file and produce
one or more summary statistics about it.
A *heuristic analyzer* is a function that applies a test to a vector
of data returning a boolean result.

The file and heuristic analyzers in BDQC plugins are expected to take
certain forms, and the plugin is expected to export certain symbols used
by the BDQC framework (described in detail `below <Plugins_>`_).

.. image:: doc/dataflow2.png
	:align: center


_`Within-file analysis`
=======================

The plugins that are executed on a file entirely determine
the content of the summary (the statistics) generated for that file.
The framework itself *never* looks inside a file; only the plugins examine
file content.

The framework:

1. assembles a list of paths identifying files to be analyzed,
2. executes a *dynamically-determined* subset of the available plugins on each file path,
3. merges the plugins' results into one (JSON_-format) summary per analyzed file.

Each `plugin <Plugins_>`_ can declare (as part of its implementation) that it depends
on zero or more other plugins.

The framework:

1. insures that a plugin's dependencies execute before the plugin itself, and
2. each plugin is provided with the results of its dependencies' execution.

By virtue of their declared dependencies, the set of all plugins available
to BDQC (installed on the user's computer and visible on the PYTHONPATH)
constitute a directed acyclic graph (DAG), and a plugin that is "upstream"
in the DAG can determine how (or even whether or not) a downstream plugin is run.

The framework minimizes work by only executing a plugin when required.
The figure above represents the skipping of plugins; plugin *#3*, for example,
was not run on file *#N*.

.. TODO: cover the rerun decision tree.

By default, the summary for file foo.txt is left in an adjacent file named
foo.txt.bdqc.

Again, the BDQC *framework* does not touch files' content; it only
handles filenames and paths.

_`Between-file analysis`
========================

1. Summary (\*.bdqc) files are `collected <Collection_>`_.
2. All files' summaries (the JSON_-formatted content of all corresponding \*.bdqc files) are `flattened <Flattening_>`_ into a matrix.
3. `Heuristics are applied <Heuristic analysis_>`_ to the columns of the matrix to identify rows (corresponding to the original files) that might be anomalies.

_`Collection`
-------------

Typically bdqc.scan automatically invokes the between-files analysis on
the results of within-file analysis.
However, the between-file analysis can also be run independently, and
files and/or directories to analyze can be specified exactly as with
bdqc.scan.

_`Flattening`
-------------

A `plugin's <Plugins_>`_ output can be (almost) anything representable as
JSON_ data.
In particular, the "statistic(s)" produced by a plugin need not be scalars
(numbers and strings); they can be compound data like matrices or sets,
too [#]_.

Moreover, since even simple scalar statistics are typically embedded in
hierarchical JSON_ data, the individual statistics in plugins' summaries are
necessarily identified by *paths* in the JSON_ data.
For example, the following excerpt of output from the bdqc.builtin.tabular_
plugin's analysis of *one file* shows some of the many statistics it produces:

.. code-block:: JSON

        "bdqc.builtin.tabular": {
			"character_histogram": {
				"cr": 0, 
				"lf": 29,
				"ascii": 50418, 
				"utf8-2": 0, 
				"utf8-3": 0, 
				"utf8-4": 0
			}, 
			"offending_byte": 0, 
			"table": {
				"separator_is_regex": false, 
				"column_separator": "\t", 
				"metadata_prefix": "", 
				"aberrant_lines": 0, 
				"empty_lines": 0, 
				"data_lines": 29, 
				"column_count": 170, 
				"columns": [
					{
						"labels": [
							"16", 
							"1"
						], 
						"max_field_length": 2, 
						"extrema": {
							"min": 1.0, 
							"max": 16.0
						}, 
						"votes": {
							"integer": 29, 
							"string": 0, 
							"empty": 0, 
							"float": 0
						}, 
						"inferred_class": "categorical", 
						"stats": {
							"mean": 3.068966, 
							"variance": 27.70936
						}, 
						"long_field_count": 0, 
						"max_labels_exceeded": false
					}, 
					{
						"labels": [], 
						"max_field_length": 4, 
						"extrema": {
							"min": 38.0, 
							"max": 52.0
						}, 
						"votes": {
							"integer": 0, 
							"string": 0, 
							"empty": 0, 
							"float": 29
						}, 
						"inferred_class": "quantitative", 
						"stats": {
							"mean": 47.37931, 
							"variance": 14.529557
						}, 
						"long_field_count": 0, 
						"max_labels_exceeded": false
					},
					...
			},
			...
		}

The plugin inferred that the 2nd column in the file contains quantitative
data ("inferred_class"), and the mean value of that column was 47.38.
The process of "flattening" the JSON summaries creates one column in the
aggregate matrix from the values of the mean statistic *for all files analyzed*,
and that column's *name* is the path:

	bdqc.builtin.tabular/table/columns/1/stats/mean.

These paths can be used to make heuristic analysis selective. (See
heuristic configuration (TODO)).

In summary, each \*.bdqc file contains all plugins' statistics for one
analyzed file; each column in the aggregate matrix contains one statistic
(from one plugin) for all files analyzed.

.. The columns of the matrix are the individual statistics that plugins produce
.. in their analysis summaries.

_`Heuristic analysis`
=====================

A "heuristic" (in the context of BDQC) is essentially a test that can be
applied to columns of the aggregate matrix. The heuristic expresses
properties one or more statistics should (or should not) manifest or
constraints they should satisfy. Examples:
	1. Quantitative data have no "outliers" (suitably defined).
	2. Certain columns should be constant. In other words, all analyzed files should have the same value for certain statistics.

Some heuristics are only applicable to specific types of data, e.g.
quantitative; some are universal (e.g. "constantness").

BDQC defines several heuristics internally. The built-in plugins each
define additional heuristics. Finally, developers may add heuristics in
their own plugins.
Intuitively, a plugin that defines a file analyzer (and, implicitly,
one or more statistics) ought usually to also define a heuristic that
applies to those statistics.

BDQC applies heuristics in one of two modes: default and configured.

In default mode, BDQC applies:

	1. all available heuristics (internal and plugin-defined) to
	2. all columns of the aggregate matrix to which they are applicable (as determined by the data types of columns)

If a heuristic configuration is provided, it entirely replaces the defaults.

Heuristics are always applied *independently* to columns of the aggregate
matrix; there is currently no covariate analysis of any kind.

**It is the heuristics that flag files (which correspond to rows in the
aggregate matrix) as "anomalies".**
BDQC's default mode applies relatively conservative heuristics--that is,
it favors minimizing false positives.

The default configuration can be exported and edited to produce configurations
more appropriate for specific data domains. In this way, BDQC "bootstraps" and
simplifies the creation of semi-automated "sanity checks" for large data.

_`Plugins`
##########

To reiterate, the BDQC executable *framework* does not touch files itself.
All file analysis, both *within* and *between* files, is performed by plugins.
Several plugins are included in (but are, nonetheless, distinct from) the
framework. These plugins are referred to as "`Built-ins`_".

A plugin is simply a Python module with several required and optional
elements shown in the example below.

.. code-block:: python

	VERSION=0x00010000
	DEPENDENCIES = ['bdqc.builtin.extrinsic','some.other.plugin']
	def process( filename, dependencies_results ):
		# Optionally, verify or use contents of dependencies_results.
		with open( filename ) as fp:
			pass # ...do whatever is required to compute the values
		# returned below...
		return {
			'a_quantitative_statistic':1.2345,
			'a_3x2_matrix_of_float_result':[[3.0,1.2],[0.0,1.0],[1,2]],
			'a_set_result':['foo','bar','baz'],
			'a_categorical_result':"yes" }

Plugins must satisfy several constraints:

1. Every plugin *must* provide a list called DEPENDENCIES (which may be empty). Each dependency is a fully-qualified Python package name (as a string).
2. Every plugin *must* provide a two-argument function called process.
3. A plugin *may* include a VERSION declaration. If present, it must be convertible to an integer (using int()).
4. The process function *must* return one of the basic Python types: dict, list, tuple, scalar, or None [#]_.

	a. If the root type is a container (dict, list, tuple) all contained types (recursively) must be basic Python types.
	b. A plugin should *never* return empty dict's.
	c. A plugin's results may contain arbitrary dimension matrices (as nested lists and/or tuples). Matrices must have a single component type and be complete in all their dimensions.

These requirements do not limit what a plugin can *do*.
They merely define a *packaging* that allows the plugin to be hosted
by the framework. In particular, a plugin may invoke compiled code (e.g.
C or Fortran) and/or use arbitrary 3rd party libraries using standard
Python mechanisms.

Moreover, while a plugin is free to return multiple statistics,
the `Unix philosophy`_ of "Do one thing and do it well" suggests that a
plugin *should* return few statistics (or even only one).
This promotes reuse, extensibility, and unit-testability of plugins, and is
the motivation behind the plugin architecture.

There is no provision for passing arguments to plugins from the framework
itself. Environment variables can be used when a plugin must be
parameterized. [#]_

Developers are advised to look at the source code of any of the built-in
plugins for examples of how to write their own. The bdqc.builtin.extrinsic_
is a very simple plugin; bdqc.builtin.tabular_ is much more complex and
demonstrates how to use C code.

The framework will incorporate the VERSION number into the plugin's output
automatically. The plugin's code need not and should not include it in the
returned value. The version number is used by the framework (along with other factors) to decide
whether to *re*-run a plugin. (This is useful during plugin development.)
If a plugin does provide a VERSION, it's return *should* be a dict.
Otherwise, the framework will simply assign the generic name "value" to the
plugin's root return.

_`Built-ins`
============

The BDQC software package includes several built-in plugins so that it is
useful "out of the box." These plugins provide very general purpose analyses
and assume *nothing* about the files they analyze.
Although their output is demonstrably useful on its own, the built-in plugins
may be viewed as a means to "bootstrap" more specific (more domain-aware)
analyses.

_`bdqc.builtin.extrinsic`
-------------------------

.. warning:: Unfinished.

_`bdqc.builtin.filetype`
------------------------

.. warning:: Unfinished.

_`bdqc.builtin.tabular`
-----------------------

.. warning:: Unfinished.

.. Framework execution
.. ###################
.. 
.. After parsing command line arguments the framework (bdqc.scan):
.. 
.. 1. builds a list *P* of all candidate plugins
.. 2. identifies an ordering of plugins that respects all declared dependencies
.. 3. builds a list *F* of files to be (potentially) analyzed
.. 4. for each file *f* in *F*, for each plugin *p* in *P* it runs *p* on *f* *if it needs to be run*.
.. 
.. The files to be analyzed as well as the set of candidate plugins are
.. controlled by multiple command line options. See online help.
.. 
.. These steps always happen.
.. Aggregate analysis--that is, analysis of the plugins' analyses--is
.. carried out if and only if a file is specified (with the {\tt --accum}
.. option) to contain the plugins' results.
.. 
.. Whether a plugin is actually run on a file depends on global options,
.. the existence of earlier analysis results, the modification time of
.. the file and the version (if present) of the plugin.
.. 
.. A plugin is run on a file:
.. 1. if the --clobber flag is included in the command line; this forces (re)run and preempts all other considerations.
.. 2. if no results from the current plugin exist for the file.
.. 3. if results exist but their modification time is older than the file.
.. 4. if any of the plugin's dependencies were (re)run.
.. 5. when the plugin version is (present and) newer (greater) than the version that produced existing results.

Advanced topics
###############

Aggregation and "flattening" of JSON data
=========================================

The JSON_-formatted summaries generated by plugins are hierarchical in nature
since JSON_ Objects and Arrays can each contain other JSON_ Objects and Arrays.

The process of flattening the JSON_ to produce the summary matrix
need not, in general, result in columns of *scalars* (eg. numbers and string
labels).
Although it is always possible to arrive at columns of scalars by flattening ("exploding")
JSON_ compound objects *exhaustively*, the process is intentionally *not* exhaustive by default.
Because we want plugins to be able to return compound values as results (e.g. sets,
vectors, matrices [#]_) *without complicating JSON by defining special labeling
requirements*, the following rules and conventions are observed:

	1.	Arrays of values of a single *scalar type* are not flattened (e.g. an Array with only Numbers).
	2.	Nested Arrays--Arrays that contain other Arrays of *identical dimension*--are also not flattened.

Arrays of the first type are interpreted as either vectors (1D matrices) or *sets*.
An Array is interpreted as a set when and only when it contains *non-repeated*
String values.

BDQC interprets the second use of JSON_ Arrays as matrices. For example, in...

.. code-block:: JSON

        "foo.bar": {
            "baz": [
                [ 1, 2 ],
                [ 3, 4 ],
                [ 5, 6 ],
                [ 7, 8 ],
            ],
            "fuz": [
                [ [ "a", "b", "c", "d" ], [ "e", "f", "g", "h" ] ],
                [ [ "i", "j", "k", "l" ], [ "m", "n", "o", "p" ] ],
                [ [ "q", "r", "s", "t" ], [ "u", "v", "w", "x" ] ],
            ],
            "woz": [ "none","of","these","strings","are","repeated" ],
            ...
        }

1. foo.bar/baz will be treated as a 4x2 (numeric) matrix.
2. foo.bar/fuz will be treated as a 3x2x4 (String-valued) matrix.
3. foo.bar/woz will be treated as a *set*.

An Array that contains *any* JSON_ Objects is *always* further flattened.

Terms and Definitions
#####################

file analyzer
within-file analysis
between-file analysis
summary matrix
heuristic

Footnotes
#########

.. [#] `Alan Kay`_
.. [#] The bdqc.builtin.tabular plugin returns sets, vectors, and matrices.
.. [#] One use for compound-valued returns is passing arguments to a "downstream"
	(dependent) plugin.
.. [#] The type constraints are motivated partially by what the Python json module can serialize and partially by limitations in the definition of heuristics.


.. Collected external URLS

..	_Python3: https://wiki.python.org/moin/Python2orPython3
..	_`Unix philosophy`: https://en.wikipedia.org/wiki/Unix_philosophy
.. _`Alan Kay`: https://en.wikipedia.org/wiki/Alan_Kay
.. _JSON: http://json.org

.. Because it is intended to be be ``domain blind'' the analysis of a file
.. proceeds heuristally.

.. using a series of heuristics and
.. produces a single file summarizing the analysis (in JSON format).

.. Files can be specified in several ways including lists of directory trees
.. to be search recursively or manifests. Additionally filters can be specified
.. to refine the search.

.. Extractors
.. 	all first-level scalars are taken by default +
.. 	any others specified
.. 
.. Quantitative model-based (Gaussian) outlier detection
.. Categorical
.. 	unanimity
.. 	conditional unanimity
.. Ordinal
.. 	any missing value
.. Set-valued
.. 	identity
.. 
.. 
.. Scalar
.. 	Quantitative
.. 		robust outlier detection.
.. 	Categorical.
.. 		predefined rules
.. 			if >N% of values are identical, all should be
.. 			alert to any non-unanimity
.. 	Ordinal
.. 		essentially preclude outliers
.. Multi-valued
.. 	Quantitative
.. 		categorical values
.. 			all
.. 
.. Table analysis can be decomposed into
.. 	0. an upstream configuration requirement
.. 		"all categorical data to accumulate up to 23 labels (to capture chromosome)"
.. 	1. an extraction problem
.. 		"pull */tabledata/columns/labels out of all files' .bdqc"
.. 	2. an analysis
.. 		All sets should be identical
.. 
.. To understand what is possible via heuristic analysis one must first
.. understand a set of concepts on which it is based.
.. 
.. The columns of the matrix of aggregated summaries each have a type:
.. 
.. 	1. floating-point
.. 	2. integer
.. 	3. string
.. 	4. matrix descriptor
.. 	5. set (a 1-dimensional matrix of string values)
.. 
.. The first three represent scalar types, and a column (vector) of scalar
.. type can be assigned a *statistical class*:
.. 
.. 	1. quantitative (typically continuous values for which magnitude is meaningful)
.. 	2. ordinal (magnitude is meaningless, only order matters, typically 1..N exhaustive)
.. 	3. categorical (a small set of unordered values, boolean is a special case)
.. 
.. A column's statistical class constrains the set of candidate statistical
.. tests that might be applied.
.. 
.. The aggregate analysis consists of a set statistical techniques to
.. identify outliers in the *univariate* statistics produced by plugins.
.. By default, any file that is an outlier in any statistic is flagged as
.. potentially anomalous.

