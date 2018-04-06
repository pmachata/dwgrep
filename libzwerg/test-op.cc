/*
   Copyright (C) 2017, 2018 Petr Machata
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

#include "builtin-core.hh"
#include "op.hh"
#include "value-cst.hh"
#include "test-zw-aux.hh"

using namespace test;

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

namespace
{
  struct empty {};
  struct value_canary
    : public value
  {
    static value_type const vtype;
    std::shared_ptr <empty> m_counter;

    value_canary (std::shared_ptr <empty> counter)
      : value {vtype, 0}
      , m_counter {counter}
    {}

    void
    show (std::ostream &o) const override
    {
      o << "canary";
    }

    std::unique_ptr <value>
    clone () const override
    {
      return std::make_unique <value_canary> (m_counter);
    }

    cmp_result
    cmp (value const &that) const override
    {
      if (that.is <value_canary> ())
	return cmp_result::equal;
      else
	return cmp_result::fail;
    }
  };

  value_type const value_canary::vtype = value_type::alloc ("canary", "");
}

TEST_F (ZwTest, frame_with_value_referencing_it_doesnt_leak)
{
  auto counter = std::make_shared <empty> ();
  auto stk = stack_with_value (std::make_unique <value_canary> (counter));
  // Bind F to a canary and G to a closure.  The closure will keep the
  // whole frame alive, including the canary.  Thus we can observe the
  // absence of leak by observing whether the canary's destructor ran.
  run_query (*builtins, std::move (stk), "{} (|F G|)");
  ASSERT_EQ (1, counter.use_count ());
}

TEST_F (ZwTest, frame_with_value_referencing_it_doesnt_leak_2)
{
  auto counter = std::make_shared <empty> ();
  auto stk = stack_with_value (std::make_unique <value_canary> (counter));
  run_query (*builtins, std::move (stk), "{} (|F G| G)");
  ASSERT_EQ (1, counter.use_count ());
}

TEST_F (ZwTest, frame_with_value_referencing_it_doesnt_leak_3)
{
  auto counter = std::make_shared <empty> ();
  auto stk = stack_with_value (std::make_unique <value_canary> (counter));
  run_query (*builtins, std::move (stk), "{{} apply} (|F G| ?(G))");
  ASSERT_EQ (1, counter.use_count ());
}

TEST_F (ZwTest, iterate_lexical_closure_1)
{
  auto stk = std::make_unique <stack> ();
  auto yielded = run_query (*builtins, std::move (stk), "1 (drop {})*");
  ASSERT_EQ (2, yielded.size ());
}

TEST_F (ZwTest, iterate_lexical_closure_2)
{
  auto stk = std::make_unique <stack> ();
  auto yielded = run_query (*builtins, std::move (stk), "{} (drop {})*");
  ASSERT_EQ (2, yielded.size ());
}

TEST_F (ZwTest, test_1)
{
  // This caused SIGSEGV due to wrong upvalue tracking.
  auto stk = std::make_unique <stack> ();
  auto yielded = run_query (*builtins, std::move (stk),
			    "let X := 1; {{} {}} apply");
  ASSERT_EQ (1, yielded.size ());
}

TEST_F (ZwTest, test_compilation_errors)
{
  for (auto const &entry: std::map <std::string, std::string> {
	    {"let A := ; let A := ;",
		"Name `A' rebound."},
	    {"(|A| let A := ;)",
		"Name `A' rebound."},
	    {"let A := let A := A 1 add; A 1 add;",
		"Attempt to read an unbound name `A'"},
	    {"let A := {A};",
		"Attempt to read an unbound name `A'"},
	    {"(|A| (|A|))",
		""},
	    {"let A := let A := ; ;",
		""},
	})
    {
      auto err = get_parse_error (*builtins, entry.first);
      ASSERT_EQ (err, entry.second);
    }
}

TEST_F (ZwTest, test_let)
{
  for (auto const &entry: std::map <size_t, std::string> {
	    {1, "1 (|A| 2 (|B| let A := A 1 add; A) A) "
		"[|X Y| X, Y] == [2, 1]"},
	    {1, "1 (|A| let B := let A := A 1 add; A; B A) "
		"[|X Y| X, Y] == [2, 1]"},
	    {1, "let length := {1}; ([] length == 1)"},
	    {1, "let length := {length 2 add}; "
		"([] length == 2) "
		"([1, 2, 3] length == 5)"},
	    {1, "let length := {length 2 add}; "
		"1 (|A| let length := {length 3 add}; "
		"       ([] length == 5)"
		"       ([1, 2, 3] length == 7))"},
	    {1, "{length 1 add} (|length| {length 2 add}) let F := ; "
		"?([] (F == 3) (length == 0)) "
		"?([1, 2, 3] (F == 6) (length == 3))"},
	})
    {
      auto stk = std::make_unique <stack> ();
      auto yielded = run_query (*builtins, std::move (stk), entry.second);
      ASSERT_EQ (entry.first, yielded.size ());
    }
}

TEST_F (ZwTest, test_assert_subx)
{
  for (auto const &entry: std::map <size_t, std::string> {
	    {1, "1 ?(|A| A 1 ?eq)"},
	    {0, "2 ?(|A| A 1 ?eq)"},
	    {1, "1 !(|A| A 1 !eq)"},
	    {0, "2 !(|A| A 1 !eq)"},

	    {1, "1 1 ?(|A B| A B ?eq)"},
	    {0, "2 1 ?(|A B| A B ?eq)"},
	    {0, "1 2 ?(|A B| A B ?eq)"},

	    {1, "2 1 ?(|A B| A B ?eq)"},
	    {0, "1 2 ?(|A B| A B ?eq)"},
	    {0, "2 1 !(|A B| A B ?eq)"},
	    {1, "1 2 !(|A B| A B ?eq)"},
	})
    {
      auto stk = std::make_unique <stack> ();
      auto yielded = run_query (*builtins, std::move (stk), entry.second);
      ASSERT_EQ (entry.first, yielded.size ());
    }
}

TEST_F (ZwTest, test_assert_block)
{
  for (auto const &entry: std::map <size_t, std::string> {
	    {1, "let ?F := ?{|A| A == 1}; 1 ?F == 1"},
	    {0, "let ?F := ?{|A| A == 2}; 1 ?F == 1"},
	    {1, "let ?F := !{|A| A == 2}; 1 ?F == 1"},
	    {1, "let ?F := ?{swap}; 1 2 ?F [|A B| A, B] == [1, 2]"},
	    {0, "let ?F := !{swap}; 1 2 ?F [|A B| A, B] == [1, 2]"},
	})
    {
      auto stk = std::make_unique <stack> ();
      auto yielded = run_query (*builtins, std::move (stk), entry.second);
      ASSERT_EQ (entry.first, yielded.size ());
    }
}

TEST_F (ZwTest, test_various)
{
  for (auto const &entry: std::map <size_t, std::string> {
	    {2, "(1, 2) drop [4, 5] [] ?ne"},
	})
    {
      auto stk = std::make_unique <stack> ();
      auto yielded = run_query (*builtins, std::move (stk), entry.second);
      ASSERT_EQ (entry.first, yielded.size ());
    }
}
