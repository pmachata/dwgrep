/*
   Copyright (C) 2017 Petr Machata
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

#ifndef TEST_ZW_AUX_H
#define TEST_ZW_AUX_H

#include <string>

#include "stack.hh"
#include "value.hh"
#include "builtin.hh"

namespace test
{
  std::unique_ptr <stack> stack_with_value (std::unique_ptr <value> v);

  std::vector <std::unique_ptr <stack>>
  run_query (vocabulary &voc, std::unique_ptr <stack> stk, std::string q);

  std::string get_parse_error (vocabulary &voc, std::string q);
}

#endif /* TEST_ZW_AUX_H */
