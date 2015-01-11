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

#include <iostream>
#include "std-memory.hh"

#include "value-closure.hh"
#include "tree.hh"

value_type const value_closure::vtype
	= value_type::alloc ("T_CLOSURE", "@hide");

value_closure::value_closure (tree const &t,
			      std::shared_ptr <frame> frame, size_t pos)
  : value {vtype, pos}
  , m_t {std::make_unique <tree> (t)}
  , m_frame {frame}
{}

value_closure::value_closure (value_closure const &that)
  : value_closure {*that.m_t, that.m_frame, that.get_pos ()}
{}

value_closure::~value_closure()
{}

void
value_closure::show (std::ostream &o, brevity brv) const
{
  o << "closure(" << *m_t << ")";
}

cmp_result
value_closure::cmp (value const &v) const
{
  if (auto that = value::as <value_closure> (&v))
    {
      auto a = std::make_tuple (static_cast <tree &> (*m_t), m_frame);
      auto b = std::make_tuple (static_cast <tree &> (*that->m_t),
				that->m_frame);
      return compare (a, b);
    }
  else
    return cmp_result::fail;
}
