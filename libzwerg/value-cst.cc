/*
   Copyright (C) 2017, 2018 Petr Machata
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
#include <memory>

#include <string.h>

#include "value-cst.hh"

value_type const value_cst::vtype = value_type::alloc ("T_CONST",
R"docstring(

Values of this type are used to hold integral constants.  Zwerg
constants allow storing signed or unsigned 64-bit numbers.  Arithmetic
operators handle transitions between signed and unsigned values
transparently::

	$ dwgrep '0xffffffffffffff00 0xffffffffffffffff sub'
	-0xff

Note that division and modulo by zero, as well as overflow, are caught
at runtime::

	$ dwgrep '5 (2, 0, 3) div'
	2
	Error: division by zero occured when computing 5/0
	1

	$ dwgrep '0xffffffffffffffff (-1, 0, 1) add'
	0xfffffffffffffffe
	0xffffffffffffffff
	Error: overflow occured when computing 18446744073709551615+1

)docstring");

void
value_cst::show (std::ostream &o) const
{
  o << m_cst;
}

std::unique_ptr <value>
value_cst::clone () const
{
  return std::make_unique <value_cst> (*this);
}

cmp_result
value_cst::cmp (value const &that) const
{
  if (auto v = value::as <value_cst> (&that))
    return compare (m_cst, v->m_cst);
  else
    return cmp_result::fail;
}


// value

value_cst
op_value_cst::operate (std::unique_ptr <value_cst> a) const
{
  return {constant {a->get_constant ().value (), &dec_constant_dom}, 0};
}

std::string
op_value_cst::docstring ()
{
  return
R"docstring(

Returns underlying value of the constant, with plain domain.  For
example::

	$ dwgrep 'DW_FORM_flag value'
	12

	$ dwgrep '0xc value'
	12

)docstring";
}


// bit

namespace
{
  struct bit_producer
    : public value_producer <value_cst>
  {
    size_t m_pos;
    uint64_t m_value;
    signedness m_sign;

    bit_producer (uint64_t value, signedness sign)
      : m_pos {0}
      , m_value {value}
      , m_sign {sign}
    {}

    std::unique_ptr <value_cst>
    next () override
    {
      if (unsigned bit = static_cast <unsigned> (ffsll (m_value)))
	{
	  auto ret = uint64_t (1) << (bit - 1);
	  m_value &= ~ret;
	  auto m = m_sign == signedness::unsign
	    ? mpz_class {ret, m_sign}
	    : mpz_class {-ret, m_sign};
	  constant cst {m, &hex_constant_dom};
	  return std::make_unique <value_cst> (cst, m_pos++);
	}
      return nullptr;
    }
  };
}

std::unique_ptr <value_producer <value_cst>>
op_bit_cst::operate (std::unique_ptr <value_cst> a) const
{
  auto const &cst = a->get_constant ();
  if (! cst.dom ()->safe_arith ())
    throw std::runtime_error
		("`bit' applied to a non-arithmetic constant value");

  if (cst.value () >= 0)
    return std::make_unique <bit_producer> (cst.value ().uval (),
					    signedness::unsign);
  else
    return std::make_unique <bit_producer> (std::abs (cst.value ().sval ()),
					    signedness::sign);
}

std::string
op_bit_cst::docstring ()
{
  return
R"docstring(

Returns bits of the value on TOS, which shall be a T_CONST with an arithmetic
domain (i.e. not a named constant). If the value is negative, the yielded bits
will be negative as well.

For example::

	$ dwgrep -e '0x37 bit'
	0x1
	0x2
	0x4
	0x10
	0x20

	$ dwgrep -e '-0x3 bit'
	-0x1
	-0x2

)docstring";
}


namespace
{
  template <class F>
  std::unique_ptr <value_cst>
  simple_arith_op (value_cst const &a, value_cst const &b, F f)
  {
    constant const &cst_a = a.get_constant ();
    constant const &cst_b = b.get_constant ();

    check_arith (cst_a, cst_b);

    constant_dom const *d = cst_a.dom ()->plain ()
      ? cst_b.dom () : cst_a.dom ();

    try
      {
	return f (cst_a, cst_b, d);
      }
    catch (std::domain_error &e)
      {
	std::cerr << "Error: " << e.what () << std::endl;
	return nullptr;
      }
  }

  template <class F>
  std::unique_ptr <value_cst>
  simple_arith_op (value_cst const &a, F f)
  {
    constant const &cst_a = a.get_constant ();
    check_arith (cst_a);

    try
      {
	return f (cst_a, cst_a.dom ());
      }
    catch (std::domain_error &e)
      {
	std::cerr << "Error: " << e.what () << std::endl;
	return nullptr;
      }
  }

  char const *const arith_docstring =
R"docstring(

Zwerg contains a suite of basic operators for integer arithmetic:
``add``, ``sub``, ``mul``, ``div`` and ``mod``.  Two elements are
popped: A and B, with B the original TOS, and ``A OP B`` is pushed
again.  *OP* is an operation suggested by the operator name.

Integers in Zwerg are 64-bit signed or unsigned quantities.  A value
of type ``T_CONST`` can hold either signed or unsigned number.  Which
it is is decided automatically.  Arithmetic operators handle these
cases transparently.

Overflows and division and modulo by zero produce an error message and
abort current computation, which is the reason this operation is
denoted with ``->?`` relation::

	$ dwgrep '5 (2, 0, 3) div'
	2
	Error: division by zero occured when computing 5/0
	1

)docstring";

  char const *const bitwise_docstring =
R"docstring(

Zwerg provides all basic bitwise operators: ``and``, ``or``, ``xor`` and
``neg``. For the first three, two elements are popped: A and B, with B the
original TOS, and ``A OP B`` is pushed again. *OP* is an operation suggested by
the operator name.

In case of ``neg``, one element is popped, and its bitwise negation (inversion)
is pushed again.

)docstring";

)docstring";

}

std::unique_ptr <value_cst>
op_add_cst::operate (std::unique_ptr <value_cst> a,
		     std::unique_ptr <value_cst> b) const
{
  return simple_arith_op
    (*a, *b,
     [] (constant const &cst_a, constant const &cst_b,
	 constant_dom const *d)
     {
       constant r {cst_a.value () + cst_b.value (), d};
       return std::make_unique <value_cst> (r, 0);
     });
}

std::string
op_add_cst::docstring ()
{
  return arith_docstring;
}


std::unique_ptr <value_cst>
op_sub_cst::operate (std::unique_ptr <value_cst> a,
		     std::unique_ptr <value_cst> b) const
{
  return simple_arith_op
    (*a, *b,
     [] (constant const &cst_a, constant const &cst_b,
	 constant_dom const *d)
     {
       constant r {cst_a.value () - cst_b.value (), d};
       return std::make_unique <value_cst> (r, 0);
     });
}

std::string
op_sub_cst::docstring ()
{
  return arith_docstring;
}


std::unique_ptr <value_cst>
op_mul_cst::operate (std::unique_ptr <value_cst> a,
		     std::unique_ptr <value_cst> b) const
{
  return simple_arith_op
    (*a, *b,
     [] (constant const &cst_a, constant const &cst_b,
	 constant_dom const *d) -> std::unique_ptr <value_cst>
     {
       constant r {cst_a.value () * cst_b.value (), d};
       return std::make_unique <value_cst> (r, 0);
     });
}

std::string
op_mul_cst::docstring ()
{
  return arith_docstring;
}


std::unique_ptr <value_cst>
op_div_cst::operate (std::unique_ptr <value_cst> a,
		     std::unique_ptr <value_cst> b) const
{
  return simple_arith_op
    (*a, *b,
     [] (constant const &cst_a, constant const &cst_b,
	 constant_dom const *d) -> std::unique_ptr <value_cst>
     {
       constant r {cst_a.value () / cst_b.value (), d};
       return std::make_unique <value_cst> (r, 0);
     });
}

std::string
op_div_cst::docstring ()
{
  return arith_docstring;
}


std::unique_ptr <value_cst>
op_mod_cst::operate (std::unique_ptr <value_cst> a,
		     std::unique_ptr <value_cst> b) const
{
  return simple_arith_op
    (*a, *b,
     [] (constant const &cst_a, constant const &cst_b,
	 constant_dom const *d) -> std::unique_ptr <value_cst>
     {
       constant r {cst_a.value () % cst_b.value (), d};
       return std::make_unique <value_cst> (r, 0);
     });
}

std::string
op_mod_cst::docstring ()
{
  return arith_docstring;
}


std::unique_ptr <value_cst>
op_and_cst::operate (std::unique_ptr <value_cst> a,
		     std::unique_ptr <value_cst> b) const
{
  return simple_arith_op
    (*a, *b,
     [] (constant const &cst_a, constant const &cst_b,
	 constant_dom const *d) -> std::unique_ptr <value_cst>
     {
       constant r {cst_a.value () & cst_b.value (), d};
       return std::make_unique <value_cst> (r, 0);
     });
}

std::string
op_and_cst::docstring ()
{
  return bitwise_docstring;
}


std::unique_ptr <value_cst>
op_or_cst::operate (std::unique_ptr <value_cst> a,
		    std::unique_ptr <value_cst> b) const
{
  return simple_arith_op
    (*a, *b,
     [] (constant const &cst_a, constant const &cst_b,
	 constant_dom const *d) -> std::unique_ptr <value_cst>
     {
       constant r {cst_a.value () | cst_b.value (), d};
       return std::make_unique <value_cst> (r, 0);
     });
}

std::string
op_or_cst::docstring ()
{
  return bitwise_docstring;
}


std::unique_ptr <value_cst>
op_xor_cst::operate (std::unique_ptr <value_cst> a,
		     std::unique_ptr <value_cst> b) const
{
  return simple_arith_op
    (*a, *b,
     [] (constant const &cst_a, constant const &cst_b,
	 constant_dom const *d) -> std::unique_ptr <value_cst>
     {
       constant r {cst_a.value () ^ cst_b.value (), d};
       return std::make_unique <value_cst> (r, 0);
     });
}

std::string
op_xor_cst::docstring ()
{
  return bitwise_docstring;
}


std::unique_ptr <value_cst>
op_neg_cst::operate (std::unique_ptr <value_cst> a) const
{
  return simple_arith_op
    (*a, [] (constant const &cst_a,
	     constant_dom const *d) -> std::unique_ptr <value_cst>
	 {
	   constant r {~cst_a.value (), d};
	   return std::make_unique <value_cst> (r, 0);
	 });
}

std::string
op_neg_cst::docstring ()
{
  return bitwise_docstring;
}
