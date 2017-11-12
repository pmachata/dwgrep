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

#ifndef _RENDEZVOUS_HH_
#define _RENDEZVOUS_HH_

#include <memory>

class value_closure;

// A rendezvous cell keeps track of the particular closure value that is
// currently under evaluation.
class rendezvous
{
  std::shared_ptr <value_closure *> m_vp;

public:
  rendezvous ()
    : m_vp {std::make_shared <value_closure *> (nullptr)}
  {}

  value_closure *exchange (value_closure *vp)
  {
    value_closure *old = *m_vp;
    *m_vp = vp;
    return old;
  }

  value_closure *operator* () const
  {
    return m_vp.operator* ();
  }

  value_closure **operator-> () const
  {
    return m_vp.operator-> ();
  }
};

#endif /* _RENDEZVOUS_HH_ */
