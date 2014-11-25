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

#include <gtest/gtest.h>

#include "builtin.hh"
#include "builtin-dw.hh"
#include "init.hh"
#include "value-dw.hh"
#include "stack.hh"
#include "parser.hh"
#include "op.hh"

std::string
test_file (std::string name)
{
  extern std::string g_test_case_directory;
  return g_test_case_directory + "/" + name;
}

struct ZwTest
  : public testing::Test
{
  std::unique_ptr <vocabulary> builtins;

  void
  SetUp () override final
  {
    builtins = std::make_unique <vocabulary>
      (*dwgrep_vocabulary_core (), *dwgrep_vocabulary_dw ());
  }
};

namespace
{
  std::unique_ptr <value_dwarf>
  dw (std::string fn, doneness d)
  {
    return std::make_unique <value_dwarf> (test_file (fn), 0, d);
  }
}

TEST (DwValueTest, dwarf_sanity)
{
  auto cooked = dw ("empty", doneness::cooked);
  ASSERT_TRUE (cooked->is_cooked ());
  ASSERT_FALSE (cooked->is_raw ());

  auto raw = dw ("empty", doneness::raw);
  ASSERT_TRUE (raw->is_raw ());
  ASSERT_FALSE (raw->is_cooked ());
}

namespace
{
  std::unique_ptr <stack>
  stack_with_value (std::unique_ptr <value> v)
  {
    auto stk = std::make_unique <stack> ();
    stk->push (std::move (v));
    return stk;
  }

  std::vector <std::unique_ptr <stack>>
  run_query (vocabulary &voc,
	     std::unique_ptr <stack> stk, std::string q)
  {
    std::shared_ptr <op> op = parse_query (voc, q)
      .build_exec (std::make_shared <op_origin> (std::move (stk)));

    std::vector <std::unique_ptr <stack>> yielded;
    while (auto r = op->next ())
      yielded.push_back (std::move (r));

    return yielded;
  }

  // A query on a stack with sole value, which is a Dwarf.
  std::vector <std::unique_ptr <stack>>
  run_dwquery (vocabulary &voc, std::string fn, std::string q)
  {
    return run_query (voc, stack_with_value (dw (fn, doneness::cooked)), q);
  }

#define SOLE_YIELDED_VALUE(TYPE, YIELDED)				\
  ({									\
    std::vector <std::unique_ptr <stack>> &_yielded = (YIELDED);	\
    									\
    ASSERT_EQ (1, _yielded.size ())					\
      << "One result expected.";					\
									\
    ASSERT_EQ (1, _yielded[0]->size ())					\
      << "Stack with one value expected.";				\
									\
    auto produced = value::as <TYPE> (&_yielded[0]->get (0));		\
    ASSERT_TRUE (produced != nullptr);					\
									\
    *produced;								\
  })
}

TEST_F (ZwTest, words_dwarf_raw_cooked)
{
  {
    auto yielded = run_query
      (*builtins, stack_with_value (dw ("empty", doneness::cooked)),
       "raw");

    auto produced = SOLE_YIELDED_VALUE (value_dwarf, yielded);
    ASSERT_TRUE (produced.is_raw ());
  }

  {
    auto yielded = run_query
      (*builtins, stack_with_value (dw ("empty", doneness::raw)),
       "cooked");

    auto produced = SOLE_YIELDED_VALUE (value_dwarf, yielded);
    ASSERT_TRUE (produced.is_cooked ());
  }
}

TEST_F (ZwTest, words_cu_raw_cooked)
{
  {
    auto yielded = run_dwquery (*builtins, "empty", "unit");
    auto produced = SOLE_YIELDED_VALUE (value_cu, yielded);
    ASSERT_TRUE (produced.is_cooked ());
  }

  {
    auto yielded = run_dwquery (*builtins, "empty", "unit raw");
    auto produced = SOLE_YIELDED_VALUE (value_cu, yielded);
    ASSERT_TRUE (produced.is_raw ());
  }

  {
    auto yielded = run_dwquery (*builtins, "empty", "raw unit");
    auto produced = SOLE_YIELDED_VALUE (value_cu, yielded);
    ASSERT_TRUE (produced.is_raw ());
  }

  {
    auto yielded = run_dwquery (*builtins, "empty", "raw unit cooked");
    auto produced = SOLE_YIELDED_VALUE (value_cu, yielded);
    ASSERT_TRUE (produced.is_cooked ());
  }

  {
    auto yielded = run_dwquery (*builtins, "empty", "entry raw unit");
    auto produced = SOLE_YIELDED_VALUE (value_cu, yielded);
    EXPECT_TRUE (produced.is_raw ());
  }
}

