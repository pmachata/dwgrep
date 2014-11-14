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

#ifndef _CACHE_H_
#define _CACHE_H_

#include <map>
#include <unordered_set>
#include <memory>
#include <vector>

#include <elfutils/libdw.h>

class parent_cache
{
  using unit_cache_t = std::vector <std::pair <Dwarf_Off, Dwarf_Off>>;
  using cache_t = std::map <std::pair <Dwarf *, Dwarf_Off>, unit_cache_t>;

  cache_t m_cache;

  void recursively_populate_unit (unit_cache_t &uc, Dwarf_Die die,
				  Dwarf_Off paroff);
  unit_cache_t populate_unit (Dwarf_Die die);

public:
  static Dwarf_Off const no_off = (Dwarf_Off) -1;
  Dwarf_Off find (Dwarf_Die die);
};

class root_cache
{
  using off_vect = std::vector <Dwarf_Off>;
  using cache_t = std::map <Dwarf *, off_vect>;

  cache_t m_cache;

public:
  bool is_root (Dwarf_Die die);
};


#endif /* _CACHE_H_ */
