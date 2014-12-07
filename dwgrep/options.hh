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

#include <cassert>
#include <getopt.h>
#include <map>
#include <string>
#include <tuple>
#include <vector>

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

inline bool
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

std::map <int, std::pair <std::vector <std::string>, std::string>>
merge_options (std::vector <ext_option> const &ext_opts);

extern ext_shopt help, version;
extern std::vector <ext_option> ext_options;
