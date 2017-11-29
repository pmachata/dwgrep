/*
   Copyright (C) 2017 Petr Machata
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

namespace
{
  struct substate
  {
    layout m_l;
    std::shared_ptr <op_origin> m_origin;
    std::shared_ptr <op> m_op;
    scon m_sc;
    scon_guard m_sg;
    std::shared_ptr <frame> m_old_frame;

    substate (value_closure &cl, stack::uptr stk)
      : m_l {}
      , m_origin {std::make_shared <op_origin> (m_l)}
      , m_op {cl.get_tree ().build_exec (m_l, m_origin)}
      , m_sc {m_l}
      , m_sg {m_sc, *m_op}
      , m_old_frame {stk->nth_frame (0)}
    {
      stk->set_frame (cl.get_frame ());
      m_origin->set_next (m_sc, std::move (stk));
    }

    stack::uptr
    next ()
    {
      if (auto stk = m_op->next (m_sc))
	{
	  // Restore the original stack frame.
	  std::shared_ptr <frame> of = stk->nth_frame (0);
	  stk->set_frame (m_old_frame);
	  value_closure::maybe_unlink_frame (of);
	  return stk;
	}
      return nullptr;
    }

    ~substate ()
    {
      value_closure::maybe_unlink_frame (m_old_frame);
    }
  };
}

struct op_apply::state
{
  std::unique_ptr <substate> m_ss;
};

op_apply::op_apply (layout &l, std::shared_ptr <op> upstream,
		    bool skip_non_closures)
  : m_upstream {upstream}
  , m_skip_non_closures {skip_non_closures}
  , m_ll {l.reserve <state> ()}
{}

void
op_apply::state_con (scon &sc) const
{
  sc.con <state> (m_ll);
  m_upstream->state_con (sc);
}

void
op_apply::state_des (scon &sc) const
{
  m_upstream->state_des (sc);
  sc.des <state> (m_ll);
}

stack::uptr
op_apply::next (scon &sc) const
{
  state &st = sc.get <state> (m_ll);
  while (true)
    {
      while (st.m_ss == nullptr)
	if (auto stk = m_upstream->next (sc))
	  {
	    if (! stk->top ().is <value_closure> ())
	      {
		if (m_skip_non_closures)
		  return stk;

		std::cerr << "Error: `apply' expects a T_CLOSURE on TOS.\n";
		continue;
	      }

	    auto val = stk->pop ();
	    auto &cl = static_cast <value_closure &> (*val);
	    st.m_ss = std::make_unique <substate> (cl, std::move (stk));
	  }
	else
	  return nullptr;

      if (auto stk = st.m_ss->next ())
	return stk;

      st.m_ss = nullptr;
    }
}

std::string
op_apply::name () const
{
  return "apply";
}

std::shared_ptr <op>
builtin_apply::build_exec (layout &l, std::shared_ptr <op> upstream) const
{
  return std::make_shared <op_apply> (l, upstream);
}

char const *
builtin_apply::name () const
{
  return "apply";
}

std::string
builtin_apply::docstring () const
{
  return "@hide";
}
