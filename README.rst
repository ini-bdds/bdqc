
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
programming) can extend its capabilities, and it is *intended to
be so extended*.

_`What is it for?`
==================

BDQC identifies anomalous files among large collections of files which are
*a priori* assumed to be "similar."
It was motivated by the realization that when faced with many thousands of
individual files it becomes challenging to even confirm they all contain
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
file's content (*within-file* analysis).
Second, a configurable set of heuristics is applied to the aggregated
file summaries (*across-file* analysis) to identify possible anomalies.

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

.. The third goal motivated the use of Python.

_`How does it work?`
####################

This section describes exhaustively how BDQC works internally.
This and following sections is required reading for plugin developers.

Analyze and summarize individual files (*within-file* analysis)
===============================================================

Possibly the most important thing to understand about BDQC is that
*plugins*, not the *framework*, carry out all analysis of input files.
The BDQC *framework* merely orchestrates the execution of *plugins*. [#]_

Said another way, the plugins that are executed on a file entirely determine
the content of the summary generated for that file. The framework itself
*never* looks inside a file; only the plugins.  The framework:

1. assembles a list of paths identifying files to be analyzed,
2. executes a *dynamically-determined* subset of the available plugins on each file path,
3. combines the executed plugins' results into (JSON_) summaries for each file.

Plugins are described more fully elsewhere. Here it suffices to understand
that each plugin can declare (as part of its implementation) that it depends
on zero or more other plugins.

The framework:

1. insures that a plugin's dependencies execute before the plugin itself, and
2. each plugin is provided with the results of its dependencies' execution.

Thus, the set of all *candidate* plugins--that is, all plugins installed on
the user's machine [#]_--constitute an implicit DAG (directed acyclic graph),
and a plugin that is "upstream" in the DAG can determine how (or even whether
or not) a downstream plugin is run.
The framework minimizes work by only executing a plugin when required.

By default, the summary for file foo.txt is left in an adjacent file named
foo.txt.bdqc.

Again, the BDQC *framework* does not touch files' content--it only
handles filenames.

Apply heuristics to aggregated summaries (*across-file* analysis)
=================================================================

1. Summary (\*.bdqc) files are collected.
2. The JSON_ content of all files' summaries is *flattened* into a matrix.
3. A specified set of heuristics are applied to the columns of the matrix.

The columns of the matrix are the individual statistics that plugins produce
in their analysis summaries.
Plugins_ are described more fully `elsewhere <Plugins_>`_. Here it suffices
to understand that a plugin's output can be (*almost*) anything
representable as JSON_ data.
Since JSON_ is capable of representing compound datatypes [#]_,
the individual statistics in plugins' summaries are identified by *paths*
in the JSON_ data. For example, in the following sample output from the
bdqc.builtin.tabular plugin...

.. code-block:: JSON

        "bdqc.builtin.tabular": {
            "character_histogram": {
                "CHAR": 626324,
                "CR": 0,
                "LF": 394,
                "UTF8/2": 0,
                "UTF8/3": 0,
                "UTF8/4": 0
            },
            "tabledata": {
                "aberrant_lines": 0,
                "column_count": 6,
                "column_separator": "\t",
                "columns": [
                    {
                        "labels": [
                            "foo",
                            "bar",
                            "baz"],
                        "max_labels_exceeded": false,
                        "stats": {
                            "mean": 0.0,
                            "variance":0.0 
                        },
                    },
                    {
                        "labels": [],
                        "max_labels_exceeded": false,
                        "stats": {
                            "mean": 142.454545,
                            "variance": 57562.872727
                        },
                    }
                ],
                "data_lines": 391,
                "empty_lines": 0,
                "meta_lines": 3,
                "metadata_prefix": "#",
                "separator_is_regex": false
            },

The mean value of the 2nd column is identified by the path:
	
	bdqc.builtin.tabular/tabledata/columns/1/stats/mean.

When summaries are aggregated and "flattened" individual columns in the resulting
matrix are named by such paths.
These paths can be used to make heuristic analysis selective.

**Advanced:** The process of flattening plugin summaries can be "tuned" by
limiting how deeply compound JSON_ types are traversed. By default, summaries
are traversed exhaustively, producing a scalar-type column for every scalar
statistic in the summaries.

Heuristic analysis
==================

Clearly, it is not generally possible to reliably identify "anomalies"
(classify samples) without **some** *a priori* knowledge of the "normal."
BDQC's goals are slightly less ambitious: to bootstrap classification
heuristically. Moreover, it aims to be easy to
(learn to) use by doing *something* reasonable with very little
guidance--that is, by having sensible default behavior.

To understand what is possible via heuristic analysis one must first
understand a set of concepts on which it is based.

The columns of the matrix of aggregated summaries each have a type:

	1. floating-point
	2. integer
	3. string
	4. matrix descriptor
	5. set (a 1-dimensional matrix of string values)

The first three represent scalar values, and a column (vector) of scalar
values can be associated with a statistical class.

	1. quantitative (typically continuous values for which magnitude is meaningful)
	2. ordinal (magnitude is meaningless, only order matters, typically 1..N exhaustive)
	3. categorical (a small set of unordered values, boolean is a special case)

.. The aggregate analysis consists of a set statistical techniques to
.. identify outliers in the *univariate* statistics produced by plugins.
.. By default, any file that is an outlier in any statistic is flagged as
.. potentially anomalous.

_`Plugins`
##########

To reiterate, the BDQC executable *framework* does not touch
files itself. All file analysis is carried out by plugins.
Several plugins are included in but, nonetheless, distinct from the
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

Plugins must observe several constrains (some shown in the example):

1. Every plugin *must* provide a list called DEPENDENCIES (which may be empty). Each dependency is a fully-qualified Python package name.
2. Every plugin *must* provide a two-argument function called process.
3. A plugin *may* include a VERSION declaration. If present, it must be convertible to an integer (using int()).
4. The process function *must* return one of the basic Python type dict, list, tuple, scalar, or None [#]_.

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
This promotes reuse, extensibility, unit-testability of plugins, and is
the motivation behind the plugin architecture.

There is no provision for passing arguments to plugins from the framework
itself. Environment variables can be used when a plugin must be
parameterized. [#]_

Developers are advised to look at the source code of any of the built-in
plugins for examples of how to write their own. The bdqc.builtin.extrinsic
is a very simple plugin; bdqc.builtin.tabular is much more complex and
demonstrates how to use C code.

The framework will incorporate the VERSION number into the plugin's output
automatically. The plugin's code need not and should not include it in the
returned value. The version number is used by the framework to decide
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

bdqc.buildtin.extrinsic
-----------------------

.. warning:: Unfinished.

bdqc.buildtin.filetype
----------------------

.. warning:: Unfinished.

bdqc.buildtin.tabular
---------------------

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

Footnotes
#########

.. [#] `Alan Kay`_
.. [#] The BDQC *package* includes several "built-in" plugins which insure
	it is useful "out of the box." Though they are built-in, they are
	nonetheless plugins.
.. [#] ...and "visible" to the BDQC framework by virtue of PYTHONPATH. 
.. [#] JSON_ "Objects" can contain anything including other Objects. Similarly, JSON_ Arrays can contain Arrays.
.. [#] The type constraints are motivated partially by what the Python json module can serialize and partially by limitations in the definition of heuristics.
.. [#] One use for set-valued returns is passing arguments to a "downstream"
	(dependent) plugin.}


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
