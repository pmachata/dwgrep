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

#include <iostream>
#include "std-memory.hh"

#include "builtin-closure.hh"
#include "value-closure.hh"

struct op_apply::pimpl
{
  std::shared_ptr <op> m_upstream;
  std::shared_ptr <op> m_op;
  std::shared_ptr <frame> m_old_frame;

  pimpl (std::shared_ptr <op> upstream)
    : m_upstream {upstream}
  {}

  void
  reset_me ()
  {
    m_op = nullptr;
    m_old_frame = nullptr;
  }

  stack::uptr
  next ()
  {
    while (true)
      {
	while (m_op == nullptr)
	  if (auto stk = m_upstream->next ())
	    {
	      if (! stk->top ().is <value_closure> ())
		{
		  std::cerr << "Error: `apply' expects a T_CLOSURE on TOS.\n";
		  continue;
		}

	      auto val = stk->pop ();
	      auto &cl = static_cast <value_closure &> (*val);

	      m_old_frame = stk->nth_frame (0);
	      stk->set_frame (cl.get_frame ());
	      auto origin = std::make_shared <op_origin> (std::move (stk));
	      m_op = cl.get_tree ().build_exec (origin);
	    }
	  else
	    return nullptr;

	if (auto stk = m_op->next ())
	  {
	    stk->set_frame (m_old_frame);
	    return stk;
	  }

	reset_me ();
      }
  }

  void
  reset ()
  {
    reset_me ();
    m_upstream->reset ();
  }
};

op_apply::op_apply (std::shared_ptr <op> upstream)
  : m_pimpl {std::make_unique <pimpl> (upstream)}
{}

op_apply::~op_apply ()
{}

void
op_apply::reset ()
{
  m_pimpl->reset ();
}

stack::uptr
op_apply::next ()
{
  return m_pimpl->next ();
}

std::string
op_apply::name () const
{
  return "apply";
}

std::shared_ptr <op>
builtin_apply::build_exec (std::shared_ptr <op> upstream) const
{
  return std::make_shared <op_apply> (upstream);
}

char const *
builtin_apply::name () const
{
  return "apply";
}
