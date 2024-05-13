/*
   Copyright (C) 2017 Petr Machata
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
#include <algorithm>

#include "value-closure.hh"
#include "tree.hh"

value_type const value_closure::vtype
	= value_type::alloc ("T_CLOSURE", "@hide");

value_closure::value_closure (layout op_layout, layout::loc rdv_ll,
			      std::shared_ptr <op_origin> origin,
			      std::shared_ptr <op> op,
			      std::vector <std::unique_ptr <value>> env,
			      size_t pos)
  : value {vtype, pos}
  , m_op_layout {op_layout}
  , m_rdv_ll {rdv_ll}
  , m_origin {origin}
  , m_op {op}
  , m_env {std::move (env)}
{}

void
value_closure::show (std::ostream &o) const
{
  o << "closure";
}

std::unique_ptr <value>
value_closure::clone () const
{
  std::vector <std::unique_ptr <value>> env;
  for (auto const &envv: m_env)
    env.push_back (envv->clone ());
  return std::make_unique <value_closure> (m_op_layout, m_rdv_ll,
					   m_origin, m_op,
					   std::move (env), get_pos ());
}

cmp_result
value_closure::cmp (value const &v) const
{
  if (auto that = value::as <value_closure> (&v))
    {
      auto a = std::make_tuple (m_op, std::cref (m_env));
      auto b = std::make_tuple (that->m_op, std::cref (that->m_env));
      return compare (a, b);
    }
  else
    return cmp_result::fail;
}
