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

#include <sstream>
#include <iomanip>

#include "docstring.hh"
#include "builtin.hh"

namespace
{
  void
  underline (std::stringstream &ss, char c)
  {
    size_t n = ss.str ().length ();
    ss << "\n" << std::setw (n + 1) << std::setfill (c) << "\n";
  }

  std::string
  format_selector (selector const &sel)
  {
    std::stringstream ss;
    auto vts = sel.get_types ();
    for (auto itb = vts.begin (), ite = vts.end (), it = itb; it != ite; ++it)
      {
	if (it != itb)
	  ss << " ";
	ss << "``" << *it << "``";
      }
    return ss.str ();
  }

  std::string
  format_head (selector const &sel, builtin_protomap const &pm)
  {
    std::stringstream ss;
    ss << format_selector (sel);

    // OVL now refers to an individual overload.  It ought to
    // contain at most one prototype.  It might contain none if
    // the prototype isn't or for whatever reason can't be
    // declared.
    assert (pm.size () <= 1);

    if (! pm.empty ())
      {
	auto const &pm0 = pm[0];
	switch (std::get <1> (pm0))
	  {
	  case yield::many:
	    ss << " ``->*``";
	    break;
	  case yield::once:
	    ss << " ``->``";
	    break;
	  case yield::maybe:
	    ss << " ``->?``";
	    break;
	  case yield::pred:
	    break;
	  }

	for (auto const &vt: std::get <2> (pm0))
	  ss << " ``" << vt << "``";
      }
    else
      ss << " ``-> ???``";

    return ss.str ();
  }

  std::string
  strip (std::string const &str, char const *whitespace)
  {
    std::string::size_type fst = str.find_first_not_of (whitespace, 0);
    if (fst == std::string::npos)
      return "";

    auto lst = str.find_last_not_of (whitespace);
    assert (lst != std::string::npos);
    return str.substr (fst, lst - fst + 1);
  }
}

std::pair <std::string, std::string>
format_overload (std::tuple <selector, std::shared_ptr <builtin>> const &ovl)
{
  builtin const &bi = *std::get <1> (ovl);

  // Trim new-lines at the beginning and at the end of docstring.
  // They may include these to make it clear what's the raw string
  // header, and what's the actual documentation.  We don't trim
  // spaces though, those are significant.
  return std::make_pair (format_head (std::get <0> (ovl), bi.protomap ()),
			 strip (bi.docstring (), "\n"));
}

std::vector <std::pair <std::vector <std::string>, std::string>>
doc_deduplicate (std::vector <std::pair <std::string, std::string>> &entries)
{
  // A map from descriptions to operators that fit that description.
  std::map <std::string, std::vector <std::string>> ds;
  while (! entries.empty ())
    {
      auto entry = std::move (entries.back ());
      entries.pop_back ();

      ds[std::move (entry.second)].push_back (std::move (entry.first));
    }
  entries.clear ();

  std::vector <std::pair <std::vector <std::string>, std::string>> ret;
  while (! ds.empty ())
    {
      ret.push_back (std::make_pair (std::move (ds.begin ()->second),
				     std::move (ds.begin ()->first)));
      ds.erase (ds.begin ());
    }
  return ret;
}

std::string
format_entry_map (std::vector <std::pair <std::vector <std::string>,
					  std::string>> entries,
		  char udc, std::string label)
{
  std::stringstream ret;

  for (auto const &entry: entries)
    {
      if (entry.second == "@hide")
	continue;

      if (label != "")
	{
	  for (auto const &n: entry.first)
	    ret << ".. _" << label << ' ' << n << ":\n";
	  ret << "\n";
	}

      std::stringstream ss;
      bool seen = false;
      for (auto const &n: entry.first)
	{
	  ss << (seen ? ", " : "") << n;
	  seen = true;
	}
      underline (ss, udc);
      ret << ss.str () << "\n" << entry.second << "\n\n";
    }

  return ret.str ();
}
