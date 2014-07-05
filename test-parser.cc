#include <string>
#include <iostream>
#include <memory>
#include <sstream>

#include "tree.hh"
#include "parser.hh"
#include "lexer.hh"
#include "known-dwarf.h"

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
#define ONE_KNOWN_DW_TAG(NAME, CODE) test (#CODE, "(CONST<" #CODE ">)");
  ALL_KNOWN_DW_TAG;
#undef ONE_KNOWN_DW_TAG

#define ONE_KNOWN_DW_AT(NAME, CODE) test (#CODE, "(CONST<" #CODE ">)");
  ALL_KNOWN_DW_AT;
#undef ONE_KNOWN_DW_AT

#define ONE_KNOWN_DW_FORM_DESC(NAME, CODE, DESC) ONE_KNOWN_DW_FORM (NAME, CODE)
#define ONE_KNOWN_DW_FORM(NAME, CODE) test (#CODE, "(CONST<" #CODE ">)");
  ALL_KNOWN_DW_FORM;
#undef ONE_KNOWN_DW_FORM
#undef ONE_KNOWN_DW_FORM_DESC

#define ONE_KNOWN_DW_LANG_DESC(NAME, CODE, DESC)	\
	test (#CODE, "(CONST<" #CODE ">)");
  ALL_KNOWN_DW_LANG;
#undef ONE_KNOWN_DW_LANG_DESC

#define ONE_KNOWN_DW_INL(NAME, CODE) test (#CODE, "(CONST<" #CODE ">)");
  ALL_KNOWN_DW_INL;
#undef ONE_KNOWN_DW_INL

#define ONE_KNOWN_DW_ATE(NAME, CODE) test (#CODE, "(CONST<" #CODE ">)");
  ALL_KNOWN_DW_ATE;
#undef ONE_KNOWN_DW_ATE

#define ONE_KNOWN_DW_ACCESS(NAME, CODE) test (#CODE, "(CONST<" #CODE ">)");
  ALL_KNOWN_DW_ACCESS;
#undef ONE_KNOWN_DW_ACCESS

#define ONE_KNOWN_DW_VIS(NAME, CODE) test (#CODE, "(CONST<" #CODE ">)");
  ALL_KNOWN_DW_VIS;
#undef ONE_KNOWN_DW_VIS

#define ONE_KNOWN_DW_VIRTUALITY(NAME, CODE) test (#CODE, "(CONST<" #CODE ">)");
  ALL_KNOWN_DW_VIRTUALITY;
#undef ONE_KNOWN_DW_VIRTUALITY

#define ONE_KNOWN_DW_ID(NAME, CODE) test (#CODE, "(CONST<" #CODE ">)");
  ALL_KNOWN_DW_ID;
#undef ONE_KNOWN_DW_ID

#define ONE_KNOWN_DW_CC(NAME, CODE) test (#CODE, "(CONST<" #CODE ">)");
  ALL_KNOWN_DW_CC;
#undef ONE_KNOWN_DW_CC

#define ONE_KNOWN_DW_ORD(NAME, CODE) test (#CODE, "(CONST<" #CODE ">)");
  ALL_KNOWN_DW_ORD;
#undef ONE_KNOWN_DW_ORD

#define ONE_KNOWN_DW_DSC(NAME, CODE) test (#CODE, "(CONST<" #CODE ">)");
  ALL_KNOWN_DW_DSC;
#undef ONE_KNOWN_DW_DSC

#define ONE_KNOWN_DW_DS(NAME, CODE) test (#CODE, "(CONST<" #CODE ">)");
  ALL_KNOWN_DW_DS;
#undef ONE_KNOWN_DW_DS

#define ONE_KNOWN_DW_OP_DESC(NAME, CODE, DESC) ONE_KNOWN_DW_OP (NAME, CODE)
#define ONE_KNOWN_DW_OP(NAME, CODE) test (#CODE, "(CONST<" #CODE ">)");
  ALL_KNOWN_DW_OP;
#undef ONE_KNOWN_DW_OP
#undef ONE_KNOWN_DW_OP_DESC

  test ("DW_ADDR_none", "(CONST<DW_ADDR_none>)");

#define ONE_KNOWN_DW_END(NAME, CODE) test (#CODE, "(CONST<" #CODE ">)");
  ALL_KNOWN_DW_END;
#undef ONE_KNOWN_DW_END

  test ("17", "(CONST<17>)");
  test ("0x17", "(CONST<0x17>)");
  test ("017", "(CONST<017>)");

  test ("\"string\"", "(FORMAT (STR<string>))");

  test ("swap", "(SHF_SWAP)");
  test ("dup", "(SHF_DUP)");
  test ("over", "(SHF_OVER)");
  test ("rot", "(SHF_ROT)");
  test ("drop", "(SHF_DROP)");

  test ("if () then () else ()",
	"(IFELSE (NOP) (NOP) (NOP))");
  test ("if child then next else prev",
	"(IFELSE (F_CHILD) (F_NEXT) (F_PREV))");
  test ("if if child then 1 else 2 then 3 else 4",
	"(IFELSE (IFELSE (F_CHILD) (CONST<1>) (CONST<2>))"
	" (CONST<3>) (CONST<4>))");
  test ("if 1 then if child then 2 else 3 else 4",
	"(IFELSE (CONST<1>) (IFELSE (F_CHILD) (CONST<2>) (CONST<3>))"
	" (CONST<4>))");
  test ("if 1 then 2 else if child then 3 else 4",
	"(IFELSE (CONST<1>) (CONST<2>)"
	" (IFELSE (F_CHILD) (CONST<3>) (CONST<4>)))");
  test ("if 1 then 2 "
	"else if child then 3 "
	"else if next then 4 "
	"else 5 ",
	"(IFELSE (CONST<1>) (CONST<2>)"
	" (IFELSE (F_CHILD) (CONST<3>)"
	" (IFELSE (F_NEXT) (CONST<4>) (CONST<5>))))");

  test ("?eq", "(ASSERT (PRED_EQ))");
  test ("!eq", "(ASSERT (PRED_NOT (PRED_EQ)))");
  test ("?ne", "(ASSERT (PRED_NE))");
  test ("!ne", "(ASSERT (PRED_NOT (PRED_NE)))");
  test ("?lt", "(ASSERT (PRED_LT))");
  test ("!lt", "(ASSERT (PRED_NOT (PRED_LT)))");
  test ("?gt", "(ASSERT (PRED_GT))");
  test ("!gt", "(ASSERT (PRED_NOT (PRED_GT)))");
  test ("?le", "(ASSERT (PRED_LE))");
  test ("!le", "(ASSERT (PRED_NOT (PRED_LE)))");
  test ("?ge", "(ASSERT (PRED_GE))");
  test ("!ge", "(ASSERT (PRED_NOT (PRED_GE)))");

  test ("?match", "(ASSERT (PRED_MATCH))");
  test ("!match", "(ASSERT (PRED_NOT (PRED_MATCH)))");
  test ("?find", "(ASSERT (PRED_FIND))");
  test ("!find", "(ASSERT (PRED_NOT (PRED_FIND)))");

  test ("?root", "(ASSERT (PRED_ROOT))");
  test ("!root", "(ASSERT (PRED_NOT (PRED_ROOT)))");

  test ("add", "(F_ADD)");
  test ("sub", "(F_SUB)");
  test ("mul", "(F_MUL)");
  test ("div", "(F_DIV)");
  test ("mod", "(F_MOD)");
  test ("parent", "(F_PARENT)");
  test ("child", "(F_CHILD)");
  test ("attribute", "(F_ATTRIBUTE)");
  test ("prev", "(F_PREV)");
  test ("next", "(F_NEXT)");
  test ("type", "(F_TYPE)");
  test ("offset", "(F_OFFSET)");
  test ("name", "(F_NAME)");
  test ("tag", "(F_TAG)");
  test ("form", "(F_FORM)");
  test ("value", "(F_VALUE)");
  test ("pos", "(F_POS)");
  test ("each", "(F_EACH)");
  test ("universe", "(SEL_UNIVERSE)");
  test ("section", "(SEL_SECTION)");
  test ("unit", "(SEL_UNIT)");

#define ONE_KNOWN_DW_AT(NAME, CODE)					\
  test ("@"#NAME, "(CAT (F_ATTR_NAMED<" #CODE ">) (F_VALUE))");		\
  test ("?@"#NAME, "(ASSERT (PRED_AT<" #CODE ">))");			\
  test ("!@"#NAME, "(ASSERT (PRED_NOT (PRED_AT<" #CODE ">)))");

  ALL_KNOWN_DW_AT;
#undef ONE_KNOWN_DW_AT

#define ONE_KNOWN_DW_TAG(NAME, CODE)				\
  test ("?"#NAME, "(ASSERT (PRED_TAG<" #CODE ">))");		\
  test ("!"#NAME, "(ASSERT (PRED_NOT (PRED_TAG<" #CODE ">)))");

  ALL_KNOWN_DW_TAG;
#undef ONE_KNOWN_DW_TAG

  test ("child*", "(CLOSE_STAR (F_CHILD))");
  test ("child+", "(CAT (F_CHILD) (CLOSE_STAR (F_CHILD)))");
  test ("child?", "(ALT (F_CHILD) (NOP))");
  test ("swap*", "(CLOSE_STAR (SHF_SWAP))");
  test ("swap+", "(CAT (SHF_SWAP) (CLOSE_STAR (SHF_SWAP)))");
  test ("swap?", "(ALT (SHF_SWAP) (NOP))");

  test ("child next",
	"(CAT (F_CHILD) (F_NEXT))");
  test ("child next*",
	"(CAT (F_CHILD) (CLOSE_STAR (F_NEXT)))");
  test ("child* next",
	"(CAT (CLOSE_STAR (F_CHILD)) (F_NEXT))");
  test ("child+ next",
	"(CAT (F_CHILD) (CLOSE_STAR (F_CHILD)) (F_NEXT))");

  test ("dup swap child",
	"(CAT (SHF_DUP) (SHF_SWAP) (F_CHILD))");
  test ("dup swap child next",
	"(CAT (SHF_DUP) (SHF_SWAP) (F_CHILD) (F_NEXT))");

  test ("2/child",
	"(TRANSFORM (CONST<2>) (F_CHILD))");
  test ("2/child next",
	"(CAT (TRANSFORM (CONST<2>) (F_CHILD)) (F_NEXT))");
  test ("2/(child next)",
	"(TRANSFORM (CONST<2>) (CAT (F_CHILD) (F_NEXT)))");
  test ("2/child 2/next",
	"(CAT (TRANSFORM (CONST<2>) (F_CHILD))"
	" (TRANSFORM (CONST<2>) (F_NEXT)))");

  test ("(child next)",
	"(CAT (F_CHILD) (F_NEXT))");
  test ("((child next))",
	"(CAT (F_CHILD) (F_NEXT))");
  test ("(child (next))",
	"(CAT (F_CHILD) (F_NEXT))");
  test ("(dup) swap child next",
	"(CAT (SHF_DUP) (SHF_SWAP) (F_CHILD) (F_NEXT))");
  test ("dup (swap) child next",
	"(CAT (SHF_DUP) (SHF_SWAP) (F_CHILD) (F_NEXT))");
  test ("dup swap (child) next",
	"(CAT (SHF_DUP) (SHF_SWAP) (F_CHILD) (F_NEXT))");
  test ("dup swap child (next)",
	"(CAT (SHF_DUP) (SHF_SWAP) (F_CHILD) (F_NEXT))");
  test ("dup (swap (child (next)))",
	"(CAT (SHF_DUP) (SHF_SWAP) (F_CHILD) (F_NEXT))");
  test ("((((dup) swap) child) next)",
	"(CAT (SHF_DUP) (SHF_SWAP) (F_CHILD) (F_NEXT))");
  test ("((((dup) swap)) (child next))",
	"(CAT (SHF_DUP) (SHF_SWAP) (F_CHILD) (F_NEXT))");

  test ("dup, over",
	"(ALT (SHF_DUP) (SHF_OVER))");
  test ("dup, over, child",
	"(ALT (SHF_DUP) (SHF_OVER) (F_CHILD))");
  test ("swap,",
	"(ALT (SHF_SWAP) (NOP))");
  test ("swap dup, over",
	"(ALT (CAT (SHF_SWAP) (SHF_DUP)) (SHF_OVER))");
  test ("swap dup, over next, parent dup",
	"(ALT (CAT (SHF_SWAP) (SHF_DUP)) (CAT (SHF_OVER) (F_NEXT)) "
	"(CAT (F_PARENT) (SHF_DUP)))");
  test ("(swap dup, (over next, (parent dup)))",
	"(ALT (CAT (SHF_SWAP) (SHF_DUP)) (CAT (SHF_OVER) (F_NEXT)) "
	"(CAT (F_PARENT) (SHF_DUP)))");
  test ("2/next, 2/prev",
	"(ALT (TRANSFORM (CONST<2>) (F_NEXT))"
	" (TRANSFORM (CONST<2>) (F_PREV)))");
  test ("next, prev*",
	"(ALT (F_NEXT) (CLOSE_STAR (F_PREV)))");

  test ("[]",
	"(EMPTY_LIST)");
  test ("[()]",
	"(CAPTURE (NOP))");
  test ("[child]",
	"(CAPTURE (F_CHILD))");
  test ("[,]",
	"(CAPTURE (ALT (NOP) (NOP)))");
  test ("[,,]",
	"(CAPTURE (ALT (NOP) (NOP) (NOP)))");
  test ("[1,,2,]",
	"(CAPTURE (ALT (CONST<1>) (NOP) (CONST<2>) (NOP)))");

  // Formatting strings.
  test ("\"%%\"", "(FORMAT (STR<%>))");
  test ("\"a%( \")%( [@name] %)(\" %)b\"",
	"(FORMAT (STR<a>) (FORMAT (STR<)>)"
	" (CAPTURE (CAT (F_ATTR_NAMED<DW_AT_name>) (F_VALUE)))"
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

  ftest ("winfo ?root",
	 "(CAT (SEL_WINFO) (ASSERT (PRED_ROOT)))");

  ftest ("winfo ?compile_unit !root",
	 "(CAT (SEL_WINFO)"
	 " (ASSERT (PRED_TAG<DW_TAG_compile_unit>))"
	 " (ASSERT (PRED_NOT (PRED_ROOT))))");

  ftest (",", "(ALT (NOP) (NOP))");
  ftest ("winfo dup (swap,)",
	 "(CAT (SEL_WINFO) (SHF_DUP)"
	 " (ALT (SHF_SWAP) (NOP)))");
  ftest ("winfo dup (,swap)",
	 "(CAT (SEL_WINFO) (SHF_DUP)"
	 " (ALT (NOP) (SHF_SWAP)))");
  ftest ("winfo (drop,drop)",
	 "(CAT (SEL_WINFO)"
	 " (ALT (SHF_DROP) (SHF_DROP)))");
  ftest ("winfo (,drop 1)",
	 "(CAT (SEL_WINFO)"
	 " (ALT (NOP) (CAT (SHF_DROP) (CONST<1>))))");
  ftest ("winfo (drop 1,)",
	 "(CAT (SEL_WINFO)"
	 " (ALT (CAT (SHF_DROP) (CONST<1>)) (NOP)))");
  ftest ("winfo drop \"foo\"",
	 "(CAT (SEL_WINFO)"
	 " (SHF_DROP) (FORMAT (STR<foo>)))");

  ftest ("winfo \"%( dup offset %): %( @name %)\"",
	 "(CAT (SEL_WINFO)"
	 " (FORMAT (STR<>)"
	 " (CAT (SHF_DUP) (F_OFFSET)) (STR<: >)"
	 " (CAT (F_ATTR_NAMED<DW_AT_name>)"
	 " (F_VALUE)) (STR<>)))",
	 true);

  test ("((1, 2), (3, 4))",
	"(ALT (CONST<1>) (CONST<2>) (CONST<3>) (CONST<4>))");

  ftest ("winfo child?",
	 "(CAT (SEL_WINFO) (ALT (F_CHILD) (NOP)))",
	 false);

  ftest ("winfo child+",
	 "(CAT (SEL_WINFO) (F_CHILD) (CLOSE_STAR (F_CHILD)))",
	 false);

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
