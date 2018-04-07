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

#ifndef _VALUE_CST_H_
#define _VALUE_CST_H_

#include "value.hh"
#include "op.hh"
#include "overload.hh"

class value_cst
  : public value
{
  constant m_cst;

public:
  static value_type const vtype;

  value_cst (constant cst, size_t pos)
    : value {vtype, pos}
    , m_cst {cst}
  {}

  value_cst (value_cst const &that) = default;

  constant const &get_constant () const
  { return m_cst; }

  void show (std::ostream &o) const override;
  std::unique_ptr <value> clone () const override;
  cmp_result cmp (value const &that) const override;
};

struct op_value_cst
  : public op_once_overload <value_cst, value_cst>
{
  using op_once_overload::op_once_overload;

  value_cst operate (std::unique_ptr <value_cst> a) const override;

  static std::string docstring ();
};

struct op_bit_cst
  : public op_yielding_overload <value_cst, value_cst>
{
  using op_yielding_overload::op_yielding_overload;

  std::unique_ptr <value_producer <value_cst>>
  operate (std::unique_ptr <value_cst> a) const override;

  static std::string docstring ();
};


// Arithmetic operator overloads.

struct op_add_cst
  : public op_overload <value_cst, value_cst, value_cst>
{
  using op_overload::op_overload;

  std::unique_ptr <value_cst>
  operate (std::unique_ptr <value_cst> a,
	   std::unique_ptr <value_cst> b) const override;
  static std::string docstring ();
};

struct op_sub_cst
  : public op_overload <value_cst, value_cst, value_cst>
{
  using op_overload::op_overload;

  std::unique_ptr <value_cst>
  operate (std::unique_ptr <value_cst> a,
	   std::unique_ptr <value_cst> b) const override;
  static std::string docstring ();
};

struct op_mul_cst
  : public op_overload <value_cst, value_cst, value_cst>
{
  using op_overload::op_overload;

  std::unique_ptr <value_cst>
  operate (std::unique_ptr <value_cst> a,
	   std::unique_ptr <value_cst> b) const override;
  static std::string docstring ();
};

struct op_div_cst
  : public op_overload <value_cst, value_cst, value_cst>
{
  using op_overload::op_overload;

  std::unique_ptr <value_cst>
  operate (std::unique_ptr <value_cst> a,
	   std::unique_ptr <value_cst> b) const override;
  static std::string docstring ();
};

struct op_mod_cst
  : public op_overload <value_cst, value_cst, value_cst>
{
  using op_overload::op_overload;

  std::unique_ptr <value_cst>
  operate (std::unique_ptr <value_cst> a,
	   std::unique_ptr <value_cst> b) const override;
  static std::string docstring ();
};

struct op_and_cst
  : public op_overload <value_cst, value_cst, value_cst>
{
  using op_overload::op_overload;

  std::unique_ptr <value_cst>
  operate (std::unique_ptr <value_cst> a,
	   std::unique_ptr <value_cst> b) const override;
  static std::string docstring ();
};

struct op_or_cst
  : public op_overload <value_cst, value_cst, value_cst>
{
  using op_overload::op_overload;

  std::unique_ptr <value_cst>
  operate (std::unique_ptr <value_cst> a,
	   std::unique_ptr <value_cst> b) const override;
  static std::string docstring ();
};

struct op_xor_cst
  : public op_overload <value_cst, value_cst, value_cst>
{
  using op_overload::op_overload;

  std::unique_ptr <value_cst>
  operate (std::unique_ptr <value_cst> a,
	   std::unique_ptr <value_cst> b) const override;
  static std::string docstring ();
};

struct op_neg_cst
  : public op_overload <value_cst, value_cst>
{
  using op_overload::op_overload;

  std::unique_ptr <value_cst>
  operate (std::unique_ptr <value_cst> a) const override;
  static std::string docstring ();
};

#endif /* _VALUE_CST_H_ */
