.. _index:

Welcome to dwgrep's documentation!
==================================

- :ref:`tutorial`

- :ref:`syntax`

- :ref:`vocabulary`

For complete table of contents, see :ref:`fulltoc`.


Introduction
============

Dwgrep is a tool, an associated language (called Zwerg) and a library
(libzwerg) for querying Dwarf (debuginfo) graphs.  If you want to find
out more about Dwarf, check out a `short introductory text`__ or
download a `Dwarf standard`__.  But you can also pretend that Dwarf is
like XML, except nodes are called DIE's.  That, and perusing the
output of ``eu-readelf -winfo``, should be enough to get you started.

.. __: http://www.dwarfstd.org/doc/Debugging%20using%20DWARF.pdf
.. __: http://dwarfstd.org/Download.php

You can think of dwgrep expressions as instructions describing a
path through a graph, with assertions about the type of nodes along
the way: that a node is of given type, that it has a given
attribute, etc.  There are also means of expressing sub-conditions,
i.e. assertions that a given node is acceptable if a separate
expression matches (or doesn't match) a different path through the
graph.

Apart from Dwarf objects (DIE's, Attributes and others), Zwerg
expressions can work with integers, strings, and sequences of other
objects.

In particular, a simple expression in dwgrep might look like this::

	entry ?DW_TAG_subprogram child ?DW_TAG_formal_parameter @DW_AT_name

On a `command line`__, you would issue it like this::

	$ dwgrep /some/file/somewhere -e 'entry ?DW_TAG_subprogram ...etc....'

.. __: XXX link to command line options.

The query itself says: show me values of attribute ``DW_AT_name`` of
``DW_TAG_formal_parameter`` nodes that are children of
``DW_TAG_subprogram`` entries (which here means debug information
entries, or DIE's).  Reading forward, you get list of instructions to
a matcher: take DIE's, accept all ``DW_TAG_subprogram``'s, look at
their children, accept those that are ``DW_TAG_formal_parameter``,
take value of attribute ``DW_AT_name``.

Another example comes from ``dwarflint``::

	entry ?DW_AT_decl_column !DW_AT_decl_line

This looks for DIE's that have ``DW_AT_decl_column``, but don't have
``DW_AT_decl_line``--a semantic violation that is worth reporting.


Computation
===========

Conceptually, Zwerg expressions form a pipeline of functions.  Much
like the shell pipeline, the individual functions take input and
produce output, which then becomes input of the next function in
chain.  Unlike shell pipelines, Zwerg expressions operate with values
such as Dwarf file, DIE, attribute, abbreviation, number, or string,
and in fact operate on stacks of these values.

Each function in pipeline thus takes on input a stack of values, and
produces zero or more stacks on the output.  Note that producing
several stacks is different from producing an element that holds a
single value of type sequence.  You can think of it as if any function
could in theory fork (if it returns more than one stack), or terminate
(if it doesn't return anything).

This may be best illustrated with an example.  Among the most trivial
Zwerg functions are literals, such as ``5`` or ``"Blah"``.  For a
stack given on input, they produce one stack on output, with the
indicated value pushed to top::

	$ dwgrep '"Hello, world!"'
	Hello, world!

Another very simple function is ``length``, which returns the input
stack with top value replaced with its length::

	$ dwgrep '"Hello, world!" length'
	13


Further reading
===============

Now would be a good time to read through the :ref:`tutorial`, which
gives a step-by-step account on fundamental tools of Zwerg language.
You might also want to look at :ref:`syntax`, where individual Zwerg
forms are introduced and described.

