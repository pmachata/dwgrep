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

#include "dwmods.hh"

#include <algorithm>
#include "dwit.hh"

std::vector <Dwarf *>
all_dwarfs (dwfl_context &dwctx)
{
  std::vector <Dwarf *> ret;
  std::for_each (dwfl_module_iterator {dwctx.get_dwfl ()},
		 dwfl_module_iterator::end (),
		 [&] (dwfl_module mod)
		 {
		   Dwarf *dw = mod.dwarf ();
		   ret.push_back (dw);
		   if (Dwarf *alt = dwarf_getalt (dw))
		     ret.push_back (alt);
		 });
  return ret;
}

bool
maybe_next_dwarf (cu_iterator &cuit,
		  std::vector <Dwarf *>::iterator &it,
		  std::vector <Dwarf *>::iterator const end)
{
  while (cuit == cu_iterator::end ())
    if (it == end)
      return false;
    else
      cuit = cu_iterator {*it++};
  return true;
}
