/*
   Copyright (C) 2017 Petr Machata
   Copyright (C) 2014 Red Hat, Inc.
   This file is part of dwgrep.

   This file is free software; you can redistribute it and/or modify
   it under the terms of either

     * the GNU Lesser General Public License as published by the Free
       Software Foundation; either version 3 of the License, or (at
       your option) any later version

   or

     * the GNU General Public License as published by the Free
       Software Foundation; either version 2 of the License, or (at
       your option) any later version

   or both in parallel, as here.

   dwgrep is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received copies of the GNU General Public License and
   the GNU Lesser General Public License along with this program.  If
   not, see <http://www.gnu.org/licenses/>.  */

#include "options.hh"

ext_argument ext_argument::no {no_argument, nullptr};

std::map <int, std::pair <std::vector <std::string>, std::string>>
merge_options (std::vector <ext_option> const &ext_opts)
{
  std::map <int, std::pair <std::vector <std::string>, std::string>> opts;

  for (auto const &opt: ext_opts)
    {
      auto &entry = opts[opt.val];
      auto arg = ext_argument::help (opt.has_arg, opt.arg_name);
      if (entry.first.empty ())
	{
	  std::string sh = ext_shopt::help (opt.val);
	  if (sh != "")
	    entry.first.push_back (sh + (opt.name == nullptr ? arg : ""));
	}

      if (opt.name != nullptr)
	entry.first.push_back (std::string ("--") + opt.name + arg);
      entry.second += opt.docstring;
    }

  return opts;
}

ext_shopt help, version, longarg;

std::vector <ext_option> ext_options = {
  {'q', "silent", ext_argument::no, ""},
  {'q', "quiet", ext_argument::no, R"docstring(

	Suppress all normal output.  Exit immediately with zero status
	if any match is found, even if an error was detected.

)docstring"},

  {'s', "no-messages", ext_argument::no, R"docstring(

	Suppress error messages.  All normal output is produced, but
	error messages (if any) are not shown.

	Note that currently libzwerg produces some error messages on
	its own (e.g. division by zero), and those are still
	displayed.

)docstring"},

  {'e', "expr", ext_argument::required ("EXPR"), R"docstring(

	*EXPR* is a query to run.  At most one ``-e`` or ``-f`` option
	shall be present.  The selected query is run over the input
	file(s).

)docstring"},

  {'c', "count", ext_argument::no,  R"docstring(

	Print only a count of query results, not the results
	themselves.

)docstring"},

  {'H', "with-filename", ext_argument::no,  R"docstring(

	Print the filename for each match.  This is the default when
	there is more than one file to search.

)docstring"},

  {'h', "no-filename", ext_argument::no,  R"docstring(

	Suppress printing filename on output.  This is the default
	when there is less than two files to search.

)docstring"},

  {'f', "file", ext_argument::required ("FILE"), R"docstring(

	Load query from *FILE*.  A Zwerg script stored in the given
	file is read and run over the input file(s).  At most one
	``-e`` or ``-f`` option shall be present.

)docstring"},

  {help, "help", ext_argument::no, R"docstring(

	Show help and exit.

)docstring"},

  {version, "version", ext_argument::no, R"docstring(

	Show version in the format MAJOR.MINOR and exit.

)docstring"},

  {longarg, "a", ext_argument::required ("ARG"), R"docstring(

	Pass an evaluated argument to the running script. The argument is parsed
	as a Zwerg program and interpreted on an empty stack. TOS of each yielded
	stack is taken and pushed to program stack before query is executed.

        Formally, a command line such as this: ``dwgrep -e PROG --a X --a Y ...``
	is interpreted like the following Zwerg snippet::

		let .X := *X*;
		let .Y := *Y*;
		...
		.X .Y *PROG*

	In particular this means that if an argument yields more than once, the
	whole following computation is "forked" and in each branch the
	argument has a different value.

	ELF files mentioned on the command line are passed as the first command
	line argument. I.e. a dwgrep invocation such as this::

		dwgrep -e PROG --a X --a Y ... file1 file2 ...

	Is handled as follows::

		dwgrep -e PROG --a '("file1", "file2", ...) dwopen' --a X --a Y ...

	Thus one can bind the explicitly-mentioned command line arguments to
	named variables and leave the ELF files themselves as implicit argument
	of the expression itself, as is usual when writing argument-less
	queries. Consider::

		dwgrep file1 file2 -e 'entry foo bar baz'
		dwgrep file1 file2 --a A --a B -e '(|A B| entry foo bar baz)'

)docstring"},

  {'a', nullptr, ext_argument::required ("ARG"), R"docstring(

	Pass a string argument to the running script. A command line argument
	such as ``-a X`` is handled like ``--a '"X"'``, except with appropriate
	escaping.

)docstring"},
};
