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

#include "scon.hh"
#include "op.hh"

scon_guard::scon_guard (scon2 &sc, op &op)
  : m_sc {sc}
  , m_op {op}
{
  m_op.state_con (m_sc);
}

scon_guard::~scon_guard ()
{
  // Des is just a destructor wrapper. If it excepts, it's as if a dtor
  // excepted, so let it terminate.
  m_op.state_des (m_sc);
}
