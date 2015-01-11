/*
   Copyright (C) 2014, 2015 Red Hat, Inc.
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
  return std::make_shared <op_const> (upstream, m_value);
}

char const *
builtin_constant::name () const
{
  return "constant";
}

std::string
builtin_constant::docstring () const
{
  auto val = value::as <value_cst> (&*m_value);
  assert (val != nullptr);
  auto dom = val->get_constant ().dom ();
  return std::string ("@") + dom->name ();
}


namespace
{
  char const *const radices_docstring =
R"docstring(

``bin`` is used for converting constants to base-2, ``oct`` to base-8,
``dec`` to base-10 and ``hex`` to base-16.  These operators yield
incoming stack, except the domain of constant on TOS is changed.
Examples::

	$ dwgrep '64 hex'
	0x40

	$ dwgrep 'DW_AT_name hex'
	0x3

The value remains a constant, only the way it's displayed changes.
You can use ``"%s"`` to convert it to a string, in which case it's rendered
with the newly-selected domain::

	$ dwgrep 'DW_AT_name "=%s="'
	=DW_AT_name=
	$ dwgrep 'DW_AT_name hex "=%s="'
	=0x3=

Though you can achieve the same effect with formatting directives
``%b``, ``%o``, ``%d`` and ``%x``::

	$ dwgrep 'DW_AT_name "=%x="'
	=0x3=

)docstring";
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

std::string
builtin_hex::docstring () const
{
  return radices_docstring;
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

std::string
builtin_dec::docstring () const
{
  return radices_docstring;
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

std::string
builtin_oct::docstring () const
{
  return radices_docstring;
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

std::string
builtin_bin::docstring () const
{
  return radices_docstring;
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

std::string
op_type::docstring ()
{
  return
R"docstring(

This produces a constant according to the type of value on TOS (such
as T_CONST, T_DIE, T_STR, etc.).

)docstring";
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

std::string
op_pos::docstring ()
{
  return R"docstring(

Each function numbers elements that it produces, and stores number of
each element along with the element.  That number can be recalled by
saying ``pos``::

	$ dwgrep ./tests/dwz-partial -e 'unit (|A| A root "%s" A pos)'
	---
	0
	[34] compile_unit
	---
	1
	[a4] compile_unit
	---
	2
	[e1] compile_unit
	---
	3
	[11e] compile_unit

If you wish to know the number of values produced, you have to count
them by hand::

	[|Die| Die child]
	let Len := length;
	(|Lst| Lst elem)

At this point, ``pos`` and ``Len`` can be used to figure out both
absolute and relative position of a given element.

Note that *every* function counts its elements anew::

	$ dwgrep '"foo" elem pos'
	0
	1
	2

	$ dwgrep '"foo" elem type pos'
	0
	0
	0

)docstring";
}
