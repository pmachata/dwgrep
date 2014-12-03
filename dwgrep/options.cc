/*
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
      if (entry.first.empty ())
	{
	  std::string sh = ext_shopt::help (opt.val);
	  if (sh != "")
	    entry.first.push_back (sh);
	}

      auto arg = ext_argument::help (opt.has_arg, opt.arg_name);
      entry.first.push_back (std::string ("--") + opt.name + arg);
      entry.second += opt.docstring;
    }

  return opts;
}

ext_shopt help;

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
};
