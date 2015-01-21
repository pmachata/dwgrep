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

#include <gtest/gtest.h>
#include "value-cst.hh"

struct dom
  : public constant_dom
{
  void show (mpz_class const &c, std::ostream &o,
	     brevity brv) const override
  {}

  char const *name () const override
  { return "dom1;"; }
} dom1, dom2;

TEST (ValueCstTest, comparison_of_different_domains)
{
  value_cst cst_a {constant {7, &dom1}, 0};
  value_cst cst_b {constant {7, &dom2}, 0};
  value_cst cst_c {constant {8, &dom1}, 0};
  value_cst cst_d {constant {8, &dom2}, 0};

  EXPECT_TRUE (cst_a.cmp (cst_b) != cmp_result::equal);
  EXPECT_TRUE (cst_a.cmp (cst_b) != cmp_result::fail);
  EXPECT_TRUE (cst_b.cmp (cst_a) != cmp_result::equal);
  EXPECT_TRUE (cst_b.cmp (cst_a) != cmp_result::fail);
  EXPECT_TRUE (cst_a.cmp (cst_b) != cst_b.cmp (cst_a));
  EXPECT_EQ (cst_a.cmp (cst_b), cst_c.cmp (cst_d));
  EXPECT_EQ (cst_b.cmp (cst_a), cst_d.cmp (cst_c));
}
