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

#include "test-stub.hh"
#include "test-zw-aux.hh"
#include "op.hh"
#include "parser.hh"
#include "scon.hh"
#include "value-cst.hh"

#include "std-memory.hh"

std::unique_ptr <stack>
test::stack_with_value (std::unique_ptr <value> v)
{
  auto stk = std::make_unique <stack> ();
  stk->push (std::move (v));
  return stk;
}

std::vector <std::unique_ptr <stack>>
test::run_query (vocabulary &voc,
	   std::unique_ptr <stack> stk, std::string q)
{
  layout l;
  auto origin = std::make_shared <op_origin> (l);
  auto op = parse_query (q).build_exec (l, origin, voc);

  scon sc {l};
  scon_guard sg {sc, *op};
  origin->set_next (sc, std::move (stk));

  std::vector <std::unique_ptr <stack>> yielded;
  while (auto r = op->next (sc))
    yielded.push_back (std::move (r));

  return yielded;
}

std::string
test::get_parse_error (vocabulary &voc, std::string q)
{
  layout l;
  auto origin = std::make_shared <op_origin> (l);
  try
    {
      parse_query (q).build_exec (l, origin, voc);
    }
  catch (std::runtime_error &err)
    {
      return err.what ();
    }
  catch (...)
    {
      return "???";
    }

  return "";
}

std::string
test::test_file_name (std::string name)
{
  return test_case_directory () + "/" + name;
}

void
test::test_builtin_constant (vocabulary &builtins, char const *name)
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
