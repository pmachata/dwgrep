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

#include "builtin-core-voc.hh"
#include "builtin-dw-voc.hh"
#include "builtin-elf-voc.hh"
#include "test-dw-aux.hh"
#include "test-zw-aux.hh"
#include "value-dw.hh"
#include "value-seq.hh"
#include "value-str.hh"

std::unique_ptr <value_dwarf>
test::dw (std::string fn, doneness d)
{
  return std::make_unique <value_dwarf> (test_file_name (fn), 0, d);
}

std::unique_ptr <value_dwarf>
test::rdw (std::string fn)
{
  return dw (fn, doneness::raw);
}

std::vector <std::unique_ptr <stack>>
test::run_dwquery (vocabulary &voc, std::string fn, std::string q)
{
  return run_query (voc, stack_with_value (dw (fn, doneness::cooked)), q);
}

void
test::test_pairs (vocabulary &builtins,
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

void
test::DwTest::SetUp ()
{
  auto core = dwgrep_vocabulary_core ();
  auto dw = dwgrep_vocabulary_dw ();

  builtins = std::make_unique <vocabulary> (*core, *dw);
}

void
test::ElfTest::SetUp ()
{
  auto core = dwgrep_vocabulary_core ();
  auto elf = dwgrep_vocabulary_elf ();

  builtins = std::make_unique <vocabulary> (*core, *elf);
}
