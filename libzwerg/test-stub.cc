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
#include <iostream>

std::string g_test_case_directory;

struct MinimalistPrinter
  : public testing::EmptyTestEventListener
{
  void
  OnTestPartResult (const testing::TestPartResult& test_part_result) override
  {
    if (test_part_result.failed ())
      std::cout << "FAIL\t" << test_part_result.file_name ()
		<< ":" << test_part_result.line_number() << std::endl
		<< test_part_result.summary () << std::endl;
  }
};

int
main (int argc, char **argv)
{
  testing::InitGoogleTest (&argc, argv);

  for (int i = 1; i < argc; ++i)
    {
      std::string arg = argv[i];
      size_t n = (sizeof "--test-case-directory=") - 1;
      if (arg.substr (0, n) == "--test-case-directory=")
	g_test_case_directory = arg.substr (n);
    }

  assert (g_test_case_directory != "");

  testing::TestEventListeners& listeners =
      testing::UnitTest::GetInstance()->listeners();

  delete listeners.Release (listeners.default_result_printer ());
  listeners.Append (new MinimalistPrinter);

  return RUN_ALL_TESTS();
}
