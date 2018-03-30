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

#include "test-zw-aux.hh"
#include "test-dw-aux.hh"

using namespace test;

TEST_F (DwTest, test_elf)
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
	    {1, "y.o", "@elfclass == ELFCLASS32"},
	    {1, "a1.out", "@elfclass == ELFCLASS64"},
	    {1, "y.o", "elf @class == ELFCLASS32"},
	    {1, "a1.out", "elf @class == ELFCLASS64"},
	    {1, "y.o", "elf @elfclass == ELFCLASS32"},
	    {1, "a1.out", "elf @elfclass == ELFCLASS64"},

	    {1, "y.o", "elf !ET_DYN"},
	    {1, "a1.out", "elf !ET_DYN"},
	    {1, "y.o", "elf ?ET_REL"},
	    {1, "a1.out", "elf ?ET_EXEC"},
	    {1, "y.o", "elf @type == ET_REL"},
	    {1, "a1.out", "elf @type == ET_EXEC"},
	    {1, "y.o", "@elftype == ET_REL"},
	    {1, "a1.out", "@elftype == ET_EXEC"},
	    {1, "y.o", "elf @elftype == ET_REL"},
	    {1, "a1.out", "elf @elftype == ET_EXEC"},

	    {1, "y.o", "elf !EM_MIPS"},
	    {1, "a1.out", "elf !EM_MIPS"},
	    {1, "y.o", "elf ?EM_ARM"},
	    {1, "a1.out", "elf ?EM_X86_64"},
	    {1, "y.o", "elf @machine == EM_ARM"},
	    {1, "a1.out", "elf @machine == EM_X86_64"},
	    {1, "y.o", "@elfmachine == EM_ARM"},
	    {1, "a1.out", "@elfmachine == EM_X86_64"},
	    {1, "y.o", "elf @elfmachine == EM_ARM"},
	    {1, "a1.out", "elf @elfmachine == EM_X86_64"},
	})
    {
      size_t nresults = std::get <0> (entry);
      auto const &fn = std::get <1> (entry);
      auto const &q = std::get <2> (entry);

      auto yielded = run_dwquery (*builtins, fn, q);
      ASSERT_EQ (nresults, yielded.size ());
    }
}
