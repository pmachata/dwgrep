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

#include <sys/time.h>
#include <sys/resource.h>

#include "atval.hh"
#include "builtin-dw-abbrev.hh"
#include "builtin-dw.hh"
#include "builtin-symbol.hh"
#include "builtin.hh"
#include "dwit.hh"
#include "op.hh"
#include "stack.hh"
#include "test-dw-aux.hh"
#include "test-zw-aux.hh"
#include "value-dw.hh"
#include "dwcst.hh"

using namespace test;

TEST (DwValueTest, dwarf_sanity)
{
  auto cooked = dw ("empty", doneness::cooked);
  ASSERT_TRUE (cooked->is_cooked ());
  ASSERT_FALSE (cooked->is_raw ());

  auto raw = dw ("empty", doneness::raw);
  ASSERT_TRUE (raw->is_raw ());
  ASSERT_FALSE (raw->is_cooked ());
}

TEST_F (DwTest, words_dwarf_raw_cooked)
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

TEST_F (DwTest, words_cu_raw_cooked)
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

TEST_F (DwTest, words_die_raw_cooked)
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

TEST_F (DwTest, words_attr_raw_cooked)
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

TEST_F (DwTest, cooked_attribute_assertion_integrates_attributes)
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

TEST_F (DwTest, name_on_raw_cooked_die)
{
  ASSERT_EQ (1, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) name").size ());
  ASSERT_EQ (0, run_dwquery (*builtins, "nullptr.o",
			     "entry (offset == 0x6e) raw name").size ());
}

TEST_F (DwTest, raw_and_cooked_values_compare_equal)
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

TEST_F (DwTest, entry_unit_abbrev_iterate_through_alt_file)
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

TEST_F (DwTest, root_with_alt_file)
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

TEST_F (DwTest, parent_with_alt_file)
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

TEST_F (DwTest, dies_from_two_files_neq)
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

TEST_F (DwTest, no_duplicate_abbrev_units)
{
  // dwz merges all abbreviations into a single unit that's reused
  // across all CU's.  Because .debug_abbrev has no header
  // information, we rely on CU iteration to discover the abbreviation
  // units, and thus were seeing duplicates in dwz files.

  layout l;
  ASSERT_EQ (2, count (op_abbrev_dwarf {l, nullptr}
			.operate (rdw ("dwz-partial2-1"))));
}

TEST_F (DwTest, value_dwarf_doesnt_leak_fd)
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

TEST_F (DwTest, attribute_die_cooked_no_dup)
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

TEST_F (DwTest, entry_dwarf_counts_every_unit_anew)
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
    auto prod = at_value_cooked (vdw->get_dwctx (), vd, attr);
    ASSERT_TRUE (prod != nullptr);

    auto va = prod->next ();
    ASSERT_TRUE (va != nullptr);
    value_cst *cst = value::as <value_cst> (va.get ());
    ASSERT_TRUE (cst != nullptr);
    ASSERT_TRUE ((cst->get_constant () == constant {0, &dec_constant_dom}));

    ASSERT_TRUE (prod->next () == nullptr);
  }
}

TEST_F (DwTest, const_value_on_enum_with_type)
{
  expect_zero_DW_AT_const_value_on ("const_value_on_enum_with_type.o", 0x27);
}

TEST_F (DwTest, template_value_parameter_const_value_on_enum_with_type)
{
  expect_zero_DW_AT_const_value_on ("const_value_on_enum_with_type.o", 0x3f);
}

TEST_F (DwTest, op_length_loclist_elem)
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

TEST_F (DwTest, op_length_loclist_elem_word)
{
  // Above, we check op_length_loclist_elem as a C++ entity.  Here, we
  // make sure it operates well as a Zwerg word as well.
  ASSERT_EQ (1, run_dwquery
	     (*builtins, "bitcount.o",
	      "[entry @AT_location length] == [1, 4, 1, 2, 1, 2]").size ());
}

TEST_F (DwTest, imported_AT_decl_file)
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

#define ADD_BUILTIN_CONSTANT_TEST(NAME)		\
  TEST_F (DwTest, test_builtin_##NAME)		\
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

#undef ADD_BUILTIN_CONSTANT_TEST

TEST_F (DwTest, test_defaulted)
{
  test_pairs (*builtins, "defaulted.o",
	      ("entry ?(raw ?DW_AT_defaulted)"
	       "[|A| A @DW_AT_defaulted, A name]"),
	{{constant {DW_DEFAULTED_in_class, &dw_defaulted_dom ()}, "Foo"},
	 {constant {DW_DEFAULTED_out_of_class, &dw_defaulted_dom ()}, "Bar"}});
}

TEST_F (DwTest, test_various)
{
  for (auto const &entry: std::vector <std::pair <size_t, std::string>> {
	    {3, "DW_DEFAULTED_no, DW_DEFAULTED_in_class, "
		"DW_DEFAULTED_out_of_class"},
	})
    {
      auto stk = std::make_unique <stack> ();
      auto yielded = run_query (*builtins, std::move (stk), entry.second);
      ASSERT_EQ (entry.first, yielded.size ())
	<< "In '" << entry.second << "'";
    }
}

TEST_F (DwTest, test_const_value_block)
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

TEST_F (DwTest, test_raw_attribute_strp)
{
  ASSERT_EQ (1, run_dwquery
	     (*builtins, "enum.o",
	      "[|Dw| Dw entry attribute ?(form == DW_FORM_strp) raw value] "
	      " == [0x1f, 0, 0x8]"
	      ).size ());

  ASSERT_EQ (1, run_dwquery
	     (*builtins, "a1.out",
	      "[|Dw| Dw entry attribute ?(form == DW_FORM_GNU_strp_alt) raw value] "
	      " == [0, 0x27, 0x32, 0x2d]"
	      ).size ());
}
