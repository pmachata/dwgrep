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

#include <string>
#include <iostream>
#include <memory>
#include <sstream>

#include "tree.hh"
#include "parser.hh"
#include "lexer.hh"
#include "init.hh"

static unsigned tests = 0, failed = 0;

std::unique_ptr <vocabulary> builtins;

void
fail (std::string parse)
{
  std::cerr << "can't parse: «" << parse << "»" << std::endl;
  ++failed;
}

void
test (std::string parse, std::string expect,
      bool full, std::string expect_exc, bool optimize)
{
  ++tests;
  tree t;
  try
    {
      t = parse_query (*builtins, parse);
      if (full && optimize)
	  t.simplify ();
    }
  catch (std::runtime_error const &e)
    {
      if (expect_exc == ""
	  || std::string (e.what ()).find (expect_exc) == std::string::npos)
	{
	  std::cerr << "wrong exception thrown" << std::endl;
	  std::cerr << "std::runtime_error: " << e.what () << std::endl;
	  fail (parse);
	}
      return;
    }
  catch (...)
    {
      if (expect_exc != "")
	std::cerr << "wrong exception thrown" << std::endl;
      else
	std::cerr << "some exception thrown" << std::endl;
      fail (parse);
      return;
    }

  std::ostringstream ss;
  ss << t;
  if (ss.str () != expect || expect_exc != "")
    {
      std::cerr << "bad parse: «" << parse << "»" << std::endl;
      std::cerr << "   result: «" << ss.str () << "»" << std::endl;
      if (expect_exc != "")
	std::cerr << "   expect: exception «" << expect_exc << "»" << std::endl;
      else
	std::cerr << "   expect: «" << expect << "»" << std::endl;
      ++failed;
    }
}

void
test (std::string parse, std::string expect)
{
  return test (parse, expect, false, "", false);
}

void
ftest (std::string parse, std::string expect, bool optimize = false)
{
  return test (parse, expect, true, "", optimize);
}

void
ftestx (std::string parse, std::string expect_exc, bool optimize = false)
{
  return test (parse, "", true, expect_exc, optimize);
}