TEST_F (ZwTest, words_die_raw_cooked)
{
  {
    auto yielded = run_dwquery (*builtins, "empty", "entry");
    auto produced = SOLE_YIELDED_VALUE (value_die, yielded);
    EXPECT_TRUE (produced.is_cooked ());
  }

  {
    auto yielded = run_dwquery (*builtins, "empty", "entry raw");
    auto produced = SOLE_YIELDED_VALUE (value_die, yielded);
    EXPECT_TRUE (produced.is_raw ());
  }

  {
    auto yielded = run_dwquery (*builtins, "empty", "raw entry");
    auto produced = SOLE_YIELDED_VALUE (value_die, yielded);
    EXPECT_TRUE (produced.is_raw ());
  }

  {
    auto yielded = run_dwquery (*builtins, "empty", "raw entry cooked");
    auto produced = SOLE_YIELDED_VALUE (value_die, yielded);
    EXPECT_TRUE (produced.is_cooked ());
  }

  {
    auto yielded = run_dwquery (*builtins, "empty", "raw unit entry");
    auto produced = SOLE_YIELDED_VALUE (value_die, yielded);
    EXPECT_TRUE (produced.is_raw ());
  }

  {
    auto yielded = run_dwquery (*builtins, "twocus", "raw unit root");
    ASSERT_EQ (2, yielded.size ());
    for (auto &stk: yielded)
      {
	ASSERT_EQ (1, stk->size ());
	auto produced = value::as <value_die> (&stk->get (0));
	ASSERT_TRUE (produced != nullptr);
	ASSERT_TRUE (produced->is_raw ());
      }
  }

  {
    auto yielded = run_dwquery (*builtins, "empty", "raw unit entry");
    auto produced = SOLE_YIELDED_VALUE (value_die, yielded);
    EXPECT_TRUE (produced.is_raw ());
  }
}

TEST_F (ZwTest, words_attr_raw_cooked)
{
  {
    auto yielded = run_dwquery (*builtins, "empty",
				"entry attribute (pos == 0)");
    auto produced = SOLE_YIELDED_VALUE (value_attr, yielded);
    ASSERT_TRUE (produced.is_cooked ());
  }

  {
    auto yielded = run_dwquery (*builtins, "empty",
				"entry attribute (pos == 0) raw");
    auto produced = SOLE_YIELDED_VALUE (value_attr, yielded);
    ASSERT_TRUE (produced.is_raw ());
  }

  {
    auto yielded = run_dwquery (*builtins, "empty",
				"raw entry attribute (pos == 0)");
    auto produced = SOLE_YIELDED_VALUE (value_attr, yielded);
    ASSERT_TRUE (produced.is_raw ());
  }

  {
    auto yielded = run_dwquery (*builtins, "empty",
				"raw entry attribute (pos == 0) cooked");
    auto produced = SOLE_YIELDED_VALUE (value_attr, yielded);
    ASSERT_TRUE (produced.is_cooked ());
  }

  {
    auto yielded = run_dwquery (*builtins, "empty",
				"entry raw attribute (pos == 0)");
    auto produced = SOLE_YIELDED_VALUE (value_attr, yielded);
    EXPECT_TRUE (produced.is_raw ());
  }
}

TEST_F (ZwTest, cooked_attribute_assertion_integrates_attributes)
{
  ASSERT_EQ (1, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) ?AT_name").size ());
  ASSERT_EQ (0, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) !AT_name").size ());
  ASSERT_EQ (1, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) @AT_name").size ());

  // But it shouldn't blindly integrate everything.
  ASSERT_EQ (0, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) ?AT_declaration").size ());
  ASSERT_EQ (1, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) !AT_declaration").size ());

  // And shouldn't integrate in raw mode either.
  ASSERT_EQ (0, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) raw ?AT_name").size ());
  ASSERT_EQ (1, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) raw !AT_name").size ());
  ASSERT_EQ (0, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) raw @AT_name").size ());
}

