.. _cli:

Command line reference
======================

Synopsis
--------

::

	dwgrep [OPTIONS] PATTERN [FILE...]
	dwgrep [OPTIONS] [-e PATTERN | -f FILE] [FILE...]

Description
-----------

Dwgrep is a tool for querying Dwarf (debuginfo) graphs.  Queries are
written in a language called Zwerg, the syntax of which is described
in :ref:`syntax`.  The engine itself is available in a shared library
which can be used from C.

Options
-------

.. include:: options.txt

Examples
--------

Find ELF files::

	$ dwgrep -sh $(find /usr/lib64/ -type f) -e 'name'
	/usr/lib64/libogrove.so.0.0.1
	/usr/lib64/libmp3lame.so.0.0.0
	/usr/lib64/libgimpcolor-2.0.so.0.600.12
	/usr/lib64/libmx-gtk-1.0.so.0.0.0
	/usr/lib64/libkpimidentities.so.4.6.0
	[... etc ...]

Find ELF files that include Dwarf information::

	$ dwgrep -sh $(find /usr/lib64/ -type f) -e '?(unit) name'
	/usr/lib64/python3.2/config-3.2mu/python.o
	/usr/lib64/libxqilla.so.5.0.4
	[... etc ...]

Find namespace names defined in one of them::

	$ dwgrep /usr/lib64/libxqilla.so.5.0.4 -e '
		entry ?TAG_namespace name' | sort -u
	__debug
	__detail
	__gnu_cxx
	__gnu_debug
	std
	xercesc_3_0
	XQParser

Find names of variables defined in the namespace ``xercesc_3_0``::

	$ dwgrep /usr/lib64/libxqilla.so.5.0.4 -e '
		entry ?TAG_namespace (name == "xercesc_3_0")
		child ?TAG_variable name' | sort -u
	chAmpersand
	chAsterisk
	chAt
	chBackSlash
	chBang
  	[... etc ...]

Of those, only list the ones that don't start in ``ch``::

	$ dwgrep /usr/lib64/libxqilla.so.5.0.4 -e '
		entry ?TAG_namespace (name == "xercesc_3_0")
		child ?TAG_variable name
		(!~ "ch.*")' | sort -u
	gControlCharMask
	gDefOutOfMemoryErrMsg
	gFirstNameCharMask
	gNameCharMask

Look where they are declared::

	$ dwgrep /usr/lib64/libxqilla.so.5.0.4 -e '
		entry ?TAG_namespace (name == "xercesc_3_0")
		child ?TAG_variable (name !~ "ch.*")
		@AT_decl_file' | sort -u
	/usr/include/xercesc/util/OutOfMemoryException.hpp
	/usr/include/xercesc/util/XMLChar.hpp

Use formatting strings to include line number information in the mix::

	$ dwgrep /usr/lib64/libxqilla.so.5.0.4 -e '
		entry ?TAG_namespace (name == "xercesc_3_0")
		child ?TAG_variable (name !~ "ch.*")
		"%(@AT_decl_file%): %(dup @AT_decl_line%)"' | sort -u
	/usr/include/xercesc/util/OutOfMemoryException.hpp: 32
	/usr/include/xercesc/util/XMLChar.hpp: 33
	/usr/include/xercesc/util/XMLChar.hpp: 34
	/usr/include/xercesc/util/XMLChar.hpp: 35
	[... etc ...]

More examples are available in documentation on syntax and in the
tutorial.

See also
--------

To learn more about Dwarf, check out:

- `Introduction to the DWARF Debugging Format`__
- `Dwarf standard`__
- `dwgrep`__ project homepage

.. __: http://www.dwarfstd.org/doc/Debugging%20using%20DWARF.pdf
.. __: http://dwarfstd.org/Download.php
.. __: https://github.com/pmachata/dwgrep
