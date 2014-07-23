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

#include <libdw.h>

enum class unit_type
  {
    INFO,
  };

class parent_cache
{
  struct unit_key
  {
    unit_type ut;
    Dwarf_Off off;	// CU offset.

    bool
    operator< (unit_key other) const
    {
      return ut < other.ut || (ut == other.ut && off < other.off);
    }
  };

  typedef std::vector <std::pair <Dwarf_Off, Dwarf_Off> > unit_cache_t;
  typedef std::map <unit_key, unit_cache_t> cache_t;

  cache_t m_cache;

  void recursively_populate_unit (unit_cache_t &uc, Dwarf_Die die,
				  Dwarf_Off paroff);
  unit_cache_t populate_unit (Dwarf_Die die);

public:
  Dwarf_Off find (Dwarf_Die die);
};

class root_cache
{
  typedef std::pair <unit_type, Dwarf_Off> typed_off_t;

  struct hash
  {
    size_t
    operator() (typed_off_t p) const
    {
      return std::hash <int> () ((int) p.first)
	^ std::hash <Dwarf_Off> () (p.second);
    }
  };

  typedef std::unordered_set <typed_off_t, hash> root_map_t;
  std::unique_ptr <root_map_t> m_roots;

public:
  bool is_root (Dwarf_Die die, Dwarf *dw);
};


#endif /* _CACHE_H_ */
