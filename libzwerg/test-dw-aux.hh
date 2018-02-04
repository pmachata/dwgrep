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

#ifndef TEST_DW_AUX_H
#define TEST_DW_AUX_H

#include <gtest/gtest.h>
#include "value-dw.hh"

struct vocabulary;

namespace test
{
  std::unique_ptr <value_dwarf> dw (std::string fn, doneness d);
  std::unique_ptr <value_dwarf> rdw (std::string fn);

  // A query Q on a stack with sole value, which is a Dwarf opened from file
  // named FN.
  std::vector <std::unique_ptr <stack>> run_dwquery (vocabulary &voc,
						     std::string fn,
						     std::string q);

  void test_pairs (vocabulary &builtins,
		   std::string fn, std::string query,
		   std::vector <std::pair <constant, std::string>> results);

  struct DwTest
    : public testing::Test
  {
    std::unique_ptr <vocabulary> builtins;
    void SetUp () override final;
  };
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

#endif /* TEST_DW_AUX_H */
