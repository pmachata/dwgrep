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
#include "options.hh"
#include "libzwerg/strip.hh"

int
main(int argc, char *argv[])
{
  std::map <int, std::pair <std::vector <std::string>, std::string>> opts
    = merge_options (ext_options);

  for (auto const &opt: opts)
    {
      bool seen = false;
      for (auto const &l: opt.second.first)
	{
	  std::cout << (seen ? ", ``" : "``") << l << "``";
	  seen = true;
	}

      std::string ds = strip (opt.second.second, "\n");
      std::cout << "\n" << ds << "\n\n";
    }
}
