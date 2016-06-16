########
Overview
########

Nothing in this directory is essential to *using* BDQC.
This is strictly a variety of tools for testing BDQC.

#############
Testing goals
#############

The structure of BDQC facilitates highly independent testing of capabilities
at a level between that of unit tests (of individual Python modules) and end-
to-end black box testing. There are three discrete areas:

1. Plugins_
2. `Within-file Analysis`
3. `Between-file Analysis`_ -- asdf
4. Reporting_

Plugins
=======

From the framework's point of view, Plugins need only:
1. export the right symbols. Only process is *required*.
2. produce results serializable as JSON.

Otherwise, plugins are independently testable.

Within-file Analysis
====================

This really means proper execution of plugin dependency chains.

1. Command line file filters operate correctly.

	1. wrt directory recursion
	2. wrt filename matching

2. Plugin dependency DAG is correctly managed.

	1. a plugin is skipped when its dependencies are missing
	2. 

Between-file Analysis
=====================

Insure true positives of each of 6 (total) output cases are reported:
1. No anomalies
2. Ambiguous types in plugin statistics
3. Incomparable files
4. Anomalies
	1. null stats
	2. quantitative outliers
	3. categorical outliers)

Reporting
=========