void
do_tests ()
{
  test ("17", "(CONST<17>)");
  test ("0x17", "(CONST<0x17>)");
  test ("017", "(CONST<017>)");

  test ("\"string\"", "(FORMAT (STR<string>))");

  test ("swap", "(F_BUILTIN<swap>)");
  test ("dup", "(F_BUILTIN<dup>)");
  test ("drop", "(F_BUILTIN<drop>)");

  test ("if () then () else ()",
	"(IFELSE (NOP) (NOP) (NOP))");
  test ("if 1 then 2 else 3",
	"(IFELSE (CONST<1>) (CONST<2>) (CONST<3>))");
  test ("if if 0 then 1 else 2 then 3 else 4",
	"(IFELSE (IFELSE (CONST<0>) (CONST<1>) (CONST<2>))"
	" (CONST<3>) (CONST<4>))");
  test ("if 1 then if 0 then 2 else 3 else 4",
	"(IFELSE (CONST<1>) (IFELSE (CONST<0>) (CONST<2>) (CONST<3>))"
	" (CONST<4>))");
  test ("if 1 then 2 else if 0 then 3 else 4",
	"(IFELSE (CONST<1>) (CONST<2>)"
	" (IFELSE (CONST<0>) (CONST<3>) (CONST<4>)))");
  test ("if 1 then 2 "
	"else if 0 then 3 "
	"else if 6 then 4 "
	"else 5 ",
	"(IFELSE (CONST<1>) (CONST<2>)"
	" (IFELSE (CONST<0>) (CONST<3>)"
	" (IFELSE (CONST<6>) (CONST<4>) (CONST<5>))))");

  test ("?eq", "(F_BUILTIN<?eq>)");
  test ("!eq", "(F_BUILTIN<!eq>)");
  test ("?ne", "(F_BUILTIN<!eq>)");
  test ("!ne", "(F_BUILTIN<?eq>)");
  test ("?lt", "(F_BUILTIN<?lt>)");
  test ("!lt", "(F_BUILTIN<!lt>)");
  test ("?gt", "(F_BUILTIN<?gt>)");
  test ("!gt", "(F_BUILTIN<!gt>)");
  test ("?le", "(F_BUILTIN<!gt>)");
  test ("!le", "(F_BUILTIN<?gt>)");
  test ("?ge", "(F_BUILTIN<!lt>)");
  test ("!ge", "(F_BUILTIN<?lt>)");
  test ("?find", "(F_BUILTIN<?find>)");
  test ("!find", "(F_BUILTIN<!find>)");
  test ("?starts", "(F_BUILTIN<?starts>)");
  test ("!starts", "(F_BUILTIN<!starts>)");
  test ("?ends", "(F_BUILTIN<?ends>)");
  test ("!ends", "(F_BUILTIN<!ends>)");

  test ("?match", "(F_BUILTIN<?match>)");
  test ("!match", "(F_BUILTIN<!match>)");

  test ("add", "(F_BUILTIN<add>)");
  test ("sub", "(F_BUILTIN<sub>)");
  test ("mul", "(F_BUILTIN<mul>)");
  test ("div", "(F_BUILTIN<div>)");
  test ("mod", "(F_BUILTIN<mod>)");
  test ("type", "(F_BUILTIN<type>)");
  test ("value", "(F_BUILTIN<value>)");
  test ("pos", "(F_BUILTIN<pos>)");
  test ("elem", "(F_BUILTIN<elem>)");

  test ("1 add: 2", "(CAT (CONST<1>) (CONST<2>) (F_BUILTIN<add>))");

  test ("()*", "(CLOSE_STAR (NOP))");
  test ("()+", "(CAT (NOP) (CLOSE_STAR (NOP)))");
  test ("swap*", "(CLOSE_STAR (F_BUILTIN<swap>))");
  test ("swap+", "(CAT (F_BUILTIN<swap>) (CLOSE_STAR (F_BUILTIN<swap>)))");
  test ("swap?", "(ALT (F_BUILTIN<swap>) (NOP))");

  test ("1 dup",
	"(CAT (CONST<1>) (F_BUILTIN<dup>))");
  test ("1 dup*",
	"(CAT (CONST<1>) (CLOSE_STAR (F_BUILTIN<dup>)))");
  test ("1* dup",
	"(CAT (CLOSE_STAR (CONST<1>)) (F_BUILTIN<dup>))");
  test ("1+ dup",
	"(CAT (CONST<1>) (CLOSE_STAR (CONST<1>)) (F_BUILTIN<dup>))");

  test ("(elem 1)",
	"(CAT (F_BUILTIN<elem>) (CONST<1>))");
  test ("((elem 1))",
	"(CAT (F_BUILTIN<elem>) (CONST<1>))");
  test ("(elem (1))",
	"(CAT (F_BUILTIN<elem>) (CONST<1>))");
  test ("(dup) swap elem 1",
	"(CAT (F_BUILTIN<dup>) (F_BUILTIN<swap>) (F_BUILTIN<elem>) (CONST<1>))");
  test ("dup (swap) elem 1",
	"(CAT (F_BUILTIN<dup>) (F_BUILTIN<swap>) (F_BUILTIN<elem>) (CONST<1>))");
  test ("dup swap (elem) 1",
	"(CAT (F_BUILTIN<dup>) (F_BUILTIN<swap>) (F_BUILTIN<elem>) (CONST<1>))");
  test ("dup swap elem (1)",
	"(CAT (F_BUILTIN<dup>) (F_BUILTIN<swap>) (F_BUILTIN<elem>) (CONST<1>))");
  test ("dup (swap (elem (1)))",
	"(CAT (F_BUILTIN<dup>) (F_BUILTIN<swap>) (F_BUILTIN<elem>) (CONST<1>))");
  test ("((((dup) swap) elem) 1)",
	"(CAT (F_BUILTIN<dup>) (F_BUILTIN<swap>) (F_BUILTIN<elem>) (CONST<1>))");
  test ("((((dup) swap)) (elem 1))",
	"(CAT (F_BUILTIN<dup>) (F_BUILTIN<swap>) (F_BUILTIN<elem>) (CONST<1>))");

  test ("dup, over",
	"(ALT (F_BUILTIN<dup>) (F_BUILTIN<over>))");
  test ("dup, over, rot",
	"(ALT (F_BUILTIN<dup>) (F_BUILTIN<over>) (F_BUILTIN<rot>))");
  test ("swap,",
	"(ALT (F_BUILTIN<swap>) (NOP))");
  test ("swap dup, over",
	"(ALT (CAT (F_BUILTIN<swap>) (F_BUILTIN<dup>)) (F_BUILTIN<over>))");
  test ("swap dup, over elem, 1 dup",
	"(ALT (CAT (F_BUILTIN<swap>) (F_BUILTIN<dup>)) "
	"(CAT (F_BUILTIN<over>) (F_BUILTIN<elem>)) "
	"(CAT (CONST<1>) (F_BUILTIN<dup>)))");
  test ("(swap dup, (over elem, (1 dup)))",
	"(ALT (CAT (F_BUILTIN<swap>) (F_BUILTIN<dup>))"
	" (CAT (F_BUILTIN<over>) (F_BUILTIN<elem>)) "
	"(CAT (CONST<1>) (F_BUILTIN<dup>)))");
  test ("elem, dup*",
	"(ALT (F_BUILTIN<elem>) (CLOSE_STAR (F_BUILTIN<dup>)))");

  test ("[]",
	"(EMPTY_LIST)");
  test ("[()]",
	"(CAPTURE (NOP))");
  test ("[elem]",
	"(CAPTURE (F_BUILTIN<elem>))");
  test ("[,]",
	"(CAPTURE (ALT (NOP) (NOP)))");
  test ("[,,]",
	"(CAPTURE (ALT (NOP) (NOP) (NOP)))");
  test ("[1,,2,]",
	"(CAPTURE (ALT (CONST<1>) (NOP) (CONST<2>) (NOP)))");

  // Formatting strings.
  test ("\"%%\"", "(FORMAT (STR<%>))");
  test ("\"a%( \")%( [elem] %)(\" %)b\"",
	"(FORMAT (STR<a>) (FORMAT (STR<)>)"
	" (CAPTURE (F_BUILTIN<elem>))"
	" (STR<(>)) (STR<b>))");
  test ("\"abc%sdef\"",
	"(FORMAT (STR<abc>) (NOP) (STR<def>))");

  test ("\"r\\aw\"", "(FORMAT (STR<r\aw>))");
  test ("r\"r\\aw\"", "(FORMAT (STR<r\\aw>))");

  // String continuation.
  test ("\"con\\t\"\\   \"i\\nued\"",
	"(FORMAT (STR<con\ti\nued>))");
  test ("r\"con\\t\"\\   \"inued\"",
	"(FORMAT (STR<con\\tinued>))");
  test ("r\"con\\t\"\\   \"i\\nued\"",
	"(FORMAT (STR<con\\ti\nued>))");
  test ("\"con\\t\"\\   r\"i\\nued\"",
	"(FORMAT (STR<con\ti\\nued>))");
  test ("r\"con\\t\"\\   r\"i\\nued\"",
	"(FORMAT (STR<con\\ti\\nued>))");
  test ("\"co\\n\"\\ \"\\ti\"\\   \"\\nued\"",
	"(FORMAT (STR<co\n\ti\nued>))");
  test ("r\"co\\n\"\\ \"\\ti\"\\   \"\\nued\"",
	"(FORMAT (STR<co\\n\ti\nued>))");
  test ("\"co\\n\"\\ r\"\\ti\"\\   \"\\nued\"",
	"(FORMAT (STR<co\n\\ti\nued>))");
  test ("\"co\\n\"\\ \"\\ti\"\\   r\"\\nued\"",
	"(FORMAT (STR<co\n\ti\\nued>))");

  ftest (",", "(ALT (NOP) (NOP))");
  ftest ("elem dup (swap,)",
	 "(CAT (F_BUILTIN<elem>) (F_BUILTIN<dup>)"
	 " (ALT (F_BUILTIN<swap>) (NOP)))");
  ftest ("elem dup (,swap)",
	 "(CAT (F_BUILTIN<elem>) (F_BUILTIN<dup>)"
	 " (ALT (NOP) (F_BUILTIN<swap>)))");
  ftest ("elem (drop,drop)",
	 "(CAT (F_BUILTIN<elem>)"
	 " (ALT (F_BUILTIN<drop>) (F_BUILTIN<drop>)))");
  ftest ("elem (,drop 1)",
	 "(CAT (F_BUILTIN<elem>)"
	 " (ALT (NOP) (CAT (F_BUILTIN<drop>) (CONST<1>))))");
  ftest ("elem (drop 1,)",
	 "(CAT (F_BUILTIN<elem>)"
	 " (ALT (CAT (F_BUILTIN<drop>) (CONST<1>)) (NOP)))");
  ftest ("elem drop \"foo\"",
	 "(CAT (F_BUILTIN<elem>)"
	 " (F_BUILTIN<drop>) (FORMAT (STR<foo>)))");

  ftest ("elem \"%( dup swap %): %( elem %)\"",
	 "(CAT (F_BUILTIN<elem>)"
	 " (FORMAT (STR<>)"
	 " (CAT (F_BUILTIN<dup>) (F_BUILTIN<swap>)) (STR<: >)"
	 " (F_BUILTIN<elem>) (STR<>)))",
	 true);

  test ("((1, 2), (3, 4))",
	"(ALT (CONST<1>) (CONST<2>) (CONST<3>) (CONST<4>))");

  std::cerr << tests << " tests total, " << failed << " failures." << std::endl;
  assert (failed == 0);
}

int
main (int argc, char *argv[])
{
  builtins = dwgrep_vocabulary_core ();

  if (argc > 1)
    {
      tree t = parse_query (*builtins, argv[1]);
      std::cerr << t << std::endl;
    }
  else
    do_tests ();

  return 0;
}
