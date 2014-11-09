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
#include <vector>
#include <cassert>

#include "libzwerg.h"


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
  textdomain ("dwgrep");

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

  std::vector <std::string> to_process;

  zw_error *err;
  zw_vocabulary *voc = zw_vocabulary_init (&err);
  assert (voc != nullptr); // XXX

  zw_vocabulary const *voc_core = zw_vocabulary_core (&err);
  assert (voc_core != nullptr); // XXX

  zw_vocabulary const *voc_dw = zw_vocabulary_dwarf (&err);
  assert (voc_dw != nullptr); // XXX

  auto error_throw = [] (zw_error *err)
    {
      std::cerr << "Error: " << zw_error_message (err) << std::endl;
      std::exit (1);
    };

  if (! zw_vocabulary_add (voc, voc_core, &err)
      || ! zw_vocabulary_add (voc, voc_dw, &err))
    error_throw (err);

  zw_query *query = nullptr;

  while (true)
    {
      int c = getopt_long (argc, argv, options, long_options, nullptr);
      if (c == -1)
	break;

      switch (c)
	{
	case 'e':
	  if ((query = zw_query_parse (voc, optarg, &err)) == nullptr)
	    error_throw (err);
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
	    if ((query = zw_query_parse_len
		 (voc, str.c_str (), str.length (), &err)) == nullptr)
	      error_throw (err);
	    break;
	  }

	default:
	  std::exit (2);
	}
    }

  argc -= optind;
  argv += optind;

  if (query == nullptr)
    {
      if (argc == 0)
	{
	  std::cerr << "No query specified.\n";
	  return 2;
	}
      if ((query = zw_query_parse (voc, *argv++, &err)) == nullptr)
	error_throw (err);
      argc--;
    }

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
      zw_stack *stack = zw_stack_init (&err);
      if (stack == nullptr)
	{
	fail:
	  if (! no_messages)
	    std::cout << "dwgrep: " << fn << ": "
		      << zw_error_message (err) << std::endl;
	  zw_error_destroy (err);
	  if (verbosity >= 0)
	    errors = true;
	  continue;
	}

      {
	zw_value *dwv = zw_value_init_dwarf (fn.c_str (), &err);
	if (dwv == nullptr
	    || ! zw_stack_push_take (stack, dwv, &err))
	  goto fail;
      }

      zw_result *result = zw_query_execute (query, stack, &err);
      if (result == nullptr)
	goto fail;

      uint64_t count = 0;
      while (true)
	{
	  zw_stack *out;
	  if (! zw_result_next (result, &out, &err))
	    {
	      if (! no_messages)
		std::cerr << "dwgrep: " << fn << ": "
			  << zw_error_message (err) << std::endl;
	      zw_error_destroy (err);
	      break;
	    }
	  if (out == nullptr)
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
	      if (zw_stack_depth (out) > 1)
		std::cout << "---\n";
	      if (! zw_stack_dump_xxx (out, &err))
		error_throw (err);
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
