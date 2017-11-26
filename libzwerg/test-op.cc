/*
   Copyright (C) 2015 Red Hat, Inc.
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
#include "std-memory.hh"

#include "op.hh"
#include "init.hh"
#include "value-cst.hh"
#include "test-zw-aux.hh"

struct ZwTest
  : public testing::Test
{
  std::unique_ptr <vocabulary> builtins;

  void
  SetUp () override final
  {
    builtins = dwgrep_vocabulary_core ();
  }
};

namespace
{
  void
  test_closure (op_tr_closure_kind k, size_t offset)
  {
    layout l;
    auto v = std::make_unique <value_cst> (constant {0, &dec_constant_dom}, 0);
    auto inner_origin = std::make_shared <op_origin> (l);
    auto inner = std::make_shared <op_const> (inner_origin, std::move (v));

    auto outer_origin = std::make_shared <op_origin> (l);
    auto outer = std::make_shared <op_tr_closure> (l, outer_origin,
						   inner_origin, inner, k);

    scon sc {l};
    scon_guard sg {sc, *outer};
    outer_origin->set_next (sc, std::make_unique <stack> ());

    for (size_t i = 0; i < 20; ++i)
      {
	auto stk = outer->next (sc);
	ASSERT_TRUE (stk != nullptr);
	EXPECT_EQ (i + offset, stk->size ());
      }
  }
}

TEST_F (ZwTest, test_closure_star)
{
  test_closure (op_tr_closure_kind::star, 0);
}

TEST_F (ZwTest, test_closure_plus)
{
  test_closure (op_tr_closure_kind::plus, 1);
}

namespace
{
  void
  test_closure_closure (op_tr_closure_kind k)
  {
    layout l;
    auto v = std::make_unique <value_cst> (constant {0, &dec_constant_dom}, 0);
    auto inner_origin = std::make_shared <op_origin> (l);
    auto inner = std::make_shared <op_const> (inner_origin, std::move (v));

    auto mid_origin = std::make_shared <op_origin> (l);
    auto mid = std::make_shared <op_tr_closure> (l, mid_origin,
						 inner_origin, inner, k);

    auto outer_origin = std::make_shared <op_origin> (l);
    auto outer = std::make_shared <op_tr_closure> (l, outer_origin,
						   mid_origin, mid, k);
    scon sc {l};
    scon_guard sg {sc, *outer};
    outer_origin->set_next (sc, std::make_unique <stack> ());

    for (size_t i = 0; i < 20; ++i)
      ASSERT_TRUE (outer->next (sc) != nullptr);
  }
}

TEST_F (ZwTest, test_closure_star_star)
{
  test_closure_closure (op_tr_closure_kind::star);
}

TEST_F (ZwTest, test_closure_plus_plus)
{
  test_closure_closure (op_tr_closure_kind::plus);
}
