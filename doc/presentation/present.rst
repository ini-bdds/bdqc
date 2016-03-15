
=============================================
Big Data Quality Control
=============================================

----

Overview
----------------------------------------------------------------------------

BDQC is a Python3_ **framework** that **simplifies** identification of
**anomalous** files in large sets of files of **arbitrary type** that are
*a priori* presumed similar.

Intended to operate in concert with data processing pipelines that consume
and/or produce large sets of files.

.. _Python3: http://www.python.org
.. It is intended to operate on large sets of files that might be the input
.. to or intermediate or final results of a data processing pipeline.
.. Although optimal use requires the user provide input files that are
	known similar, we hold as a goal that it should be reasonable--and
	produce reasonable output--if pointed at almost anything. It will
	at least not stumble.

----

Scenarios
----------------------------------------------------------------------------

1. Preclude an expensive (in time or CPU charges) pipeline run by validating input files before run.
2. Detect bugs in (or aberrant termination of) a pipeline by validating (the file output of) intermediate or final stages.
	
.. frequently very generic "measurements" of a file suffice to detect trouble.

----

"Framework" because...
----------------------------------------------------------------------------

1. ...it knows *almost* nothing about specific file types.
2. ...it hosts plugins that analyze files and is therefore extensible to any type of file. 
3. ..."almost" nothing because it includes a set of "built-in plugins" that perform analyses meaningful for *all* file types in order to "bootstrap" analysis.
4.	Plugins are just Python packages.

	a) They may may be thin wrappers around off-the-shelf system executables.
	b) They may be Python/native code hybrids highly specialized for a particular analysis (e.g. colorspace summarization in image) files.

5.	Plugins declare their dependency on other plugins which allows large analyses to be decomposed into small *reusable* parts.
6.	Each plugin decides dynamically whether or not to run based on the output of its declared dependencies.


----

"Simplifies" because...
----------------------------------------------------------------------------

Core/built-in capabilities include:

1. easy specification of input files (filesystem recursion, in/exclusion patterns, manifests)
2. built-in heuristics and statistical analysis
3. can be run manually from the command line or itself incorporated into a pipeline
4. single self-contained-file HTML output reports suspicious files and reasons for suspicion, including graphical plots where appropriate.
5. basic analysis applicable to any/all files which serves to bootstrap special-purpose/type-specific analyses
6. explicit goal is to require as little configuration/as few arguments as possible.

.. builtins are a research Q
..  formalize handling of univariate statistical classes
.. Every tool should simplify a core set of tasks.

----

Runtime behavior
----------------------------------------------------------------------------

1. Determines a plugin execution order that respects the dependency structure of default and explicitly specified file-analysis plugins
2. Executes an analysis--that is, executes plugins in order--on each file separately. Each plugin decides for itself at runtime what, if anything, to do. **This step is "embarassingly parallel".**
3. Generates JSON_ results files stored "adjacent" to subject files.
4. Analyzes the statistics produced by the file-analysis plugins to identify *anomalies*
5. Optionally output HTML report summarizing anomalies and evidence

It's command-line executable as a standalone tool as well as programmatically
integratable into existing pipelines.

.. _JSON: http://www.json.org
.. Note results of #2 is not *necessarily* a table; rather a ragged table.
.. Obviously, #2 cries out for parallelism

----

"Anomalies
----------------------------------------------------------------------------

Currently three criteria (heuristics and statistics):

1. Because plugins "decide" whether or not to run based on their upstream dependencies' results, it may happen than all files in a run do not receive identical analyses. If a minority received different analyses in a set of files presumed similar they are considered likely anomalies.
2. A minority of files contain missing data
3. Outlier analysis of scalar statistics.

Note. **This is really where "research" enters the picture.**
	There is a large body of statistical work on outlier detection (both uni-
	and multivariate). In addition to this a variety of heuristics may apply
	more or less to different data types and pipelines. The goal is to
	**develop anomaly detection while preserving the clarity of purpose and
	ease of use of BDQC** as a *tool*.

..	1. A majority of files were analyzed by a particular set of plugins, but a minority was subject to a different set.

.. Configurability?

----

Future
----------------------------------------------------------------------------

The "research" aspect lies in determining what features/changes will be most
useful and informative.

1. "make"-like work minimization to allow follow-up runs to reuse earlier results when it can ascertain files are unchanged.
2. Add additional heuristics/statistical tests motivated by real use cases.
3. Increase control/parameterization of heuristic/statistical test application. (**Option bloat is a danger. Must be done in a way that preserves ease of use.)**
4. Currently only scalar statistics are analyzed, but plugins *may* produce arbitrarily complex output (e.g. set-valued). How should these be handled, or should they be prohibited?
5. Opportunities for integration with other tools (e.g. `Big Data Script`__).

.. _BDS: https://pcingola.github.io/BigDataScript/
.. __: BDS_