TEST_F (ZwTest, name_on_raw_cooked_die)
{
  ASSERT_EQ (1, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) name").size ());
  ASSERT_EQ (0, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) raw name").size ());
}

TEST_F (ZwTest, raw_and_cooked_values_compare_equal)
{
  ASSERT_EQ (1, run_dwquery
	     (*builtins, "empty",
	      "(|A| [A] == [A raw])").size ());

  ASSERT_EQ (1, run_dwquery
	     (*builtins, "empty",
	      "(|A| [A unit] == [A raw unit])").size ());

  ASSERT_EQ (1, run_dwquery
	     (*builtins, "empty",
	      "(|A| [A unit entry] == [A raw unit entry])").size ());

  ASSERT_EQ (1, run_dwquery
	     (*builtins, "empty",
	      "(|A| [A unit entry attribute] "
	      "== [A raw unit entry attribute])").size ());
}

TEST_F (ZwTest, entry_unit_abbrev_iterate_through_alt_file)
{
  // Show root entries in a1.out, which should show a compile unit
  // from the main file and a partial unit entries from the alt file.
  ASSERT_EQ (1, run_dwquery
	     (*builtins, "a1.out",
	      "[raw entry ?root] "
	      "?(elem ?TAG_compile_unit) ?(elem ?TAG_partial_unit) (length==2)"
	      ).size ());

  // Check that unit behaves like that as well.
  ASSERT_EQ (1, run_dwquery
	     (*builtins, "a1.out",
	      "[raw unit root] "
	      "?(elem ?TAG_compile_unit) ?(elem ?TAG_partial_unit) (length==2)"
	      ).size ());

  // And abbrev should show also abbreviation units from the alt file.
  ASSERT_EQ (1, run_dwquery
	     (*builtins, "a1.out",
	      "[abbrev entry (?TAG_partial_unit, ?TAG_compile_unit)] "
	      "?(elem ?TAG_compile_unit) ?(elem ?TAG_partial_unit) (length==2)"
	      ).size ());
}

TEST_F (ZwTest, root_with_alt_file)
{
#define DOIT(FN)							\
  do									\
    {									\
      ASSERT_EQ (0, run_dwquery						\
		 (*builtins, FN,					\
		  "entry ?root !TAG_compile_unit").size ());		\
      ASSERT_EQ (0, run_dwquery						\
		 (*builtins, "dwz-partial3-1",				\
		  "raw entry ?root "					\
		  "!TAG_compile_unit !TAG_partial_unit").size ());	\
									\
      /* Test that we actually see all roots.  */			\
      ASSERT_EQ (7, run_dwquery						\
		 (*builtins, "dwz-partial3-1",				\
		  "raw entry ?root").size ());				\
    }									\
  while (false)

  // These two files are very similar.  The first one has a property
  // that there's a root DIE in gnu_debugaltlink file that has the
  // same offset as a non-root DIE in main file.  The other has a
  // symmetrical property.  We use these files to make sure that
  // offsets are not confused between main and altfile.
  DOIT ("dwz-partial2-1");
  DOIT ("dwz-partial3-1");

#undef DOIT
}

TEST_F (ZwTest, parent_with_alt_file)
{
  // dwz-partial4-1.o uses dwz-partial4-C as its altlink file.  Both
  // files have DIE's on the same offset, but their structure is
  // different.  When DIE's with offset 0xd are selected, that matches
  // one DIE from the main file and one from the altfile.  Then we
  // request parent, and that has to yield 0xb in the main file and
  // 0xc in the altfile.
  ASSERT_EQ (1, run_dwquery
	     (*builtins, "dwz-partial4-1.o",
	      "raw entry (offset == 0xd) parent (offset == 0xb)").size ());

  // Check that parent can go back from altfile to main file across
  // partial imports.
  ASSERT_EQ (1, run_dwquery
	     (*builtins, "a1.out",
	      "entry (offset == 0x14) parent").size ());
}

TEST_F (ZwTest, dies_from_two_files_neq)
{
  // Two DIE's with the same offset should not compare equal if one of
  // them comes from alt-file and the other from the main file.
  ASSERT_EQ (0, run_dwquery
	     (*builtins, "a1.out",
	      "[raw unit root] (elem (pos == 0) == elem (pos == 1))").size ());
}
