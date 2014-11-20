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
#include <sstream>
#include <memory>

#include "builtin-cst.hh"
#include "op.hh"
#include "value-cst.hh"

namespace
{
  class op_cast
    : public op
  {
    std::shared_ptr <op> m_upstream;
    constant_dom const *m_dom;

  public:
    op_cast (std::shared_ptr <op> upstream, constant_dom const *dom)
      : m_upstream {upstream}
      , m_dom {dom}
    {}

    stack::uptr
    next () override
    {
      while (auto stk = m_upstream->next ())
	{
	  auto vp = stk->pop ();
	  if (auto v = value::as <value_cst> (&*vp))
	    {
	      constant cst2 {v->get_constant ().value (), m_dom};
	      stk->push (std::make_unique <value_cst> (cst2, 0));
	      return stk;
	    }
	  else
	    std::cerr << "Error: cast to " << m_dom->name ()
		      << " expects a constant on TOS.\n";
	}

      return nullptr;
    }

    std::string name () const override
    {
      std::stringstream ss;
      ss << "f_cast<" << m_dom->name () << ">";
      return ss.str ();
    }

    void reset () override
    {
      return m_upstream->reset ();
    }
  };
}


std::shared_ptr <op>
builtin_constant::build_exec (std::shared_ptr <op> upstream) const
{
  return std::make_shared <op_const> (upstream, m_value->clone ());
}

char const *
builtin_constant::name () const
{
  return "constant";
}

std::string
builtin_constant::docstring () const
{
  using namespace std::literals;

  auto val = value::as <value_cst> (&*m_value);
  assert (val != nullptr);
  auto dom = val->get_constant ().dom ();
  return "@"s + dom->name ();
}


std::shared_ptr <op>
builtin_hex::build_exec (std::shared_ptr <op> upstream) const
{
  return std::make_shared <op_cast> (upstream, &hex_constant_dom);
}

char const *
builtin_hex::name () const
{
  return "hex";
}


std::shared_ptr <op>
builtin_dec::build_exec (std::shared_ptr <op> upstream) const
{
  return std::make_shared <op_cast> (upstream, &dec_constant_dom);
}

char const *
builtin_dec::name () const
{
  return "dec";
}


std::shared_ptr <op>
builtin_oct::build_exec (std::shared_ptr <op> upstream) const
{
  return std::make_shared <op_cast> (upstream, &oct_constant_dom);
}

char const *
builtin_oct::name () const
{
  return "oct";
}


std::shared_ptr <op>
builtin_bin::build_exec (std::shared_ptr <op> upstream) const
{
  return std::make_shared <op_cast> (upstream, &bin_constant_dom);
}

char const *
builtin_bin::name () const
{
  return "bin";
}


stack::uptr
op_type::next ()
{
  if (auto stk = m_upstream->next ())
    {
      constant t = stk->pop ()->get_type_const ();
      stk->push (std::make_unique <value_cst> (t, 0));
      return stk;
    }

  return nullptr;
}

stack::uptr
op_pos::next ()
{
  if (auto stk = m_upstream->next ())
    {
      auto vp = stk->pop ();
      static numeric_constant_dom_t pos_dom_obj ("pos");
      stk->push (std::make_unique <value_cst>
		(constant {vp->get_pos (), &pos_dom_obj}, 0));
      return stk;
    }

  return nullptr;
}
