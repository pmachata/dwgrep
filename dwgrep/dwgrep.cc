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
#include <getopt.h>
#include <map>

#include "libzwerg.h"
#include "libzwerg-dw.h"
#include "../libzwerg/std-memory.hh"

struct ext_argument
{
  char const *name;
  int type;

  static ext_argument required (char const *name)
  { return {required_argument, name}; }

  static ext_argument optional (char const *name)
  { return {optional_argument, name}; }

  static ext_argument no;

  static std::string
  shopt (int has_arg)
  {
    switch (has_arg)
      {
      case optional_argument:
	return "::";
      case required_argument:
	return ":";
      case no_argument:
	return "";
      };
    assert (has_arg != has_arg);
    abort ();
  }

  static std::string
  help (int has_arg, char const *name)
  {
    switch (has_arg)
      {
      case optional_argument:
	return std::string ("[=") + name + "]";
      case required_argument:
	return std::string ("=") + name;
      case no_argument:
	return "";
      };
    assert (has_arg != has_arg);
    abort ();
  }

private:
  ext_argument (int type, char const *name)
    : name {name}
    , type {type}
  {}
};

ext_argument ext_argument::no {no_argument, nullptr};

struct ext_shopt
{
  int const code;

  ext_shopt (char code)
    : code {code}
  {}

  ext_shopt ()
    : code {gencode ()}
  {}

  static std::string
  shopt (int c, int has_arg)
  {
    if (c < 256)
      {
	char cc = c;
	return std::string (&cc, 1) + ext_argument::shopt (has_arg);
      }
    else
      return "";
  }

  static std::string
  help (int c)
  {
    if (c < 256)
      return std::string ("-") + ((char) c);
    else
      return "";
  }

private:
  int
  gencode ()
  {
    static int last = 256;
    return last++;
  }
};

bool
operator== (int c, ext_shopt const &shopt)
{
  return c == shopt.code;
}

struct ext_option
  : public option
{
  char const *docstring;
  char const *arg_name;

  ext_option (ext_shopt shopt, const char *lopt, ext_argument arg,
	      char const *docstring)
    : option {lopt, arg.type, nullptr, shopt.code}
    , docstring {docstring}
    , arg_name {arg.name}
  {}

  std::string
  shopt () const
  {
    return ext_shopt::shopt (val, has_arg);
  }
};

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


  for (auto const &opt: opts)
    {
      std::cout << "  ";
      bool seen = false;
      for (auto const &l: opt.second.first)
	{
	  std::cout << (seen ? ", " : "") << l;
	  seen = true;
	}
      std::cout << "\n\t" << opt.second.second << "\n";
    }
}

int
main(int argc, char *argv[])
{
  setlocale (LC_ALL, "");
  textdomain ("dwgrep");

  ext_shopt help;
  std::vector <ext_option> ext_options = {
    {'q', "quiet", ext_argument::no,
     "Suppress all normal output."},
    {'q', "silent", ext_argument::no, ""},

    {'s', "no-messages", ext_argument::no,
     "Suppress error messages."},

    {'e', "expr", ext_argument::required ("EXPR"),
     "*EXPR* is a query to run."},

    {'c', "count", ext_argument::no,
     "Print only a count of query results, not the results themselves."},

    {'H', "with-filename", ext_argument::no,
     "Print the filename for each match."},

    {'h', "no-filename", ext_argument::no,
     "Suppress printing filename on output."},

    {'f', "file", ext_argument::required ("FILE"),
     "Load query from *FILE*."},

    {help, "help", ext_argument::no,
     "Show help and exit."},
  };

  std::unique_ptr <option[]> long_options = gen_options (ext_options);
  std::string options = gen_shopts (ext_options);

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
      int c = getopt_long (argc, argv, options.c_str (), long_options.get (), nullptr);
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
      if ((query = zw_query_parse (voc, *argv++, &err)) == nullptr)
	error_throw (err);
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

      if (fn != "")
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
