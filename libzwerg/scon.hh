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

#include "op.hh"

// State Contatiner keeps an activation record for one recursive instance of op
// chain.
class scon
{
  op const &m_op;
  std::vector <uint8_t> m_buf;

  void *buf ();
  void *buf_end ();

public:
  scon (op_origin const &origin, op const &op, stack::uptr stk);
  ~scon ();
  stack::uptr next ();
};

#endif // _SCON_H_
