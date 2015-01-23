/*
   Copyright (C) 2014, 2015 Red Hat, Inc.
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

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstring>
#include <fstream>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <libintl.h>
#include <map>
#include <memory>
#include <vector>

#include "libzwerg.hh"
#include "libzwerg-dw.h"
#include "options.hh"
#include "libzwerg/std-memory.hh"
#include "libzwerg/strip.hh"
#include "version.h"
#include "libzwerg/flag_saver.hh"

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
    {
      auto shopt = ext_opt.shopt ();
      if (std::find (ret.begin (), ret.end (), shopt[0]) == ret.end ())
	ret += shopt;
    }
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

void dump_value (std::ostream &os, zw_value const &val, bool outer);

void
dump_err (zw_error *err)
{
  assert (err != nullptr);
  std::cerr << "Error: " << zw_error_message (err) << std::endl;
}

void
dump_const (std::ostream &os, zw_value const &val, bool)
{
  zw_error *err;
  if (std::unique_ptr <zw_value, zw_deleter> str
	{zw_value_const_format (&val, &err)})
    dump_value (os, *str.get (), true);
  else
    dump_err (err);
}

void
dump_charp (std::ostream &os, char const *buf, size_t len, bool outer)
{
  if (outer)
    os << std::string {buf, len};
  else
    {
      os << '"';
      for (size_t i = 0; i < len; ++i)
	switch (buf[i])
	  {
#define ESCAPE(L, E) case L: os << E; break

	    ESCAPE (0, "\\0");
	    ESCAPE ('"', "\\");
	    ESCAPE ('\\', "\\\\");
	    ESCAPE ('\a', "\\a");
	    ESCAPE ('\b', "\\b");
	    ESCAPE ('\t', "\\t");
	    ESCAPE ('\n', "\\n");
	    ESCAPE ('\v', "\\v");
	    ESCAPE ('\f', "\\f");
	    ESCAPE ('\r', "\\r");

#undef ESCAPE

	  default:
	    if (isprint (buf[i]))
	      os << buf[i];
	    else
	      {
		ios_flag_saver ifs {os};
		os << "\\x" << std::hex << std::setw (2)
		   << (unsigned) (unsigned char) buf[i];
	      }
	  }
      os << '"';
    }
}

void
dump_string (std::ostream &os, zw_value const &val, bool outer)
{
  size_t len;
  char const *buf = zw_value_str_str (&val, &len);
  dump_charp (os, buf, len, outer);
}

void
dump_seq (std::ostream &os, zw_value const &val, bool)
{
  os << "[";
  for (size_t n = zw_value_seq_length (&val), i = 0; i < n; ++i)
    {
      if (i > 0)
	os << ", ";
      zw_value const *emt = zw_value_seq_at (&val, i);
      assert (emt != nullptr);
      dump_value (os, *emt, false);
    }
  os << "]";
}

void
dump_dwarf (std::ostream &os, zw_value const &val, bool)
{
  os << "<Dwarf ";
  char const *name = zw_value_dwarf_name (&val);
  dump_charp (os, name, strlen (name), false);
  os << '>';
}

void
dump_cu (std::ostream &os, zw_value const &val, bool)
{
  ios_flag_saver ifs {os};
  os << "<CU " << std::hex << std::showbase << zw_value_cu_offset (&val) << ">";
}

void
dump_die (std::ostream &os, zw_value const &val, bool outer)
{
  Dwarf_Die die = zw_value_die_die (&val);
  os << "DIE";
}

void
dump_value (std::ostream &os, zw_value const &val, bool outer)
{
  if (zw_value_is_const (&val))
    dump_const (os, val, outer);
  else if (zw_value_is_str (&val))
    dump_string (os, val, outer);
  else if (zw_value_is_seq (&val))
    dump_seq (os, val, outer);
  else if (zw_value_is_dwarf (&val))
    dump_dwarf (os, val, outer);
  else if (zw_value_is_cu (&val))
    dump_cu (os, val, outer);
  else if (zw_value_is_die (&val))
    dump_die (os, val, outer);
  else
    os << "<unknown value type>";
}

int
main(int argc, char *argv[])
try
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

    std::unique_ptr <zw_vocabulary, zw_deleter> voc
    {zw_vocabulary_init (zw_throw_on_error {})};
    zw_vocabulary const *voc_core = zw_vocabulary_core (zw_throw_on_error {});
    zw_vocabulary const *voc_dw = zw_vocabulary_dwarf (zw_throw_on_error {});

    zw_vocabulary_add (voc.get (), voc_core, zw_throw_on_error {});
    zw_vocabulary_add (voc.get (), voc_dw, zw_throw_on_error {});

    bool query_specified = false;
    std::string query_str;

    while (true)
      {
	int c = getopt_long (argc, argv, options.c_str (),
			     long_options.get (), nullptr);
	if (c == -1)
	  break;

	switch (c)
	  {
	  case 'e':
	    query_str += optarg;
	    query_specified = true;
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
	      auto buf_to_string = [] (std::istream &is)
		{
		  return std::string {std::istreambuf_iterator <char> {is},
				      std::istreambuf_iterator <char> {}};
		};

	      if (strcmp (optarg, "-") != 0)
		{
		  std::ifstream ifs {optarg};
		  if (ifs.fail ())
		    {
		      std::cerr << "Error: can't open script file `"
				<< optarg << "'.\n";
		      return 2;
		    }
		  query_str += buf_to_string (ifs);
		}
	      else
		query_str += buf_to_string (std::cin);
	      query_specified = true;
	      break;
	    }

	  default:
	    if (c == help)
	      {
		show_help (ext_options);
		return 0;
	      }
	    else if (c == version)
	      {
		std::cout << "dwgrep "
			  << DWGREP_VERSION_MAJOR << "."
			  << DWGREP_VERSION_MINOR << std::endl;
		return 0;
	      }

	    return 2;
	  }
      }

    argc -= optind;
    argv += optind;

    std::unique_ptr <zw_query, zw_deleter> query {
	[&] ()
	  {
	    if (query_specified)
	      return zw_query_parse_len (voc.get (), query_str.c_str (),
					 query_str.length (),
					 zw_throw_on_error {});
	    else
	      {
		if (argc == 0)
		  throw std::runtime_error ("No query specified.");

		argc--;
		return zw_query_parse (&*voc, *argv++, zw_throw_on_error {});
	      }
	  } ()};

    std::vector <char const *> to_process;
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
      try
	{
	  std::unique_ptr <zw_stack, zw_deleter> stack
		{zw_stack_init (zw_throw_on_error {})};

	  if (fn[0] != '\0')
	    {
	      std::unique_ptr <zw_value, zw_deleter> dwv
			{zw_value_init_dwarf (fn, 0, zw_throw_on_error {})};

	      zw_stack_push_take (stack.get (), dwv.get (),
				  zw_throw_on_error {});
	      dwv.release ();
	    }

	  std::unique_ptr <zw_result, zw_deleter> result
		{zw_query_execute (query.get (), stack.get (),
				   zw_throw_on_error {})};

	  uint64_t count = 0;
	  while (auto out = zw_result_next (*result))
	    {
	      // grep: Exit immediately with zero status if any match
	      // is found, even if an error was detected.
	      if (verbosity < 0)
		return 0;

	      match = true;
	      if (! show_count)
		{
		  if (with_filename)
		    std::cout << fn << ":\n";
		  if (zw_stack_depth (out.get ()) > 1)
		    std::cout << "---\n";
		  for (size_t i = 0, n = zw_stack_depth (out.get ());
		       i < n; ++i)
		    {
		      auto const *val = zw_stack_at (out.get (), i);
		      assert (val != nullptr);
		      dump_value (std::cout, *val, true);
		      std::cout << std::endl;
		    }
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
      catch (std::runtime_error const &e)
	{
	  if (! no_messages)
	    std::cout << "dwgrep: " << (fn[0] != '\0' ? fn : "<no-file>")
		      << ": " << e.what () << std::endl;

	  if (verbosity >= 0)
	    errors = true;

	  continue;
	}
      catch (...)
	{
	  std::cout << "blah\n";
	  continue;
	}

    if (errors)
	return 2;

    return match ? 0 : 1;
  }
catch (std::runtime_error const& e)
  {
    std::cout << "dwgrep: " << e.what () << std::endl;
    return 2;
  }
