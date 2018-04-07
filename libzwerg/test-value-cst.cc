/*
   Copyright (C) 2018 Petr Machata
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

#include "test-zw-aux.hh"
#include "value-cst.hh"

using namespace test;

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

TEST_F (CoreTest, test_value_cst)
{
  typedef std::vector <std::tuple <size_t, std::string>> vec;
  for (auto const &entry: vec {
	    {1, "[3 bit] == [0x1, 0x2]"},
	    {1, "[-3 bit] == [-0x1, -0x2]"},
	    {1, "[0x1234 bit] == [0x4, 0x10, 0x20, 0x200, 0x1000]"},
	    {1, "[4660 bit] == [0x4, 0x10, 0x20, 0x200, 0x1000]"},
	})
    {
      size_t nresults = std::get <0> (entry);
      auto const &q = std::get <1> (entry);
      auto stk = std::make_unique <stack> ();

      auto yielded = run_query (*builtins, std::move (stk), q);
      ASSERT_EQ (nresults, yielded.size ());
    }
}

TEST_F (CoreTest, test_value_cst_bit_named)
{
  ASSERT_EQ (1, run_query (*builtins, std::make_unique <stack> (),
			   "[1 type value bit]").size ());
  ASSERT_THROW (run_query (*builtins, std::make_unique <stack> (),
			   "[1 type bit]"),
		std::runtime_error);
}
