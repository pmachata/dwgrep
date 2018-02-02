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

#include <gtest/gtest.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "atval.hh"
#include "builtin-dw-abbrev.hh"
#include "builtin-dw.hh"
#include "builtin-symbol.hh"
#include "builtin.hh"
#include "dwit.hh"
#include "init.hh"
#include "op.hh"
#include "parser.hh"
#include "stack.hh"
#include "test-dw-aux.hh"
#include "test-zw-aux.hh"
#include "value-dw.hh"
#include "value-seq.hh"
#include "dwcst.hh"

using namespace test;

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

  std::unique_ptr <value_dwarf>
  rdw (std::string fn)
  {
    return dw (fn, doneness::raw);
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

namespace
{
  template <class T>
  size_t
  count (std::unique_ptr <value_producer <T>> prod)
  {
    size_t ret = 0;
    while (prod->next () != nullptr)
      ++ret;
    return ret;
  }
}

TEST_F (ZwTest, no_duplicate_abbrev_units)
{
  // dwz merges all abbreviations into a single unit that's reused
  // across all CU's.  Because .debug_abbrev has no header
  // information, we rely on CU iteration to discover the abbreviation
  // units, and thus were seeing duplicates in dwz files.

  layout l;
  ASSERT_EQ (2, count (op_abbrev_dwarf {l, nullptr}
			.operate (rdw ("dwz-partial2-1"))));
}

TEST_F (ZwTest, value_dwarf_doesnt_leak_fd)
{
  rlimit orig;
  int rc = getrlimit (RLIMIT_NOFILE, &orig);
  assert (rc == 0);

  rlimit rl = orig;
  assert (rl.rlim_cur >= 10);
  rl.rlim_cur = 10;
  rc = setrlimit (RLIMIT_NOFILE, &rl);
  assert (rc == 0);

  // Attempt to open a legitimate file several times.  If descriptors
  // are leaked, this will eventually start failing.
  for (int i = 0; i < 10; ++i)
    ASSERT_NO_THROW (rdw ("dwz-partial2-1"));

  // Attempt to create a value_dwarf from a file that's not a Dwarf.
  // Do it many, many times.
  for (int i = 0; i < 10; ++i)
    ASSERT_THROW (rdw ("char_16_32.cc"), std::runtime_error);

  // Now attempt to open a legitimate file.  If the file descriptor
  // leaks, this will fail.
  ASSERT_NO_THROW (rdw ("dwz-partial2-1"));

  rc = setrlimit (RLIMIT_NOFILE, &orig);
  assert (rc == 0);
}

namespace
{
  void
  get_sole_dwarf (char const *fn,
		  std::unique_ptr <value_dwarf> &ret_vdw, Dwarf *&ret_dw)
  {
    ret_vdw = nullptr;
    ret_dw = nullptr;

    auto vdw = rdw (fn);
    auto ctx = vdw->get_dwctx ();

    std::vector <dwfl_module> modules
      {dwfl_module_iterator {ctx->get_dwfl ()}, dwfl_module_iterator::end ()};
    ASSERT_EQ (1, modules.size ());

    ret_vdw = std::move (vdw);
    ret_dw = modules[0].dwarf ();
  }
}

TEST_F (ZwTest, attribute_die_cooked_no_dup)
{
  std::unique_ptr <value_dwarf> vdw;
  Dwarf *dw;
  get_sole_dwarf ("attribute-die-cooked-no-dup.o", vdw, dw);
  ASSERT_TRUE (vdw != nullptr);
  ASSERT_TRUE (dw != nullptr);

  bool seen_specification = false;
  bool seen_abstract_origin = false;

  layout l;
  value_die vd (vdw->get_dwctx (), dwpp_offdie (dw, 0x1c), 0, doneness::cooked);
  for (auto prod = op_attribute_die {l, nullptr}
			.operate (std::make_unique <value_die> (vd));
       auto va = prod->next (); )
    switch (int c = va->get_attr ().code)
      {
      case DW_AT_specification:
	EXPECT_FALSE (seen_specification);
	seen_specification = true;
	break;

      case DW_AT_abstract_origin:
	EXPECT_FALSE (seen_abstract_origin);
	seen_abstract_origin = true;
	break;

      default:
	FAIL() << "Unexpected attribute " << c << ".";
      }

  EXPECT_TRUE (seen_specification);
  EXPECT_TRUE (seen_abstract_origin);
}

TEST_F (ZwTest, entry_dwarf_counts_every_unit_anew)
{
  std::unique_ptr <value_dwarf> vdw;
  Dwarf *dw;
  get_sole_dwarf ("entry_dwarf_counts_every_unit_anew.o", vdw, dw);
  ASSERT_TRUE (vdw != nullptr);

  layout l;
  size_t i = 0;
  for (auto prod = op_entry_dwarf {l, nullptr}.operate (std::move (vdw));
       auto va = prod->next (); )
    ASSERT_EQ (i++, va->get_pos ());
}

namespace
{
  void
  expect_zero_DW_AT_const_value_on (char const *dwname,
				    Dwarf_Off die_off)
  {
    std::unique_ptr <value_dwarf> vdw;
    Dwarf *dw;
    get_sole_dwarf (dwname, vdw, dw);
    ASSERT_TRUE (vdw != nullptr);
    ASSERT_TRUE (dw != nullptr);

    value_die vd (vdw->get_dwctx (), dwpp_offdie (dw, die_off),
		  0, doneness::cooked);

    Dwarf_Die die = vd.get_die ();
    Dwarf_Attribute attr;
    ASSERT_TRUE (dwarf_attr (&die, DW_AT_const_value, &attr) != nullptr);

    value_attr at (vd, attr, 0, doneness::cooked);
    auto prod = at_value (vdw->get_dwctx (), vd, attr);
    ASSERT_TRUE (prod != nullptr);

    auto va = prod->next ();
    ASSERT_TRUE (va != nullptr);
    value_cst *cst = value::as <value_cst> (va.get ());
    ASSERT_TRUE (cst != nullptr);
    ASSERT_TRUE ((cst->get_constant () == constant {0, &dec_constant_dom}));

    ASSERT_TRUE (prod->next () == nullptr);
  }
}

TEST_F (ZwTest, const_value_on_enum_with_type)
{
  expect_zero_DW_AT_const_value_on ("const_value_on_enum_with_type.o", 0x27);
}

TEST_F (ZwTest, template_value_parameter_const_value_on_enum_with_type)
{
  expect_zero_DW_AT_const_value_on ("const_value_on_enum_with_type.o", 0x3f);
}

TEST_F (ZwTest, op_length_loclist_elem)
{
  layout l;
  op_length_loclist_elem lengther {l, nullptr};
  std::vector <unsigned> lengths = {1, 4, 1, 2, 1, 2};
  auto it = lengths.begin ();
  for (auto const &stk:
	 run_dwquery (*builtins, "bitcount.o", "entry @AT_location"))
    {
      ASSERT_EQ (1, stk->size ());

      auto val = stk->pop ();
      ASSERT_TRUE (val != nullptr);

      std::unique_ptr <value_loclist_elem> loc
		{value::as <value_loclist_elem> (val.release ())};
      ASSERT_TRUE (loc != nullptr);

      value_cst vcst = lengther.operate (std::move (loc));
      ASSERT_TRUE (it != lengths.end ());
      ASSERT_EQ ((constant {*it++, &dec_constant_dom}), vcst.get_constant ());
    }
  ASSERT_EQ (it, lengths.end ());
}

TEST_F (ZwTest, op_length_loclist_elem_word)
{
  // Above, we check op_length_loclist_elem as a C++ entity.  Here, we
  // make sure it operates well as a Zwerg word as well.
  ASSERT_EQ (1, run_dwquery
	     (*builtins, "bitcount.o",
	      "[entry @AT_location length] == [1, 4, 1, 2, 1, 2]").size ());
}

TEST_F (ZwTest, imported_AT_decl_file)
{
  std::unique_ptr <value_dwarf> vdw;
  Dwarf *dw;
  get_sole_dwarf ("imported-AT_decl_file.o", vdw, dw);
  ASSERT_TRUE (vdw != nullptr);
  ASSERT_TRUE (dw != nullptr);

  layout l;
  auto vd = std::make_unique <value_die>
		(vdw->get_dwctx (), dwpp_offdie (dw, 0x1f),
		 0, doneness::cooked);
  auto prod = op_atval_die {l, nullptr, DW_AT_decl_file}
		.operate (std::move (vd));
  ASSERT_TRUE (prod != nullptr);
  auto v = prod->next ();
  ASSERT_TRUE (v != nullptr);
  auto vs = value::as <value_str> (v.get ());
  ASSERT_TRUE (vs != nullptr);
  std::string n = vs->get_string ();
  ASSERT_EQ ("foo/foo.c", n);
  ASSERT_TRUE (prod->next () == nullptr);
}

namespace
{
  void
  test_builtin_constant (vocabulary &builtins, char const *name)
  {
    layout l;
    auto bi = builtins.find (name);
    ASSERT_TRUE (bi != nullptr);
    auto origin = std::make_shared <op_origin> (l);
    auto op = bi->build_exec (l, origin);
    ASSERT_TRUE (op != nullptr);

    scon sc {l};
    scon_guard sg {sc, *op};

    origin->set_next (sc, std::make_unique <stack> ());
    auto stk = op->next (sc);
    ASSERT_TRUE (stk != nullptr);
    ASSERT_EQ (1, stk->size ());
    auto val = stk->pop ();
    ASSERT_TRUE (val->is <value_cst> ());
    ASSERT_EQ (&slot_type_dom,
	       value::as <value_cst> (val.get ())->get_constant ().dom ());
    ASSERT_TRUE (op->next (sc) == nullptr);
  }
}

#define ADD_BUILTIN_CONSTANT_TEST(NAME)		\
  TEST_F (ZwTest, test_builtin_##NAME)		\
  {						\
    test_builtin_constant (*builtins, #NAME);	\
  }

ADD_BUILTIN_CONSTANT_TEST (T_ASET)
ADD_BUILTIN_CONSTANT_TEST (T_DWARF)
ADD_BUILTIN_CONSTANT_TEST (T_CU)
ADD_BUILTIN_CONSTANT_TEST (T_DIE)
ADD_BUILTIN_CONSTANT_TEST (T_ATTR)
ADD_BUILTIN_CONSTANT_TEST (T_ABBREV_UNIT)
ADD_BUILTIN_CONSTANT_TEST (T_ABBREV)
ADD_BUILTIN_CONSTANT_TEST (T_ABBREV_ATTR)
ADD_BUILTIN_CONSTANT_TEST (T_LOCLIST_ELEM)
ADD_BUILTIN_CONSTANT_TEST (T_LOCLIST_OP)
ADD_BUILTIN_CONSTANT_TEST (T_ELFSYM)
ADD_BUILTIN_CONSTANT_TEST (T_ELF)

#undef ADD_BUILTIN_CONSTANT_TEST

TEST_F (ZwTest, builtin_symbol_yields_once_per_symbol)
{
  layout l;
  size_t count = 0;
  std::vector <std::string> names = {"", "enum.cc", "", "", "", "", "", "", "",
				     "", "", "", "ae", "af"};
  for (auto prod = op_symbol_dwarf {l, nullptr}.operate (rdw ("enum.o"));
       auto val = prod->next ();)
    {
      ASSERT_LT (count, names.size ());
      EXPECT_EQ (names[count], val->get_name ());
      count++;
    }
  EXPECT_EQ (names.size (), count);

  ASSERT_EQ (1, run_dwquery
	     (*builtins, "enum.o",
	      "[symbol name] =="
	      "[\"\", \"enum.cc\", \"\", \"\", \"\", \"\", \"\", \"\", \"\","
	      "\"\", \"\", \"\", \"ae\", \"af\"]").size ());
}

TEST_F (ZwTest, builtin_symbol_label)
{
  layout l;
  std::vector <unsigned> results = {
    STT_NOTYPE, STT_FILE, STT_SECTION, STT_SECTION, STT_SECTION, STT_SECTION,
    STT_SECTION, STT_SECTION, STT_SECTION, STT_SECTION, STT_SECTION,
    STT_SECTION, STT_OBJECT, STT_OBJECT
  };

  op_label_symbol op {l, nullptr};

  size_t count = 0;
  for (auto prod = op_symbol_dwarf {l, nullptr}.operate (rdw ("enum.o"));
       auto val = prod->next ();)
    {
      ASSERT_LT (count, results.size ());

      value_cst cst = op.operate (std::move (val));
      ASSERT_EQ (&elfsym_stt_dom (EM_X86_64), cst.get_constant ().dom ());
      EXPECT_EQ (results[count], cst.get_constant ().value ());

      count++;
    }
  EXPECT_EQ (results.size (), count);

  EXPECT_EQ (1, run_dwquery
	     (*builtins, "enum.o",
	      "[symbol label] == "
	      "[STT_NOTYPE, STT_FILE, STT_SECTION, STT_SECTION,"
	      " STT_SECTION, STT_SECTION, STT_SECTION, STT_SECTION,"
	      " STT_SECTION, STT_SECTION, STT_SECTION, STT_SECTION,"
	      " STT_OBJECT, STT_OBJECT]").size ());

  EXPECT_EQ (1, run_dwquery
	     (*builtins, "enum.o",
	      "[symbol label \"%s\"] == "
	      "[\"STT_NOTYPE\", \"STT_FILE\", \"STT_SECTION\", \"STT_SECTION\","
	      " \"STT_SECTION\", \"STT_SECTION\", \"STT_SECTION\","
	      " \"STT_SECTION\", \"STT_SECTION\", \"STT_SECTION\","
	      " \"STT_SECTION\", \"STT_SECTION\", \"STT_OBJECT\","
	      " \"STT_OBJECT\"]").size ());

  EXPECT_EQ (1, run_dwquery
	     (*builtins, "y.o",
	      "symbol (name == \"main\") label == STT_ARM_TFUNC")
	     .size ());

  // This is ARM object, and ARM has a per-arch ELF constants, and the
  // label constants thus come from a STT_ARM_ domain..  But
  // STT_SECTION by itself has a plain domain.  Check that the two are
  // compared as they should.
  EXPECT_EQ (6, run_dwquery (*builtins, "y.o",
			     "symbol (label == STT_SECTION)").size ());

  // STT_ARM_16BIT used to be omitted by known-elf.awk.
  ASSERT_TRUE (builtins->find ("STT_ARM_16BIT") != nullptr);
}

TEST_F (ZwTest, builtin_symbol_binding)
{
  layout l;
  std::vector <unsigned> results = {
    STB_LOCAL, STB_LOCAL, STB_LOCAL, STB_LOCAL, STB_LOCAL, STB_LOCAL,
    STB_LOCAL, STB_LOCAL, STB_LOCAL, STB_LOCAL, STB_LOCAL, STB_LOCAL,
    STB_GLOBAL, STB_GLOBAL
  };

  op_binding_symbol op {l, nullptr};

  size_t count = 0;
  for (auto prod = op_symbol_dwarf {l, nullptr}.operate (rdw ("enum.o"));
       auto val = prod->next ();)
    {
      ASSERT_LT (count, results.size ());

      value_cst cst = op.operate (std::move (val));
      ASSERT_EQ (&elfsym_stb_dom (EM_X86_64), cst.get_constant ().dom ());
      EXPECT_EQ (results[count], cst.get_constant ().value ());

      count++;
    }
  EXPECT_EQ (results.size (), count);

  EXPECT_EQ (1, run_dwquery
	     (*builtins, "enum.o",
	      "[symbol binding] == "
	      "[STB_LOCAL, STB_LOCAL, STB_LOCAL, STB_LOCAL, STB_LOCAL, "
	      " STB_LOCAL, STB_LOCAL, STB_LOCAL, STB_LOCAL, STB_LOCAL, "
	      " STB_LOCAL, STB_LOCAL, STB_GLOBAL, STB_GLOBAL]")
	     .size ());

  EXPECT_EQ (1, run_dwquery
	     (*builtins, "enum.o",
	      "[symbol binding \"%s\"] == "
	      "[\"STB_LOCAL\",\"STB_LOCAL\",\"STB_LOCAL\",\"STB_LOCAL\","
	      " \"STB_LOCAL\",\"STB_LOCAL\",\"STB_LOCAL\",\"STB_LOCAL\","
	      " \"STB_LOCAL\",\"STB_LOCAL\",\"STB_LOCAL\",\"STB_LOCAL\","
	      " \"STB_GLOBAL\",\"STB_GLOBAL\"]")
	     .size ());

  EXPECT_EQ (1, run_dwquery
	     (*builtins, "y-mips.o",
	      "symbol (name == \"main\") binding == STB_MIPS_SPLIT_COMMON")
	     .size ());

  // See above at "label" for reasoning behind this test.
  EXPECT_EQ (9, run_dwquery (*builtins, "y-mips.o",
			     "symbol (binding == STB_LOCAL)").size ());
}

TEST_F (ZwTest, builtin_symbol_visibility)
{
  layout l;
  op_visibility_symbol op {l, nullptr};

  size_t count = 0;
  for (auto prod = op_symbol_dwarf {l, nullptr}.operate (rdw ("enum.o"));
       auto val = prod->next ();)
    {
      value_cst cst = op.operate (std::move (val));
      ASSERT_EQ (&elfsym_stv_dom (), cst.get_constant ().dom ());
      EXPECT_EQ (STV_DEFAULT, cst.get_constant ().value ());

      count++;
    }
  EXPECT_EQ (14, count);

#define TEST_STV(NAME)					\
  {							\
    std::stringstream ss;				\
    ss << constant {NAME, &elfsym_stv_dom ()};		\
    EXPECT_EQ (#NAME, ss.str ());			\
    EXPECT_TRUE (builtins->find (#NAME) != nullptr);	\
  }

  TEST_STV (STV_DEFAULT)
  TEST_STV (STV_INTERNAL)
  TEST_STV (STV_HIDDEN)
  TEST_STV (STV_PROTECTED)

#undef TEST_STV
}

TEST_F (ZwTest, builtin_symbol_size)
{
  layout l;
  std::vector <size_t> results = {4, 137, 11, 0, 0, 0, 16, 0, 0};

  op_size_symbol op {l, nullptr};

  size_t count = 0;
  for (auto prod = op_symbol_dwarf {l, nullptr}.operate (rdw ("twocus"));
       auto val = prod->next ();)
    if (val->get_pos () >= 63)
      {
	ASSERT_LT (count, results.size ());

	value_cst cst = op.operate (std::move (val));
	EXPECT_TRUE (cst.get_constant ().dom ()->safe_arith ());
	EXPECT_EQ (results[count], cst.get_constant ().value ());

	count++;
      }

  EXPECT_EQ (results.size (), count);

  EXPECT_EQ (1, run_dwquery
	     (*builtins, "twocus",
	      "[symbol (pos >= 63) size] == [4, 137, 11, 0, 0, 0, 16, 0, 0]")
	     .size ());
}

TEST_F (ZwTest, builtin_symbol_address_value)
{
  layout l;
  std::vector <GElf_Addr> results = {
    0x4005b8, 0x4004d0, 0x4004b2, 0x601038, 0x4003d0, 0x601024,
    0x4004bd, 0, 0x400390
  };

  op_address_symbol op {l, nullptr};

  size_t count = 0;
  for (auto prod = op_symbol_dwarf {l, nullptr}.operate (rdw ("twocus"));
       auto val = prod->next ();)
    if (val->get_pos () >= 63)
      {
	ASSERT_LT (count, results.size ());

	value_cst cst = op.operate (std::move (val));
	EXPECT_TRUE (cst.get_constant ().dom ()->safe_arith ());
	EXPECT_EQ (results[count], cst.get_constant ().value ());

	count++;
      }

  EXPECT_EQ (results.size (), count);

  EXPECT_EQ (1, run_dwquery
	     (*builtins, "twocus",
	      "[symbol (pos >= 63) address] == "
	      "[0x4005b8, 0x4004d0, 0x4004b2, 0x601038, 0x4003d0, 0x601024,"
	      " 0x4004bd, 0, 0x400390]")
	     .size ());

  EXPECT_EQ (1, run_dwquery
	     (*builtins, "twocus",
	      "[symbol (pos >= 63) value] == "
	      "[0x4005b8, 0x4004d0, 0x4004b2, 0x601038, 0x4003d0, 0x601024,"
	      " 0x4004bd, 0, 0x400390]")
	     .size ());
}

TEST_F (ZwTest, symbol_cmp)
{
  layout l;
  // Test all symbols on equality with itself.
  for (auto prod = op_symbol_dwarf {l, nullptr}.operate (rdw ("enum.o"));
       auto val = prod->next (); )
    EXPECT_EQ (cmp_result::equal, val->cmp (*val));
}

namespace
{
  void
  test_pairs (vocabulary &builtins,
	      std::string fn, std::string query,
	      std::vector <std::pair <constant, std::string>> results)
  {
    auto yielded = run_dwquery (builtins, fn, query);
    EXPECT_EQ (results.size (), yielded.size ());

    for (size_t i = 0; i < results.size (); ++i)
      {
	auto stk = std::move (yielded[i]);
	ASSERT_EQ (1, stk->size ());
	auto tos = stk->pop ();
	std::shared_ptr <value_seq::seq_t> seq
	  = value::require_as <value_seq> (&*tos).get_seq ();
	ASSERT_EQ (2, seq->size ());

	auto v1 = std::move ((*seq)[0]);
	constant cst = value::require_as <value_cst> (&*v1).get_constant ();
	EXPECT_EQ (results[i].first, cst);

	auto v2 = std::move ((*seq)[1]);
	std::string str = value::require_as <value_str> (&*v2).get_string ();
	EXPECT_EQ (results[i].second, str);
      }
  }
}

TEST_F (ZwTest, test_defaulted)
{
  test_pairs (*builtins, "defaulted.o",
	      ("entry ?(raw ?DW_AT_defaulted)"
	       "[|A| A @DW_AT_defaulted, A name]"),
	{{constant {DW_DEFAULTED_in_class, &dw_defaulted_dom ()}, "Foo"},
	 {constant {DW_DEFAULTED_out_of_class, &dw_defaulted_dom ()}, "Bar"}});
}

TEST_F (ZwTest, test_various)
{
  for (auto const &entry: std::map <size_t, std::string> {
	    {3, "DW_DEFAULTED_no, DW_DEFAULTED_in_class, "
		"DW_DEFAULTED_out_of_class"},
	})
    {
      auto stk = std::make_unique <stack> ();
      auto yielded = run_query (*builtins, std::move (stk), entry.second);
      ASSERT_EQ (entry.first, yielded.size ());
    }
}

TEST_F (ZwTest, test_const_value_block)
{
  test_pairs (*builtins, "const_value_block.o",
	      ("entry ?TAG_template_value_parameter "
	       "[|E| E @DW_AT_const_value, E parent name]"),
	{{constant {0, &bool_constant_dom}, "aaa<int*, false>"},
	 {constant {1, &bool_constant_dom}, "aaa<int&, true>"},
	 {constant {-7, &dec_constant_dom}, "bbb<int*, -7>"},
	 {constant {21, &dec_constant_dom}, "bbb<int&, 21>"},
	 {constant {7, &dec_constant_dom}, "ccc<int*, 7>"},
	 {constant {3000000000, &dec_constant_dom}, "ccc<int&, 3000000000>"},
	 {constant {-7, &dec_constant_dom}, "ddd<int*, -7>"},
	 {constant {21, &dec_constant_dom}, "ddd<int&, 21>"},
	 {constant {7, &dec_constant_dom}, "eee<int*, 7>"},
	 {constant {254, &dec_constant_dom}, "eee<int&, 254>"},
	 {constant {-7, &dec_constant_dom}, "fff<int*, -7>"},
	 {constant {21, &dec_constant_dom}, "fff<int&, 21>"},
	 {constant {7, &dec_constant_dom}, "ggg<int*, 7>"},
	 {constant {6000000000, &dec_constant_dom}, "ggg<int&, 6000000000>"}});
}
