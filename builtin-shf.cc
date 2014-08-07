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

#include "builtin-shf.hh"
#include "op.hh"

valfile::uptr
op_drop::next ()
{
  if (auto vf = m_upstream->next ())
    {
      vf->pop ();
      return vf;
    }
  return nullptr;
}

valfile::uptr
op_swap::next ()
{
  if (auto vf = m_upstream->next ())
    {
      auto a = vf->pop ();
      auto b = vf->pop ();
      vf->push (std::move (a));
      vf->push (std::move (b));
      return vf;
    }
  return nullptr;
}

valfile::uptr
op_dup::next ()
{
  if (auto vf = m_upstream->next ())
    {
      vf->push (vf->top ().clone ());
      return vf;
    }
  return nullptr;
}

valfile::uptr
op_over::next ()
{
  if (auto vf = m_upstream->next ())
    {
      vf->push (vf->get (1).clone ());
      return vf;
    }
  return nullptr;
}

valfile::uptr
op_rot::next ()
{
  if (auto vf = m_upstream->next ())
    {
      auto a = vf->pop ();
      auto b = vf->pop ();
      auto c = vf->pop ();
      vf->push (std::move (b));
      vf->push (std::move (c));
      vf->push (std::move (a));
      return vf;
    }
  return nullptr;
}
