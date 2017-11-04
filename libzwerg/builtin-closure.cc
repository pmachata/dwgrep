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

struct op_apply::pimpl
{
  std::shared_ptr <op> m_upstream;
  std::unique_ptr <value_closure> m_value;

  pimpl (std::shared_ptr <op> upstream)
    : m_upstream {upstream}
  {}

  void
  reset_me ()
  {
    m_value = nullptr;
  }

  ~pimpl ()
  {
    reset_me ();
  }

  stack::uptr
  next ()
  {
    while (true)
      {
	while (m_value == nullptr)
	  if (auto stk = m_upstream->next ())
	    {
	      if (! stk->top ().is <value_closure> ())
		{
		  std::cerr << "Error: `apply' expects a T_CLOSURE on TOS.\n";
		  continue;
		}

	      m_value = std::unique_ptr <value_closure>
		(static_cast <value_closure *> (stk->pop ().release ()));
	      m_value->get_op ().reset ();
	      m_value->get_origin ().set_next (std::move (stk));
	    }
	  else
	    return nullptr;

	value_closure *orig = m_value->rdv_exchange (&*m_value);
	auto stk = m_value->get_op ().next ();
	m_value->rdv_exchange (orig);
	if (stk)
	  return stk;

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

std::string
builtin_apply::docstring () const
{
  return "@hide";
}
