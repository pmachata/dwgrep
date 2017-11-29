/*
   Copyright (C) 2017 Petr Machata
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

#include <gtest/gtest.h>
#include "builtin-cmp.hh"
#include "value-str.hh"
#include "value-cst.hh"

namespace
{
  template <class Builtin>
  pred_result
  result_of_builtin (stack &stk)
  {
    layout l;
    auto pred = Builtin {true}.build_pred (l);
    scon sc {l};
    return pred->result (sc, stk);
  }
}

TEST (TestBuiltinCmp, comparison_of_different_types)
{
  stack stk;
  stk.push (std::make_unique <value_cst> (constant {7, &dec_constant_dom}, 0));
  stk.push (std::make_unique <value_str> ("foo", 0));

  auto eqr = result_of_builtin <builtin_eq> (stk);
  auto ltr = result_of_builtin <builtin_lt> (stk);
  auto gtr = result_of_builtin <builtin_gt> (stk);

  ASSERT_NE (pred_result::fail, eqr);
  ASSERT_NE (pred_result::fail, ltr);
  ASSERT_NE (pred_result::fail, gtr);

  ASSERT_NE (pred_result::yes, eqr);
  ASSERT_TRUE (ltr == pred_result::yes || gtr == pred_result::yes);
  ASSERT_FALSE (ltr == pred_result::yes && gtr == pred_result::yes);
}
