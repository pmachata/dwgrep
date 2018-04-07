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
#include "std-memory.hh"

#include "test-zw-aux.hh"
#include "value.hh"

using namespace test;

TEST_F (CoreTest, test_value)
{
  typedef std::vector <std::tuple <size_t, std::string>> vec;
  for (auto const &entry: vec {
	    {1, "1 type 100 add \"%s\""},
	})
    {
      size_t nresults = std::get <0> (entry);
      auto const &q = std::get <1> (entry);
      auto stk = std::make_unique <stack> ();

      auto yielded = run_query (*builtins, std::move (stk), q);
      ASSERT_EQ (nresults, yielded.size ())
	<< "In '" << q << "'";
    }
}
