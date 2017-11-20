/*
   Copyright (C) 2017 Petr Machata
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

#include "bindings.hh"

#include <set>
#include <stdexcept>

void
bindings::bind (std::string name, op_bind &op)
{
  if (m_bindings.find (name) != m_bindings.end ())
    throw std::runtime_error
      (std::string () + "Name `" + name + "' rebound.");
  m_bindings.emplace (std::move (name), std::ref (op));
}

op_bind *
bindings::find (std::string name)
{
  auto it = m_bindings.find (name);
  if (it != m_bindings.end ())
    return &it->second.get ();
  else if (m_super != nullptr)
    return m_super->find (name);
  else
    return nullptr;
}

std::vector <std::string>
bindings::names () const
{
  std::vector <std::string> names;
  for (auto const &elem: m_bindings)
    names.push_back (elem.first);
  return names;
}

std::vector <std::string>
bindings::names_closure () const
{
  std::set <std::string> names;

  for (bindings const *bnd = this; bnd != nullptr; bnd = bnd->m_super)
    for (std::string const &name: bnd->names ())
      names.insert (name);

  return {names.begin (), names.end ()};
}


uprefs::uprefs ()
  : m_nextid {0}
{}

uprefs::uprefs (bindings &bns, uprefs &super)
  : m_nextid {0}
{
  for (auto &nm: bns.names_closure ())
    m_ids.emplace (std::move (nm), std::make_pair (false, 0));
  for (auto &el: super.m_ids)
    m_ids.emplace (el.first, std::make_pair (false, 0));
}

std::pair <bool, unsigned>
uprefs::find (std::string name)
{
  auto it = m_ids.find (name);
  if (it != m_ids.end ())
    {
      if (! it->second.first)
	it->second = {true, m_nextid++};
      return it->second;
    }
  else
    return std::make_pair (false, -1u);
}

std::vector <std::string>
uprefs::refd_names () const
{
  std::vector <std::string> ret;
  for (auto &el: m_ids)
    if (el.second.first)
      ret.push_back (el.first);
  return ret;
}
