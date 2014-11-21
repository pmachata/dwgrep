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
#include <algorithm>

#include "builtin.hh"
#include "init.hh"
#include "builtin-dw.hh"
#include "docstring.hh"

void underline (std::stringstream &ss, char c);

int
main (int argc, char const **argv)
{
  std::cout << ".. _vocabulary:\n\n"
    "Vocabulary reference\n"
    "====================\n\n";


  std::vector <std::pair <std::string, std::string>> entries;

  auto bis = dwgrep_vocabulary_core ()->get_builtins ();
  std::transform (bis.begin (), bis.end (), std::back_inserter (entries),
		  [] (std::pair <std::string,
				 std::shared_ptr <builtin const>> const &bi)
		  {
		    return std::make_pair (bi.first, bi.second->docstring ());
		  });

  entries.erase
    (std::remove_if (entries.begin (), entries.end (),
		     [] (std::pair <std::string, std::string> const &p)
		     {
		       return p.second == "@hide";
		     }),
     entries.end ());

  auto dentries = doc_deduplicate (entries);

  for (auto &dentry: dentries)
    std::sort (dentry.first.begin (), dentry.first.end ());

  std::sort (dentries.begin (), dentries.end (),
	     [] (std::pair <std::vector <std::string>, std::string> const &p1,
		 std::pair <std::vector <std::string>, std::string> const &p2)
	     {
	       return p1.first < p2.first;
	     });

  std::cout << format_entry_map (dentries, '-');

}
