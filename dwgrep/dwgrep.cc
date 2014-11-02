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

#include <getopt.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <libintl.h>

#include "builtin-dw.hh"
#include "op.hh"
#include "parser.hh"
#include "stack.hh"
#include "tree.hh"
#include "value-dw.hh"

static void
show_help ()
{
  std::cout << "\
Typical usage: dwgrep FILE... -e PATTERN\n\
Searches for PATTER in FILEs.\n\
\n\
-e, --expr=EXPR		EXPR is a query to run\n\
-f, --file=FILE		load query from FILE\n\
\n\
-s, --no-messages	suppress error messages\n\
-q, --quiet, --silent	suppress all normal output\n\
    --verbose		show query parse in addition to normal output\n\
-H, --with-filename	print the filename for each match\n\
-h, --no-filename	suppress printing filename on output\n\
-c, --count		print only a count of query results\n\
\n\
    --help		this message\n\
";
}

int
main(int argc, char *argv[])
{
  setlocale (LC_ALL, "");
  textdomain ("dwlocstats");

  elf_version (EV_CURRENT);

  enum
  {
    verbose_flag = 257,
    help_flag,
  };

  static option long_options[] = {
    {"quiet", no_argument, nullptr, 'q'},
    {"verbose", no_argument, nullptr, verbose_flag},
    {"silent", no_argument, nullptr, 'q'},
    {"no-messages", no_argument, nullptr, 's'},
    {"expr", required_argument, nullptr, 'e'},
    {"count", no_argument, nullptr, 'c'},
    {"with-filename", no_argument, nullptr, 'H'},
    {"no-filename", no_argument, nullptr, 'h'},
    {"file", required_argument, nullptr, 'f'},
    {"help", no_argument, nullptr, help_flag},
    {nullptr, no_argument, nullptr, 0},
  };
  static char const *options = "ce:Hhqsf:O:";

  int verbosity = 0;
  bool no_messages = false;
  bool show_count = false;
  bool with_filename = false;
  bool no_filename = false;
  bool optimize = true;

  std::vector <std::string> to_process;

  builtin_dict builtins {*dwgrep_builtins_core (), *dwgrep_builtins_dw ()};

  tree query;
  bool seen_query = false;

  while (true)
    {
      int c = getopt_long (argc, argv, options, long_options, nullptr);
      if (c == -1)
	break;

      switch (c)
	{
	case 'e':
	  query = parse_query (builtins, optarg);
	  seen_query = true;
	  break;

	case 'c':
	  show_count = true;
	  break;

	case 'H':
	  with_filename = true;
	  break;

	case 'h':
	  no_filename = true;
	  break;

	case 'q':
	  verbosity = -1;
	  break;

	case verbose_flag:
	  verbosity = 1;
	  break;

	case help_flag:
	  show_help ();
	  return 0;

	case 's':
	  no_messages = true;
	  break;

	case 'f':
	  {
	    std::ifstream ifs {optarg};
	    std::string str {std::istreambuf_iterator <char> {ifs},
			     std::istreambuf_iterator <char> {}};
	    query = parse_query (builtins, str);
	    seen_query = true;
	    break;
	  }

	case 'O':
	  if (std::string {optarg} == "1")
	    optimize = true;
	  else if (std::string {optarg} == "0")
	    optimize = false;
	  else
	    {
	      std::cerr << "Unknown optimization level " << optarg << std::endl;
	      return 2;
	    }
	  break;

	default:
	  std::exit (2);
	}
    }

  argc -= optind;
  argv += optind;

  if (! seen_query)
    {
      if (argc == 0)
	{
	  std::cerr << "No query specified.\n";
	  return 2;
	}
      query = parse_query (builtins, *argv++);
      argc--;
    }

  if (optimize)
    query.simplify ();

  if (verbosity > 0)
    std::cerr << query << std::endl;

  if (argc == 0)
    {
      std::cerr << "No input files.\n";
      return 2;
    }
  else
    for (int i = 0; i < argc; ++i)
      to_process.push_back (argv[i]);

  if (to_process.size () > 1)
    with_filename = true;
  if (no_filename)
    with_filename = false;

  bool errors = false;
  bool match = false;
  for (auto const &fn: to_process)
    {
      auto stk = std::make_unique <stack> ();

      std::unique_ptr <value_dwarf> vdw;
      try
	{
	  vdw = std::make_unique <value_dwarf> (fn, 0);
	}
      catch (std::runtime_error e)
	{
	  if (! no_messages)
	    std::cout << "dwgrep: " << fn << ": " << e.what () << std::endl;
	  if (verbosity >= 0)
	    errors = true;
	  continue;
	}

      stk->push (std::move (vdw));
      auto upstream = std::make_shared <op_origin> (std::move (stk));
      auto program = query.build_exec (upstream);

      uint64_t count = 0;
      while (true)
	{
	  stack::uptr result;
	  try
	    {
	      result = program->next ();
	    }
	  catch (std::runtime_error e)
	    {
	      std::cerr << "dwgrep: " << fn << ": " << e.what () << std::endl;
	      break;
	    }

	  if (result == nullptr)
	    break;

	  // grep: Exit immediately with zero status if any match
	  // is found, even if an error was detected.
	  if (verbosity < 0)
	    std::exit (0);

	  match = true;
	  if (! show_count)
	    {
	      if (with_filename)
		std::cout << fn << ":\n";
	      if (result->size () > 1)
		std::cout << "---\n";
	      while (result->size () > 0)
		std::cout << *result->pop () << std::endl;
	    }
	  else
	    ++count;
	}

      if (show_count)
	{
	  if (with_filename)
	    std::cout << fn << ":";
	  std::cout << std::dec << count << std::endl;
	}
    }

  if (errors)
    std::exit (2);

  if (match)
    std::exit (0);
  else
    std::exit (1);
}
