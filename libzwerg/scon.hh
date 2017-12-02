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

#ifndef _SCON_H_
#define _SCON_H_

#include <vector>
#include <cstdint>
#include <memory>

#include "layout.hh"

class scon
{
  std::vector <uint8_t> m_buf;

  void *
  mem (layout::loc loc)
  {
    return m_buf.data () + loc.m_loc;
  }

public:
  scon (layout const &l);

  template <class State>
  State &
  get (layout::loc loc)
  {
    return *reinterpret_cast <State *> (this->mem (loc));
  }

  template <class State, class... Args>
  void
  con (layout::loc loc, Args const&... args)
  {
    new (this->mem (loc)) State {args...};
  }

  template <class State>
  void
  des (layout::loc loc)
  {
    this->get <State> (loc).~State ();
  }

  template <class State, class... Args>
  void
  reset (layout::loc loc, Args const&... args)
  {
    this->des <State> (loc);
    this->con <State> (loc, args...);
  }
};

class op;
class stack;
struct scon_guard
{
  scon &m_sc;
  op *m_op;

  scon_guard (scon &sc, op &op);
  scon_guard (scon_guard const &cp) = delete;
  scon_guard (scon_guard &&mv);
  ~scon_guard ();

  std::unique_ptr <stack> next () const;
};

#endif // _SCON_H_
