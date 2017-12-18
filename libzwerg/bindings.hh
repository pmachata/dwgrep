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

#ifndef _BINDINGS_H_
#define _BINDINGS_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

struct op_bind;
struct builtin;
struct vocabulary;

class binding
{
  op_bind *m_bind;
  builtin const *m_bi;

public:
  binding (op_bind &bind);
  binding (builtin const &bi);

  bool is_builtin () const
  { return m_bi != nullptr; }

  op_bind &get_bind () const;
  builtin const &get_builtin () const;
};

class bindings
{
  std::map <std::string, binding> m_bindings;
  bindings *m_super;

public:
  bindings ()
    : m_super {nullptr}
  {}

  explicit bindings (bindings &super)
    : m_super {&super}
  {}

  explicit bindings (vocabulary const &voc);

  void bind (std::string name, op_bind &op);
  binding const *find (std::string name);
  std::vector <std::string> names () const;
  std::vector <std::string> names_closure () const;
};

class upref
{
  bool m_id_used;
  unsigned m_id;
  builtin const *m_bi;

  explicit upref (builtin const *bi);

public:
  explicit upref (binding const &bn);
  static upref from (upref const &other);

  bool is_builtin () const
  { return m_bi != nullptr; }

  void mark_used (unsigned id);
  bool is_id_used () const;
  unsigned get_id () const;
  builtin const &get_builtin () const;
};

class uprefs
{
  std::map <std::string, upref> m_ids;
  unsigned m_nextid;

public:
  uprefs ();
  uprefs (bindings &bns, uprefs &super);

  upref *find (std::string name);
  std::map <unsigned, std::string> refd_ids () const;
};

#endif//_BINDINGS_H_
