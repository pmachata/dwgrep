/*
   Copyright (C) 2018 Petr Machata
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
#include "selector.hh"
#include "stack.hh"

selector::selector (stack const &s)
  : m_imprint {s.profile ()}
  , m_mask {0}
{}

std::vector <value_type>
selector::get_types () const
{
  std::vector <value_type> ret;

  const unsigned shift = 8 * (sizeof (selector::sel_t) - 1);
  for (auto imprint = m_imprint; imprint != 0; imprint <<= 8)
    if (uint8_t code = (imprint & (0xff << shift)) >> shift)
      ret.push_back (value_type {code});

  return ret;
}

std::ostream &
operator<< (std::ostream &o, selector const &sel)
{
  bool seen = false;
  auto types = sel.get_types ();
  if (types.empty ())
    o << "[empty]";
  else
    for (auto const &vt: types)
      {
	if (seen)
	  o << " ";
	seen = true;
	o << vt.name ();
      }

  return o;
}
