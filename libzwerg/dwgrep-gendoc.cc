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

#include "builtin.hh"
#include "init.hh"
#include "builtin-dw.hh"

int
main (int argc, char const **argv)
{
  auto voc = dwgrep_vocabulary_core ();
  for (auto const &bi: voc->get_builtins ())
    {
      std::cout << "``" << bi.first << "``\n===\n\n"
		<< bi.second->docstring ()
		<< std::endl << std::endl;
    }
}
