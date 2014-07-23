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

#include <memory>
#include "make_unique.hh"
#include <iostream>

#include "builtin-arith.hh"
#include "op.hh"

namespace
{
  struct arith_op
    : public inner_op
  {
    using inner_op::inner_op;

    valfile::uptr
    next () override
    {
      while (auto vf = m_upstream->next ())
	{
	  auto bp = vf->pop ();
	  auto ap = vf->pop ();
	  if (auto cp = operate (*ap, *bp))
	    {
	      vf->push (std::move (cp));
	      return vf;
	    }
	}

      return nullptr;
    }

    virtual std::unique_ptr <value>
    operate (value const &a, value const &b) const = 0;
  };

  template <class F>
  std::unique_ptr <value>
  simple_arith_op (value const &a, value const &b,
		   char const *name, F f)
  {
    if (auto va = value::as <value_cst> (&a))
      if (auto vb = value::as <value_cst> (&b))
	{
	  constant const &cst_a = va->get_constant ();
	  constant const &cst_b = vb->get_constant ();

	  check_arith (cst_a, cst_b);

	  constant_dom const *d = cst_a.dom ()->plain ()
	    ? cst_b.dom () : cst_a.dom ();

	  return f (cst_a, cst_b, d);
	}

    std::cerr << "Error: `" << name << "' expects T_CONST at and below TOS.\n";
    return nullptr;
  }
}


struct builtin_sub::o
  : public arith_op
{
  using arith_op::arith_op;

  std::unique_ptr <value>
  operate (value const &a, value const &b) const override
  {
    return simple_arith_op
      (a, b, "sub",
       [] (constant const &cst_a, constant const &cst_b,
	   constant_dom const *d)
       {
	 constant r {cst_a.value () - cst_b.value (), d};
	 return std::make_unique <value_cst> (r, 0);
       });
  }

  std::string name () const override
  {
    return "sub";
  }
};

std::shared_ptr <op>
builtin_sub::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			 std::shared_ptr <scope> scope) const
{
  return std::make_shared <o> (upstream);
}

char const *
builtin_sub::name () const
{
  return "sub";
}


struct builtin_mul::o
  : public arith_op
{
  using arith_op::arith_op;

  std::unique_ptr <value>
  operate (value const &a, value const &b) const override
  {
    return simple_arith_op
      (a, b, "mul",
       [] (constant const &cst_a, constant const &cst_b,
	   constant_dom const *d)
       {
	 constant r {cst_a.value () * cst_b.value (), d};
	 return std::make_unique <value_cst> (r, 0);
       });
  }

  std::string name () const override
  {
    return "mul";
  }
};

std::shared_ptr <op>
builtin_mul::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			 std::shared_ptr <scope> scope) const
{
  return std::make_shared <o> (upstream);
}

char const *
builtin_mul::name () const
{
  return "mul";
}


struct builtin_div::o
  : public arith_op
{
  using arith_op::arith_op;

  std::unique_ptr <value>
  operate (value const &a, value const &b) const override
  {
    return simple_arith_op
      (a, b, "div",
       [] (constant const &cst_a, constant const &cst_b,
	   constant_dom const *d) -> std::unique_ptr <value>
       {
	 if (cst_b.value () == 0)
	   {
	     std::cerr << "Error: `div': division by zero.\n";
	     return nullptr;
	   }

	 constant r {cst_a.value () / cst_b.value (), d};
	 return std::make_unique <value_cst> (r, 0);
       });
  }

  std::string name () const override
  {
    return "div";
  }
};

std::shared_ptr <op>
builtin_div::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			 std::shared_ptr <scope> scope) const
{
  return std::make_shared <o> (upstream);
}

char const *
builtin_div::name () const
{
  return "div";
}


struct builtin_mod::o
  : public arith_op
{
  using arith_op::arith_op;

  std::unique_ptr <value>
  operate (value const &a, value const &b) const override
  {
    return simple_arith_op
      (a, b, "mod",
       [] (constant const &cst_a, constant const &cst_b,
	   constant_dom const *d) -> std::unique_ptr <value>
       {
	 if (cst_b.value () == 0)
	   {
	     std::cerr << "Error: `mod': division by zero.\n";
	     return nullptr;
	   }

	 constant r {cst_a.value () % cst_b.value (), d};
	 return std::make_unique <value_cst> (r, 0);
       });
  }

  std::string name () const override
  {
    return "mod";
  }
};

std::shared_ptr <op>
builtin_mod::build_exec (std::shared_ptr <op> upstream, dwgrep_graph::sptr q,
			 std::shared_ptr <scope> scope) const
{
  return std::make_shared <o> (upstream);
}

char const *
builtin_mod::name () const
{
  return "mod";
}
