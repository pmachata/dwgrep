/*
   Copyright (C) 2018 Petr Machata
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

#include "builtin-symbol.hh"
#include "test-zw-aux.hh"
#include "test-dw-aux.hh"

using namespace test;

#define ADD_BUILTIN_CONSTANT_TEST(NAME)		\
  TEST_F (ElfTest, test_builtin_##NAME)		\
  {						\
    test_builtin_constant (*builtins, #NAME);	\
  }

ADD_BUILTIN_CONSTANT_TEST (T_ELFSYM)
ADD_BUILTIN_CONSTANT_TEST (T_ELF)

#undef ADD_BUILTIN_CONSTANT_TEST

TEST_F (ElfTest, builtin_symbol_yields_once_per_symbol)
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

TEST_F (ElfTest, builtin_symbol_label)
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

TEST_F (ElfTest, builtin_symbol_binding)
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

TEST_F (ElfTest, builtin_symbol_visibility)
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

TEST_F (ElfTest, builtin_symbol_size)
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

TEST_F (ElfTest, builtin_symbol_address_value)
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

TEST_F (ElfTest, symbol_cmp)
{
  layout l;
  // Test all symbols on equality with itself.
  for (auto prod = op_symbol_dwarf {l, nullptr}.operate (rdw ("enum.o"));
       auto val = prod->next (); )
    EXPECT_EQ (cmp_result::equal, val->cmp (*val));
}

TEST_F (ElfTest, test_elf)
{
  typedef std::vector <std::tuple <size_t, std::string, std::string>> vec;
  for (auto const &entry: vec {
	    {2, "twocus-copy", "elf"},
	    {1, "twocus-copy", "elf \"%s\" =~ \"twocus-copy[^.]\""},
	    {1, "twocus-copy", "elf \"%s\" =~ \"twocus-copy.dbg\""},
	    {1, "twocus-copy", "elf name =~ \"twocus-copy$\""},
	    {1, "twocus-copy", "elf name =~ \"twocus-copy.dbg\""},
	    {1, "twocus", "(|Dw| [Dw symbol name] == [Dw elf symbol name])"},
	    {5, "twocus-copy",
		"(|Dw| Dw elf ?1 symbol !(name == Dw elf ?0 symbol name))"},

	    {1, "y.o", "!ELFCLASS64"},
	    {1, "a1.out", "!ELFCLASS32"},
	    {1, "y.o", "?ELFCLASS32"},
	    {1, "a1.out", "?ELFCLASS64"},
	    {1, "y.o", "elf ?ELFCLASS32"},
	    {1, "a1.out", "elf ?ELFCLASS64"},
	    {1, "y.o", "eclass == ELFCLASS32"},
	    {1, "a1.out", "eclass == ELFCLASS64"},
	    {1, "y.o", "elf eclass == ELFCLASS32"},
	    {1, "a1.out", "elf eclass == ELFCLASS64"},

	    {1, "y.o", "elf !ET_DYN"},
	    {1, "a1.out", "elf !ET_DYN"},
	    {1, "y.o", "elf ?ET_REL"},
	    {1, "a1.out", "elf ?ET_EXEC"},
	    {1, "y.o", "elf etype == ET_REL"},
	    {1, "a1.out", "elf etype == ET_EXEC"},
	    {1, "y.o", "etype == ET_REL"},
	    {1, "a1.out", "etype == ET_EXEC"},

	    {1, "y.o", "elf !EM_MIPS"},
	    {1, "a1.out", "elf !EM_MIPS"},
	    {1, "y.o", "elf ?EM_ARM"},
	    {1, "a1.out", "elf ?EM_X86_64"},
	    {1, "y.o", "elf emachine == EM_ARM"},
	    {1, "a1.out", "elf emachine == EM_X86_64"},
	    {1, "y.o", "emachine == EM_ARM"},
	    {1, "a1.out", "emachine == EM_X86_64"},
	})
    {
      size_t nresults = std::get <0> (entry);
      auto const &fn = std::get <1> (entry);
      auto const &q = std::get <2> (entry);

      auto yielded = run_dwquery (*builtins, fn, q);
      ASSERT_EQ (nresults, yielded.size ());
    }
}
