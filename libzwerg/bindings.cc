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
#include <cassert>
#include "builtin.hh"

binding::binding (op_bind &bind)
  : m_bind {&bind}
  , m_bi {nullptr}
{}

binding::binding (builtin const &bi)
  : m_bind {nullptr}
  , m_bi {&bi}
{}

op_bind &
binding::get_bind () const
{
  assert (! is_builtin ());
  return *m_bind;
}

builtin const &
binding::get_builtin () const
{
  assert (is_builtin ());
  return *m_bi;
}

bindings::bindings (vocabulary const &voc)
  : m_super {nullptr}
{
  for (auto const &entry: voc.get_builtins ())
    m_bindings.emplace (entry.first, binding {*entry.second});
}

void
bindings::bind (std::string name, op_bind &op)
{
  if (m_bindings.find (name) != m_bindings.end ())
    throw std::runtime_error
      (std::string () + "Name `" + name + "' rebound.");
  m_bindings.emplace (std::move (name), std::ref (op));
}

binding const *
bindings::find (std::string name)
{
  auto it = m_bindings.find (name);
  if (it != m_bindings.end ())
    return &it->second;
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


upref::upref (builtin const *bi)
  : m_id_used {false}
  , m_bi {bi}
{}

upref::upref (binding const &bn)
  : m_id_used {false}
  , m_bi {bn.is_builtin () ? &bn.get_builtin () : nullptr}
{}

upref
upref::from (upref const &other)
{
  // N.B. this initializes m_id_used and therefore is not simply a copy
  // constructor.
  return upref {other.is_builtin () ? &other.get_builtin () : nullptr};
}

void
upref::mark_used (unsigned id)
{
  assert (! is_builtin ());
  m_id_used = true;
  m_id = id;
}

bool
upref::is_id_used () const
{
  assert (! is_builtin ());
  return m_id_used;
}

unsigned
upref::get_id () const
{
  assert (! is_builtin ());
  return m_id;
}

builtin const &
upref::get_builtin () const
{
  assert (is_builtin ());
  return *m_bi;
}


uprefs::uprefs ()
  : m_nextid {0}
{}

uprefs::uprefs (bindings &bns, uprefs &super)
  : m_nextid {0}
{
  for (auto &nm: bns.names_closure ())
    {
      auto const &bn = *bns.find (nm);
      m_ids.emplace (std::move (nm), upref {bn});
    }
  for (auto &el: super.m_ids)
    m_ids.emplace (el.first, upref::from (el.second));
}

upref *
uprefs::find (std::string name)
{
  auto it = m_ids.find (name);
  if (it != m_ids.end ())
    {
      if (! it->second.is_builtin ()
	  && ! it->second.is_id_used ())
	it->second.mark_used (m_nextid++);
      return &it->second;
    }
  else
    return nullptr;
}

std::map <unsigned, std::string>
uprefs::refd_ids () const
{
  std::map <unsigned, std::string> ret;
  for (auto &el: m_ids)
    if (! el.second.is_builtin () && el.second.is_id_used ())
      ret[el.second.get_id ()] = el.first;
  return ret;
}
