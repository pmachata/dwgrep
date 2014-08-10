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

std::array <value_type, 4>
selector::get_vts (stack const &s)
{
  auto ret = selector {}.m_vts;
  for (unsigned i = 0; i < ret.size () && i < s.size (); ++i)
    ret[ret.size () - i - 1] = s.get (i).get_type ();
  return ret;
}

selector::selector (stack const &s)
  : m_vts (get_vts (s))
  , m_mask {0} // No need to care about the mask, this is just for
	       // purposes of getting the imprint.
{}

std::ostream &
operator<< (std::ostream &o, selector const &sel)
{
  bool seen = false;
  for (unsigned i = 0; i < sel.m_vts.size (); ++i)
    if (sel.m_vts[i].code () != 0)
      {
	if (seen)
	  o << " ";
	seen = true;
	o << sel.m_vts[i].name ();
      }

  return o;
}
