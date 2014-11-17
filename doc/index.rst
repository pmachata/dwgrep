.. dwgrep documentation master file, created by
   sphinx-quickstart on Mon Nov 17 01:52:44 2014.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

Welcome to dwgrep's documentation!
==================================

Contents:

.. toctree::
   :maxdepth: 2

   test


Introduction
============

Dwgrep is a tool, an associated language (called Zwerg) and a library
(libzwerg) for querying Dwarf (debuginfo) graphs.  If you want to find
out more about Dwarf, check out a `short introductory text`_ or
download a `Dwarf standard`_.  But you can also pretend that Dwarf is
like XML, except nodes are called DIE's.  That, and perusing the
output of eu-readelf -winfo, should be enough to get you started.

.. _short introductory text: http://www.dwarfstd.org/doc/Debugging%20using%20DWARF.pdf
.. _Dwarf standard: http://dwarfstd.org/Download.php

You can think of dwgrep expressions as instructions describing a
path through a graph, with assertions about the type of nodes along
the way: that a node is of given type, that it has a given
attribute, etc.  There are also means of expressing sub-conditions,
i.e. assertions that a given node is acceptable if a separate
expression matches (or doesn't match) a different path through the
graph.

Apart from Dwarf objects (DIE's, Attributes and others), dwgrep
expressions can work with integers, strings, and sequences of other
objects.

In particular, a simple expression in dwgrep might look like this::

	entry ?DW_TAG_subprogram child ?DW_TAG_formal_parameter @DW_AT_name

On a commad line, you would issue it like this::


	$ dwgrep /some/file/somewhere -e 'entry ?DW_TAG_subprogram ...etc....'

The query itself (ignoring the initial colon that's part of
meta-syntax) says: show me values of attribute DW_AT_name of
DW_TAG_formal_parameter nodes that are children of DW_TAG_subprogram
entries (which here means debug information entries, or DIE's).
Reading forward, you get list of instructions to a matcher: take
DIE's, accept all DW_TAG_subprogram's, look at their children,
accept those that are DW_TAG_formal_parameter, take value of
attribute DW_AT_name.

Another example comes from dwarflint::

	entry ?DW_AT_decl_column !DW_AT_decl_line

This looks for DIE's that have DW_AT_decl_column, but don't have
DW_AT_decl_line--a semantic violation that is worth reporting.



Indices and tables
==================

* :ref:`genindex`
* :ref:`search`

