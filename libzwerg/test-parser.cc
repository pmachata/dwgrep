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

#include <string>
#include <iostream>
#include <memory>
#include <sstream>

#include "tree.hh"
#include "builtin-core-voc.hh"
#include "parser.hh"
#include "lexer.hh"

static unsigned tests = 0, failed = 0;

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
      t = parse_query (parse);
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
  test ("\"ab\ncd\"", "(FORMAT (STR<ab\ncd>))");
  test ("\"ab\\\ncd\"", "(FORMAT (STR<abcd>))");

  // This tests single-line comments that don't end in an EOL.
  test ("17 //blah blah", "(CONST<17>)");
  test ("17 #blah blah", "(CONST<17>)");

  ftestx ("\"unterminated", "terminated");
  ftestx ("\x01", "0x01");

  test ("swap", "(READ<swap>)");
  test ("dup", "(READ<dup>)");
  test ("drop", "(READ<drop>)");

  test ("if () then () else ()",
	"(IFELSE (SCOPE (NOP)) (SCOPE (NOP)) (SCOPE (NOP)))");
  test ("if 1 then 2 else 3",
	"(IFELSE (SCOPE (CONST<1>)) (SCOPE (CONST<2>)) (SCOPE (CONST<3>)))");
  test ("if if 0 then 1 else 2 then 3 else 4",
	"(IFELSE (SCOPE (IFELSE (SCOPE (CONST<0>)) (SCOPE (CONST<1>))"
	" (SCOPE (CONST<2>)))) (SCOPE (CONST<3>)) (SCOPE (CONST<4>)))");
  test ("if 1 then if 0 then 2 else 3 else 4",
	"(IFELSE (SCOPE (CONST<1>)) (SCOPE (IFELSE (SCOPE (CONST<0>))"
	" (SCOPE (CONST<2>)) (SCOPE (CONST<3>)))) (SCOPE (CONST<4>)))");
  test ("if 1 then 2 else if 0 then 3 else 4",
	"(IFELSE (SCOPE (CONST<1>)) (SCOPE (CONST<2>))"
	" (SCOPE (IFELSE (SCOPE (CONST<0>)) (SCOPE (CONST<3>))"
	" (SCOPE (CONST<4>)))))");
  test ("if 1 then 2 "
	"else if 0 then 3 "
	"else if 6 then 4 "
	"else 5 ",
	"(IFELSE (SCOPE (CONST<1>)) (SCOPE (CONST<2>))"
	" (SCOPE (IFELSE (SCOPE (CONST<0>)) (SCOPE (CONST<3>))"
	" (SCOPE (IFELSE (SCOPE (CONST<6>)) (SCOPE (CONST<4>))"
	" (SCOPE (CONST<5>)))))))");

  test ("?eq", "(READ<?eq>)");
  test ("!eq", "(READ<!eq>)");
  test ("?ne", "(READ<?ne>)");
  test ("!ne", "(READ<!ne>)");
  test ("?lt", "(READ<?lt>)");
  test ("!lt", "(READ<!lt>)");
  test ("?gt", "(READ<?gt>)");
  test ("!gt", "(READ<!gt>)");
  test ("?le", "(READ<?le>)");
  test ("!le", "(READ<!le>)");
  test ("?ge", "(READ<?ge>)");
  test ("!ge", "(READ<!ge>)");
  test ("?find", "(READ<?find>)");
  test ("!find", "(READ<!find>)");
  test ("?starts", "(READ<?starts>)");
  test ("!starts", "(READ<!starts>)");
  test ("?ends", "(READ<?ends>)");
  test ("!ends", "(READ<!ends>)");

  test ("?match", "(READ<?match>)");
  test ("!match", "(READ<!match>)");

  test ("add", "(READ<add>)");
  test ("sub", "(READ<sub>)");
  test ("mul", "(READ<mul>)");
  test ("div", "(READ<div>)");
  test ("mod", "(READ<mod>)");
  test ("type", "(READ<type>)");
  test ("value", "(READ<value>)");
  test ("pos", "(READ<pos>)");
  test ("elem", "(READ<elem>)");

  test ("1 add: 2", "(CAT (CONST<1>) (CONST<2>) (READ<add>))");

  test ("()*", "(CLOSE_STAR (SCOPE (NOP)))");
  test ("()+", "(CLOSE_PLUS (SCOPE (NOP)))");
  test ("()******", "(CLOSE_STAR (SCOPE (NOP)))");
  test ("()++++++", "(CLOSE_PLUS (SCOPE (NOP)))");
  test ("()+*+*+*", "(CLOSE_STAR (SCOPE (NOP)))");
  test ("()*+*+*+", "(CLOSE_STAR (SCOPE (NOP)))");
  test ("swap*", "(CLOSE_STAR (SCOPE (READ<swap>)))");
  test ("swap+", "(CLOSE_PLUS (SCOPE (READ<swap>)))");
  test ("swap?", "(ALT (READ<swap>) (NOP))");

  test ("1 dup",
	"(CAT (CONST<1>) (READ<dup>))");
  test ("1 dup*",
	"(CAT (CONST<1>) (CLOSE_STAR (SCOPE (READ<dup>))))");
  test ("1* dup",
	"(CAT (CLOSE_STAR (SCOPE (CONST<1>))) (READ<dup>))");
  test ("1+ dup",
	"(CAT (CLOSE_PLUS (SCOPE (CONST<1>))) (READ<dup>))");

  test ("(elem 1)",
	"(CAT (READ<elem>) (CONST<1>))");
  test ("((elem 1))",
	"(CAT (READ<elem>) (CONST<1>))");
  test ("(elem (1))",
	"(CAT (READ<elem>) (CONST<1>))");
  test ("(dup) swap elem 1",
	"(CAT (READ<dup>) (READ<swap>) (READ<elem>) (CONST<1>))");
  test ("dup (swap) elem 1",
	"(CAT (READ<dup>) (READ<swap>) (READ<elem>) (CONST<1>))");
  test ("dup swap (elem) 1",
	"(CAT (READ<dup>) (READ<swap>) (READ<elem>) (CONST<1>))");
  test ("dup swap elem (1)",
	"(CAT (READ<dup>) (READ<swap>) (READ<elem>) (CONST<1>))");
  test ("dup (swap (elem (1)))",
	"(CAT (READ<dup>) (READ<swap>) (READ<elem>) (CONST<1>))");
  test ("((((dup) swap) elem) 1)",
	"(CAT (READ<dup>) (READ<swap>) (READ<elem>) (CONST<1>))");
  test ("((((dup) swap)) (elem 1))",
	"(CAT (READ<dup>) (READ<swap>) (READ<elem>) (CONST<1>))");

  test ("dup, over",
	"(ALT (SCOPE (READ<dup>)) (SCOPE (READ<over>)))");
  test ("dup, over, rot",
	"(ALT (SCOPE (READ<dup>)) (SCOPE (READ<over>))"
	" (SCOPE (READ<rot>)))");
  test ("swap,",
	"(ALT (SCOPE (READ<swap>)) (SCOPE (NOP)))");
  test ("swap dup, over",
	"(ALT (SCOPE (CAT (READ<swap>) (READ<dup>)))"
	" (SCOPE (READ<over>)))");
  test ("swap dup, over elem, 1 dup",
	"(ALT (SCOPE (CAT (READ<swap>) (READ<dup>))) "
	"(SCOPE (CAT (READ<over>) (READ<elem>))) "
	"(SCOPE (CAT (CONST<1>) (READ<dup>))))");
  test ("(swap dup, (over elem, (1 dup)))",
	"(ALT (SCOPE (CAT (READ<swap>) (READ<dup>)))"
	" (SCOPE (CAT (READ<over>) (READ<elem>))) "
	"(SCOPE (CAT (CONST<1>) (READ<dup>))))");
  test ("elem, dup*",
	"(ALT (SCOPE (READ<elem>)) (SCOPE (CLOSE_STAR (SCOPE (READ<dup>)))))");

  test ("[]",
	"(EMPTY_LIST)");
  test ("[()]",
	"(SCOPE (CAPTURE (SCOPE (NOP))))");
  test ("[elem]",
	"(SCOPE (CAPTURE (SCOPE (READ<elem>))))");
  test ("[,]",
	"(SCOPE (CAPTURE (SCOPE (ALT (SCOPE (NOP)) (SCOPE (NOP))))))");
  test ("[,,]",
	"(SCOPE (CAPTURE (SCOPE (ALT (SCOPE (NOP))"
	" (SCOPE (NOP)) (SCOPE (NOP))))))");
  test ("[1,,2,]",
	"(SCOPE (CAPTURE (SCOPE (ALT (SCOPE (CONST<1>))"
	" (SCOPE (NOP)) (SCOPE (CONST<2>)) (SCOPE (NOP))))))");

  // Formatting strings.
  test ("\"%%\"", "(FORMAT (STR<%>))");
  test ("\"a%( \")%( [elem] %)(\" %)b\"",
	"(FORMAT (STR<a>) (FORMAT (STR<)>)"
	" (SCOPE (CAPTURE (SCOPE (READ<elem>))))"
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

  ftest (",", "(ALT (SCOPE (NOP)) (SCOPE (NOP)))");
  ftest ("elem dup (swap,)",
	 "(CAT (READ<elem>) (READ<dup>)"
	 " (ALT (SCOPE (READ<swap>)) (SCOPE (NOP))))");
  ftest ("elem dup (,swap)",
	 "(CAT (READ<elem>) (READ<dup>)"
	 " (ALT (SCOPE (NOP)) (SCOPE (READ<swap>))))");
  ftest ("elem (drop,drop)",
	 "(CAT (READ<elem>)"
	 " (ALT (SCOPE (READ<drop>)) (SCOPE (READ<drop>))))");
  ftest ("elem (,drop 1)",
	 "(CAT (READ<elem>)"
	 " (ALT (SCOPE (NOP)) (SCOPE (CAT (READ<drop>) (CONST<1>)))))");
  ftest ("elem (drop 1,)",
	 "(CAT (READ<elem>)"
	 " (ALT (SCOPE (CAT (READ<drop>) (CONST<1>))) (SCOPE (NOP))))");
  ftest ("elem drop \"foo\"",
	 "(CAT (READ<elem>)"
	 " (READ<drop>) (FORMAT (STR<foo>)))");

  ftest ("elem \"%( dup swap %): %( elem %)\"",
	 "(CAT (READ<elem>)"
	 " (FORMAT (STR<>)"
	 " (CAT (READ<dup>) (READ<swap>)) (STR<: >)"
	 " (READ<elem>) (STR<>)))",
	 true);

  test ("((1, 2), (3, 4))",
	"(ALT (SCOPE (CONST<1>)) (SCOPE (CONST<2>))"
	" (SCOPE (CONST<3>)) (SCOPE (CONST<4>)))");

  test ("?0", "(F_BUILTIN<pred_pos>)");
  test ("!0", "(F_BUILTIN<pred_pos>)");
  test ("?0x0a", "(F_BUILTIN<pred_pos>)");
  test ("!0o77", "(F_BUILTIN<pred_pos>)");
  ftestx ("!-1", "Invalid");

  std::cerr << tests << " tests total, " << failed << " failures." << std::endl;
  assert (failed == 0);
}

int
main (int argc, char *argv[])
{
  if (argc > 1)
    {
      tree t = parse_query (argv[1]);
      std::cerr << t << std::endl;
    }
  else
    do_tests ();

  return 0;
}
