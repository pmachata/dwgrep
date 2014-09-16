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
#include "selector.hh"
#include "stack.hh"

selector::selector (stack const &s)
  : m_imprint {s.profile ()}
  , m_mask {0}
{}

std::ostream &
operator<< (std::ostream &o, selector const &sel)
{
  const unsigned shift = 8 * (sizeof (selector::sel_t) - 1);
  bool seen = false;
  for (auto imprint = sel.m_imprint; imprint != 0; imprint <<= 8)
    if (uint8_t code = (imprint & (0xff << shift)) >> shift)
      {
	if (seen)
	  o << " ";
	seen = true;
	o << value_type {code}.name ();
      }

  return o;
}
