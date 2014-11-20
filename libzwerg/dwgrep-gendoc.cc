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
#include <iomanip>
#include <sstream>

#include "builtin.hh"
#include "init.hh"
#include "builtin-dw.hh"

void underline (std::stringstream &ss, char c);

int
main (int argc, char const **argv)
{
  std::cout << ".. _vocabulary:\n\n"
    "Vocabulary reference\n"
    "====================\n\n";

  std::map <std::vector <std::string>, std::string> bis;

  {
    // A map from descriptions to operators that fit that description.
    // Typically this will be useful for pairing ?X to its matching !X
    // without really relying on the naming too much.
    std::map <std::string, std::vector <std::string>> ds;

    auto voc = dwgrep_vocabulary_core ();
    for (auto const &bi: voc->get_builtins ())
      {
	auto doc = bi.second->docstring ();
	if (doc != "@hide")
	  ds[doc].push_back (bi.first);
      }

    while (! ds.empty ())
      {
	bis.insert (std::make_pair (std::move (ds.begin ()->second),
				    std::move (ds.begin ()->first)));
	ds.erase (ds.begin ());
      }
  }

  for (auto const &bi: bis)
    {
      std::stringstream ss;
      bool seen = false;
      for (auto const &n: bi.first)
	{
	  if (seen)
	    ss << ", ";
	  seen = true;
	  ss << "``" << n << "``";
	}
      underline (ss, '=');
      std::cout << ss.str () << "\n"
		<< bi.second << "\n\n";
    }
}
