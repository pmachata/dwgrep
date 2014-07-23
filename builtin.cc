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

#include <memory>
#include "make_unique.hh"
#include "builtin.hh"
#include "op.hh"

std::unique_ptr <pred>
builtin::build_pred (dwgrep_graph::sptr q, std::shared_ptr <scope> scope) const
{
  return nullptr;
}

std::shared_ptr <op>
builtin::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
		     std::shared_ptr <scope> scope) const
{
  return nullptr;
}

std::unique_ptr <pred>
pred_builtin::maybe_invert (std::unique_ptr <pred> pred) const
{
  if (m_positive)
    return pred;
  else
    return std::make_unique <pred_not> (std::move (pred));
}

namespace
{
  std::map <std::string, builtin const &> &
  get_builtin_map ()
  {
    static std::map <std::string, builtin const &> bi_map;
    return bi_map;
  }
}

void
add_builtin (builtin &b, std::string const &name)
{
  get_builtin_map ().insert
    (std::pair <std::string, builtin const &> (name, b));
}

void
add_builtin (builtin &b)
{
  add_builtin (b, b.name ());
}

builtin const *
find_builtin (std::string const &name)
{
  auto it = get_builtin_map ().find (name);
  if (it != get_builtin_map ().end ())
    return &it->second;
  else
    return nullptr;
}
