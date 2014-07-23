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

namespace
{
  struct op_drop
    : public inner_op
  {
    using inner_op::inner_op;

    valfile::uptr
    next () override
    {
      if (auto vf = m_upstream->next ())
	{
	  vf->pop ();
	  return vf;
	}
      return nullptr;
    }

    std::string
    name () const override
    {
      return "drop";
    }
  };

  struct op_swap
    : public inner_op
  {
    using inner_op::inner_op;

    valfile::uptr
    next () override
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

    std::string
    name () const override
    {
      return "swap";
    }
  };

  struct op_dup
    : public inner_op
  {
    using inner_op::inner_op;

    valfile::uptr
    next () override
    {
      if (auto vf = m_upstream->next ())
	{
	  vf->push (vf->top ().clone ());
	  return vf;
	}
      return nullptr;
    }

    std::string
    name () const override
    {
      return "dup";
    }
  };

  struct op_over
    : public inner_op
  {
    using inner_op::inner_op;

    valfile::uptr
    next () override
    {
      if (auto vf = m_upstream->next ())
	{
	  vf->push (vf->get (1).clone ());
	  return vf;
	}
      return nullptr;
    }

    std::string
    name () const override
    {
      return "over";
    }
  };

  struct op_rot
    : public inner_op
  {
    using inner_op::inner_op;

    valfile::uptr
    next () override
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

    std::string
    name () const override
    {
      return "rot";
    }
  };
}

std::shared_ptr <op>
builtin_drop::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			  std::shared_ptr <scope> scope) const
{
  return std::make_shared <op_drop> (upstream);
}

char const *
builtin_drop::name () const
{
  return "drop";
}

std::shared_ptr <op>
builtin_swap::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			  std::shared_ptr <scope> scope) const
{
  return std::make_shared <op_swap> (upstream);
}

char const *
builtin_swap::name () const
{
  return "swap";
}

std::shared_ptr <op>
builtin_dup::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			 std::shared_ptr <scope> scope) const
{
  return std::make_shared <op_dup> (upstream);
}

char const *
builtin_dup::name () const
{
  return "dup";
}

std::shared_ptr <op>
builtin_over::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			  std::shared_ptr <scope> scope) const
{
  return std::make_shared <op_over> (upstream);
}

char const *
builtin_over::name () const
{
  return "over";
}

std::shared_ptr <op>
builtin_rot::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			 std::shared_ptr <scope> scope) const
{
  return std::make_shared <op_rot> (upstream);
}

char const *
builtin_rot::name () const
{
  return "rot";
}
