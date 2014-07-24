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
#include <map>

#include "builtin.hh"
#include "builtin-cst.hh"
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

struct builtin_dict::builtins
  : public std::map <std::string, std::shared_ptr <builtin const>>
{};

builtin_dict::builtin_dict ()
  : m_builtins {std::make_unique <builtins> ()}
{}

builtin_dict::builtin_dict (builtin_dict &a, builtin_dict &b)
  : builtin_dict {}
{
  for (auto const &builtin: *a.m_builtins)
    add (builtin.second, builtin.first);
  for (auto const &builtin: *b.m_builtins)
    add (builtin.second, builtin.first);
}

builtin_dict::~builtin_dict ()
{}

void
builtin_dict::add (std::shared_ptr <builtin const> b)
{
  add (b, b->name ());
}

void
builtin_dict::add (std::shared_ptr <builtin const> b, std::string const &name)
{
  assert (find (name) == nullptr);
  m_builtins->insert (std::make_pair (name, b));
}

std::shared_ptr <builtin const>
builtin_dict::find (std::string const &name) const
{
  auto it = m_builtins->find (name);
  if (it == m_builtins->end ())
    return nullptr;
  else
    return it->second;
}

void
add_builtin_constant (builtin_dict &dict, constant cst, char const *name)
{
  auto builtin = std::make_shared <builtin_constant>
    (std::make_unique <value_cst> (cst, 0));
  dict.add (builtin, name);
}
