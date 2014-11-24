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
#include <dlfcn.h>
#include <iomanip>

#include "libzwergP.hh"
#include "builtin.hh"
#include "init.hh"
#include "builtin-dw.hh"
#include "docstring.hh"
#include "flag_saver.hh"

int
main (int argc, char const **argv)
{
  if (argc != 3)
    {
      std::cerr << "usage: " << argv[0] << " <vocabulary handle> <title>\n";
      return -1;
    }

  std::string handle = argv[1];
  std::string title = argv[2];

  void *sym = dlsym (RTLD_DEFAULT, handle.c_str ());
  if (sym == nullptr)
    {
      std::cerr << "Couldn't find a vocabulary with handle " << handle << "\n"
		<< dlerror () << std::endl;
      return -1;
    }

  {
    ios_flag_saver s {std::cout};
    std::cout << ".. _" << handle << ":\n\n"
	      << title << "\n"
	      << std::setw (title.length ()) << std::setfill ('=') << '='
	      << "\n\n";
  }

  std::vector <std::pair <std::string, std::string>> entries;

  auto bis = [&] ()
    {
      auto bldr = reinterpret_cast <decltype (&zw_vocabulary_core)> (sym);

      zw_error *err;
      zw_vocabulary const *voc = bldr (&err);
      if (voc == nullptr)
	  throw std::runtime_error (zw_error_message (err));

      return voc->m_voc->get_builtins ();
    } ();

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

  std::cout << format_entry_map (dentries, '-', handle);

}
