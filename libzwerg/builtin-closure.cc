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

#include "builtin-closure.hh"
#include "value-closure.hh"
#include "scon.hh"

// State for the sub-program that the apply executes. This gets constructed each
// time a new program is pulled from upstream, then used to fetch stacks to
// yield, and then torn down.
struct op_apply::substate
{
  std::unique_ptr <value_closure> m_value;
  scon m_scon;
  scon_guard m_sg;

  substate (stack::uptr stk)
    : m_value {static_cast <value_closure *> (stk->pop ().release ())}
    , m_scon {m_value->get_layout ()}
    , m_sg {m_scon, m_value->get_op ()}
  {
    m_scon.con <op_apply::rendezvous> (m_value->get_rdv_ll (),
				       std::ref (*m_value));
    m_value->get_origin ().set_next (m_scon, std::move (stk));
  }

  stack::uptr
  next ()
  {
    return m_value->get_op ().next (m_scon);
  }
};

// State for this operation.
struct op_apply::state
{
  std::unique_ptr <substate> m_substate;
};

op_apply::op_apply (layout &l, std::shared_ptr <op> upstream,
		    bool skip_non_closures)
  : m_upstream {upstream}
  , m_skip_non_closures {skip_non_closures}
  , m_ll {l.reserve <state> ()}
{}

std::string
op_apply::name () const
{
  return "apply";
}

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
      while (st.m_substate == nullptr)
	if (auto stk = m_upstream->next (sc))
	  {
	    if (! stk->top ().is <value_closure> ())
	      {
		if (m_skip_non_closures)
		  return stk;

		std::cerr << "Error: `apply' expects a T_CLOSURE on TOS.\n";
		continue;
	      }

	    st.m_substate = std::make_unique <substate> (std::move (stk));
	  }
	else
	  return nullptr;

      if (auto stk = st.m_substate->next ())
	return stk;

      st.m_substate = nullptr;
    }
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
