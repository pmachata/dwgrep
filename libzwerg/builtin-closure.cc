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
#include "scon.hh"

// State for the sub-program that the apply executes. This gets constructed each
// time a new program is pulled from upstream, then used to fetch stacks to
// yield, and then torn down.
struct op_apply::substate
{
  std::unique_ptr <value_closure> m_value;
  scon m_scon;

  substate (stack::uptr stk)
    : m_value {static_cast <value_closure *> (stk->pop ().release ())}
    , m_scon {m_value->get_origin (), m_value->get_op (), std::move (stk)}
  {}

  stack::uptr
  next ()
  {
    // xxx I think that now that we have a full-featured stacking, the
    // rendezvous logic can be rethought.
    value_closure *orig = m_value->rdv_exchange (&*m_value);
    auto stk = m_scon.next ();
    m_value->rdv_exchange (orig);
    return stk;
  }
};

// State for this operation.
struct op_apply::state
{
  std::unique_ptr <substate> m_substate;
};

std::string
op_apply::name () const
{
  return "apply";
}

size_t
op_apply::reserve () const
{
  return m_upstream->reserve () + sizeof (state);
}

void
op_apply::state_con (void *buf) const
{
  state *st = op::this_state <state> (buf);
  new (st) state {};
  m_upstream->state_con (op::next_state (st));
}

void
op_apply::state_des (void *buf) const
{
  state *st = op::this_state <state> (buf);
  m_upstream->state_des (op::next_state (st));
  st->~state ();
}

stack::uptr
op_apply::next (void *buf) const
{
  state *st = op::this_state <state> (buf);
  while (true)
    {
      while (st->m_substate == nullptr)
	if (auto stk = m_upstream->next (st + 1))
	  {
	    if (! stk->top ().is <value_closure> ())
	      {
		std::cerr << "Error: `apply' expects a T_CLOSURE on TOS.\n";
		continue;
	      }

	    st->m_substate = std::make_unique <substate> (std::move (stk));
	  }
	else
	  return nullptr;

      if (auto stk = st->m_substate->next ())
	return stk;

      st->m_substate = nullptr;
    }
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

std::string
builtin_apply::docstring () const
{
  return "@hide";
}
