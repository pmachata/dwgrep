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

#include <iostream>
#include <fstream>
#include <memory>
#include <libintl.h>
#include <vector>
#include <cassert>
#include <getopt.h>
#include <map>

#include "libzwerg.h"
#include "libzwerg-dw.h"
#include "options.hh"
#include "libzwerg/std-memory.hh"
#include "libzwerg/strip.hh"

std::unique_ptr <option[]>
gen_options (std::vector <ext_option> const &ext_opts)
{
  size_t sz = ext_opts.size ();
  auto ret = std::make_unique <option[]> (sz + 1);

  for (size_t i = 0; i < sz; ++i)
    // Slice off the extra stuff.
    ret[i] = ext_opts[i];

  ret[sz] = (struct option) {0, 0, 0, 0};
  return ret;
}

std::string
gen_shopts (std::vector <ext_option> const &ext_opts)
{
  std::string ret;
  for (auto const &ext_opt: ext_opts)
    ret += ext_opt.shopt ();
  return ret;
}

void
show_help (std::vector <ext_option> const &ext_opts)
{
  std::cout << "Usage:\n  dwgrep FILE... -e PATTERN\n\n"
	    << "Options:\n";

  std::map <int, std::pair <std::vector <std::string>, std::string>> opts
    = merge_options (ext_opts);

  for (auto const &opt: opts)
    {
      std::cout << "  ";
      bool seen = false;
      for (auto const &l: opt.second.first)
	{
	  std::cout << (seen ? ", " : "") << l;
	  seen = true;
	}

      std::string ds = strip (opt.second.second, " \t\n");
      auto period = ds.find_first_of (".");
      std::cout << "\n\t" << ds.substr (0, period) << "\n";
    }
}

int
main(int argc, char *argv[])
{
  setlocale (LC_ALL, "");
  textdomain ("dwgrep");

  std::unique_ptr <option[]> long_options = gen_options (ext_options);
  std::string options = gen_shopts (ext_options);

  int verbosity = 0;
  bool no_messages = false;
  bool show_count = false;
  bool with_filename = false;
  bool no_filename = false;

  std::vector <std::string> to_process;

  zw_error *err;
  std::shared_ptr <zw_vocabulary> voc (zw_vocabulary_init (&err),
				       &zw_vocabulary_destroy);
  assert (voc != nullptr); // XXX

  zw_vocabulary const *voc_core = zw_vocabulary_core (&err);
  assert (voc_core != nullptr); // XXX

  zw_vocabulary const *voc_dw = zw_vocabulary_dwarf (&err);
  assert (voc_dw != nullptr); // XXX

  auto die = [] (zw_error *err)
    {
      std::cerr << "Error: " << zw_error_message (err) << std::endl;
      return 2;
    };

  if (! zw_vocabulary_add (&*voc, voc_core, &err)
      || ! zw_vocabulary_add (&*voc, voc_dw, &err))
    return die (err);

  std::shared_ptr <zw_query> query;

  while (true)
    {
      int c = getopt_long (argc, argv, options.c_str (), long_options.get (), nullptr);
      if (c == -1)
	break;

      switch (c)
	{
	case 'e':
	  if (zw_query *q = zw_query_parse (&*voc, optarg, &err))
	    query.reset (q, &zw_query_destroy);
	  else
	    return die (err);
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

	case 's':
	  no_messages = true;
	  break;

	case 'f':
	  {
	    std::ifstream ifs {optarg};
	    std::string str {std::istreambuf_iterator <char> {ifs},
			     std::istreambuf_iterator <char> {}};
	    if (zw_query *q = zw_query_parse_len (&*voc, str.c_str (),
						  str.length (), &err))
	      query.reset (q, &zw_query_destroy);
	    else
	      return die (err);
	    break;
	  }

	default:
	  if (c == help)
	    {
	      show_help (ext_options);
	      return 0;
	    }

	  return 2;
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
      if (zw_query *q = zw_query_parse (&*voc, *argv++, &err))
	query.reset (q, &zw_query_destroy);
      else
	return die (err);
      argc--;
    }

  if (argc == 0)
    // No input files.
    to_process.push_back ("");
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
      std::shared_ptr <zw_stack> stack (zw_stack_init (&err),
					&zw_stack_destroy);
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

      if (fn != "")
	{
	  zw_value *dwv = zw_value_init_dwarf (fn.c_str (), 0, &err);
	  if (dwv == nullptr
	      || ! zw_stack_push_take (&*stack, dwv, &err))
	    {
	      zw_value_destroy (dwv);
	      goto fail;
	    }
	}

      std::shared_ptr <zw_result> result
	(zw_query_execute (&*query, &*stack, &err),
	 &zw_result_destroy);
      if (result == nullptr)
	goto fail;

      uint64_t count = 0;
      while (true)
	{
	  zw_stack *out;
	  if (! zw_result_next (&*result, &out, &err))
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
	    return 0;

	  match = true;
	  if (! show_count)
	    {
	      if (with_filename)
		std::cout << fn << ":\n";
	      if (zw_stack_depth (out) > 1)
		std::cout << "---\n";
	      if (! zw_stack_dump_xxx (out, &err))
		return die (err);
	    }
	  else
	    ++count;

	  zw_stack_destroy (out);
	}

      if (show_count)
	{
	  if (with_filename)
	    std::cout << fn << ":";
	  std::cout << std::dec << count << std::endl;
	}
    }

  if (errors)
    return 2;

  return match ? 0 : 1;
}
