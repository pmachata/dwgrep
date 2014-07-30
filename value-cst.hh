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

#ifndef _VALUE_CST_H_
#define _VALUE_CST_H_

#include "value.hh"
#include "op.hh"

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

  void show (std::ostream &o, brevity brv) const override;
  std::unique_ptr <value> clone () const override;
  cmp_result cmp (value const &that) const override;
};

struct op_value_cst
  : public stub_op
{
  using stub_op::stub_op;

  static value_type get_value_type ()
  { return value_cst::vtype; }

  valfile::uptr next () override;
};


// Arithmetic operator overloads.

struct cst_arith_op
  : public stub_op
{
  using stub_op::stub_op;

  valfile::uptr next () override;

  virtual std::unique_ptr <value>
  operate (value_cst const &a, value_cst const &b) const = 0;
};

struct op_add_cst
  : public cst_arith_op
{
  using cst_arith_op::cst_arith_op;

  static value_type get_value_type ()
  { return value_cst::vtype; }

  std::unique_ptr <value>
  operate (value_cst const &a, value_cst const &b) const override;
};

struct op_sub_cst
  : public cst_arith_op
{
  using cst_arith_op::cst_arith_op;

  static value_type get_value_type ()
  { return value_cst::vtype; }

  std::unique_ptr <value>
  operate (value_cst const &a, value_cst const &b) const override;
};

struct op_mul_cst
  : public cst_arith_op
{
  using cst_arith_op::cst_arith_op;

  static value_type get_value_type ()
  { return value_cst::vtype; }

  std::unique_ptr <value>
  operate (value_cst const &a, value_cst const &b) const override;
};

struct op_div_cst
  : public cst_arith_op
{
  using cst_arith_op::cst_arith_op;

  static value_type get_value_type ()
  { return value_cst::vtype; }

  std::unique_ptr <value>
  operate (value_cst const &a, value_cst const &b) const override;
};

struct op_mod_cst
  : public cst_arith_op
{
  using cst_arith_op::cst_arith_op;

  static value_type get_value_type ()
  { return value_cst::vtype; }

  std::unique_ptr <value>
  operate (value_cst const &a, value_cst const &b) const override;
};

#endif /* _VALUE_CST_H_ */
