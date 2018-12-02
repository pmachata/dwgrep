/*
   Copyright (C) 2017, 2018 Petr Machata
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

namespace
{
  char const *const shf_docstring = R"docstring(

These words reorder elements on stack according to the following
schemes:

+------+---------+-----------+
| op   | before  | after     |
+======+=========+===========+
| dup  | A B C D | A B C D D |
+------+---------+-----------+
| over | A B C D | A B C D C |
+------+---------+-----------+
| swap | A B C D | A B D C   |
+------+---------+-----------+
| rot  | A B C D | A C D B   |
+------+---------+-----------+
| drop | A B C D | A B C     |
+------+---------+-----------+

Realistically, most of what end users should write will be an
occasional dup or drop.  Most of the stack reorganization effects can
be described more clearly using bindings and sub-expressions.  But the
operators are present for completeness' sake.

)docstring";
}

stack::uptr
op_drop::next (scon &sc) const
{
  if (auto stk = m_upstream->next (sc))
    {
      stk->pop ();
      return stk;
    }
  return nullptr;
}

std::string
op_drop::name () const
{
  return "drop";
}

std::string
op_drop::docstring ()
{
  return shf_docstring;
}


stack::uptr
op_swap::next (scon &sc) const
{
  if (auto stk = m_upstream->next (sc))
    {
      auto a = stk->pop ();
      auto b = stk->pop ();
      stk->push (std::move (a));
      stk->push (std::move (b));
      return stk;
    }
  return nullptr;
}

std::string
op_swap::name () const
{
  return "swap";
}

std::string
op_swap::docstring ()
{
  return shf_docstring;
}


stack::uptr
op_dup::next (scon &sc) const
{
  if (auto stk = m_upstream->next (sc))
    {
      stk->push (stk->top ().clone ());
      return stk;
    }
  return nullptr;
}

std::string
op_dup::name () const
{
  return "dup";
}

std::string
op_dup::docstring ()
{
  return shf_docstring;
}


stack::uptr
op_over::next (scon &sc) const
{
  if (auto stk = m_upstream->next (sc))
    {
      stk->push (stk->get (1).clone ());
      return stk;
    }
  return nullptr;
}

std::string
op_over::name () const
{
  return "over";
}

std::string
op_over::docstring ()
{
  return shf_docstring;
}


stack::uptr
op_rot::next (scon &sc) const
{
  if (auto stk = m_upstream->next (sc))
    {
      auto a = stk->pop ();
      auto b = stk->pop ();
      auto c = stk->pop ();
      stk->push (std::move (b));
      stk->push (std::move (a));
      stk->push (std::move (c));
      return stk;
    }
  return nullptr;
}

std::string
op_rot::name () const
{
  return "rot";
}

std::string
op_rot::docstring ()
{
  return shf_docstring;
}
