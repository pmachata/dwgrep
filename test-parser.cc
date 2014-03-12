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
test (std::string parse, std::string expect)
{
  yyscan_t sc;
  if (yylex_init (&sc) != 0)
    throw std::runtime_error ("can't init lexer");

  ++tests;
  yy_scan_string (parse.c_str (), sc);
  std::unique_ptr <tree> t;
  int result = -1;
  try
    {
      result = yyparse (t, sc);
    }
  catch (std::runtime_error const &e)
    {
      std::cerr << "std::runtime_error: " << e.what () << std::endl;
    }
  catch (...)
    {
      std::cerr << "some exception thrown" << std::endl;
    }

  if (result == 0)
    {
      std::ostringstream ss;
      t->dump (ss);
      if (ss.str () != expect)
	{
	  std::cerr << "bad parse: «" << parse << "»" << std::endl;
	  std::cerr << "   result: «" << ss.str () << "»" << std::endl;
	  std::cerr << "   expect: «" << expect << "»" << std::endl;
	  ++failed;
	}
      return;
    }
  else
    {
      std::cerr << "can't parse: «" << parse << "»" << std::endl;
      ++failed;
    }
}

int
main (int argc, char *argv[])
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

#define ONE_KNOWN_DW_OP_DESC(NAME, CODE, DESC) ONE_KNOWN_DW_OP (NAME, CODE)
#define ONE_KNOWN_DW_OP(NAME, CODE) test (#CODE, "(CONST<" #CODE ">)");
  ALL_KNOWN_DW_OP;
#undef ONE_KNOWN_DW_OP
#undef ONE_KNOWN_DW_OP_DESC

  test ("17", "(CONST<17>)");
  test ("0x17", "(CONST<23>)");
  test ("017", "(CONST<15>)");

  test ("\"string\"", "(STR<\"string\">)");

  test ("swap", "(SHF_SWAP)");
  test ("dup", "(SHF_DUP)");
  test ("over", "(SHF_OVER)");
  test ("rot", "(SHF_ROT)");
  test ("drop", "(SHF_DROP)");
  test ("if", "(PIPE (ASSERT (PRED_NOT (PRED_EMPTY))) (SHF_DROP))");
  test ("else", "(PIPE (ASSERT (PRED_EMPTY)) (SHF_DROP))");

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
  test ("count", "(F_COUNT)");
  test ("each", "(EACH)");
  test ("universe", "(SEL_UNIVERSE)");
  test ("section", "(SEL_SECTION)");
  test ("unit", "(SEL_UNIT)");

  test ("+add", "(PROTECT (F_ADD))");
  test ("+sub", "(PROTECT (F_SUB))");
  test ("+mul", "(PROTECT (F_MUL))");
  test ("+div", "(PROTECT (F_DIV))");
  test ("+mod", "(PROTECT (F_MOD))");
  test ("+parent", "(PROTECT (F_PARENT))");
  test ("+child", "(PROTECT (F_CHILD))");
  test ("+attribute", "(PROTECT (F_ATTRIBUTE))");
  test ("+prev", "(PROTECT (F_PREV))");
  test ("+next", "(PROTECT (F_NEXT))");
  test ("+type", "(PROTECT (F_TYPE))");
  test ("+offset", "(PROTECT (F_OFFSET))");
  test ("+name", "(PROTECT (F_NAME))");
  test ("+tag", "(PROTECT (F_TAG))");
  test ("+form", "(PROTECT (F_FORM))");
  test ("+value", "(PROTECT (F_VALUE))");
  test ("+pos", "(PROTECT (F_POS))");
  test ("+count", "(PROTECT (F_COUNT))");
  test ("+each", "(PROTECT (EACH))");
  test ("+universe", "(PROTECT (SEL_UNIVERSE))");
  test ("+section", "(PROTECT (SEL_SECTION))");
  test ("+unit", "(PROTECT (SEL_UNIT))");

#define ONE_KNOWN_DW_AT(NAME, CODE)				\
  test ("@"#NAME, "(ATVAL<" #CODE ">)");			\
  test ("+@"#NAME, "(PROTECT (ATVAL<" #CODE ">))");		\
  test ("?@"#NAME, "(ASSERT (PRED_AT<" #CODE ">))");		\
  test ("!@"#NAME, "(ASSERT (PRED_NOT (PRED_AT<" #CODE ">)))");

  ALL_KNOWN_DW_AT;
#undef ONE_KNOWN_DW_AT

#define ONE_KNOWN_DW_TAG(NAME, CODE)				\
  test ("?"#NAME, "(ASSERT (PRED_TAG<" #CODE ">))");		\
  test ("!"#NAME, "(ASSERT (PRED_NOT (PRED_TAG<" #CODE ">)))");

  ALL_KNOWN_DW_TAG;
#undef ONE_KNOWN_DW_TAG

  test ("child*", "(CLOSE_STAR (F_CHILD))");
  test ("child+", "(CLOSE_PLUS (F_CHILD))");
  test ("child?", "(MAYBE (F_CHILD))");
  test ("swap*", "(CLOSE_STAR (SHF_SWAP))");
  test ("swap+", "(CLOSE_PLUS (SHF_SWAP))");
  test ("swap?", "(MAYBE (SHF_SWAP))");

  test ("child next",
	"(PIPE (F_CHILD) (F_NEXT))");
  test ("child next*",
	"(PIPE (F_CHILD) (CLOSE_STAR (F_NEXT)))");
  test ("child* next",
	"(PIPE (CLOSE_STAR (F_CHILD)) (F_NEXT))");
  test ("child+ next",
	"(PIPE (CLOSE_PLUS (F_CHILD)) (F_NEXT))");
  test ("child +next",
	"(PIPE (F_CHILD) (PROTECT (F_NEXT)))");
  test ("child+ +next",
	"(PIPE (CLOSE_PLUS (F_CHILD)) (PROTECT (F_NEXT)))");

  test ("dup swap child",
	"(PIPE (SHF_DUP) (SHF_SWAP) (F_CHILD))");
  test ("dup swap child next",
	"(PIPE (SHF_DUP) (SHF_SWAP) (F_CHILD) (F_NEXT))");

  test ("2/child",
	"(TRANSFORM (CONST<2>) (F_CHILD))");
  test ("2/child next",
	"(PIPE (TRANSFORM (CONST<2>) (F_CHILD)) (F_NEXT))");
  test ("2/(child next)",
	"(TRANSFORM (CONST<2>) (PIPE (F_CHILD) (F_NEXT)))");
  test ("2/child 2/next",
	"(PIPE (TRANSFORM (CONST<2>) (F_CHILD)) (TRANSFORM (CONST<2>) (F_NEXT)))");

  test ("(child next)",
	"(PIPE (F_CHILD) (F_NEXT))");
  test ("((child next))",
	"(PIPE (F_CHILD) (F_NEXT))");
  test ("(child (next))",
	"(PIPE (F_CHILD) (F_NEXT))");
  test ("(dup) swap child next",
	"(PIPE (SHF_DUP) (SHF_SWAP) (F_CHILD) (F_NEXT))");
  test ("dup (swap) child next",
	"(PIPE (SHF_DUP) (SHF_SWAP) (F_CHILD) (F_NEXT))");
  test ("dup swap (child) next",
	"(PIPE (SHF_DUP) (SHF_SWAP) (F_CHILD) (F_NEXT))");
  test ("dup swap child (next)",
	"(PIPE (SHF_DUP) (SHF_SWAP) (F_CHILD) (F_NEXT))");
  test ("dup (swap (child (next)))",
	"(PIPE (SHF_DUP) (SHF_SWAP) (F_CHILD) (F_NEXT))");
  test ("((((dup) swap) child) next)",
	"(PIPE (SHF_DUP) (SHF_SWAP) (F_CHILD) (F_NEXT))");
  test ("((((dup) swap)) (child next))",
	"(PIPE (SHF_DUP) (SHF_SWAP) (F_CHILD) (F_NEXT))");

  test ("dup, over",
	"(ALT (SHF_DUP) (SHF_OVER))");
  test ("dup, over, +child",
	"(ALT (SHF_DUP) (SHF_OVER) (PROTECT (F_CHILD)))");
  test ("swap,",
	"(ALT (SHF_SWAP) (NOP))");
  test ("swap dup, over",
	"(ALT (PIPE (SHF_SWAP) (SHF_DUP)) (SHF_OVER))");
  test ("swap dup, over next, parent dup",
	"(ALT (PIPE (SHF_SWAP) (SHF_DUP)) (PIPE (SHF_OVER) (F_NEXT)) "
	"(PIPE (F_PARENT) (SHF_DUP)))");
  test ("(swap dup, (over next, (parent dup)))",
	"(ALT (PIPE (SHF_SWAP) (SHF_DUP)) (PIPE (SHF_OVER) (F_NEXT)) "
	"(PIPE (F_PARENT) (SHF_DUP)))");
  test ("2/next, 2/prev",
	"(ALT (TRANSFORM (CONST<2>) (F_NEXT)) (TRANSFORM (CONST<2>) (F_PREV)))");
  test ("next, prev*",
	"(ALT (F_NEXT) (CLOSE_STAR (F_PREV)))");

  test ("[]",
	"(CAPTURE (NOP))");
  test ("[child]",
	"(CAPTURE (F_CHILD))");
  test ("[,]",
	"(CAPTURE (ALT (NOP) (NOP)))");
  test ("[,,]",
	"(CAPTURE (ALT (NOP) (NOP) (NOP)))");
  test ("[1,,2,]",
	"(CAPTURE (ALT (CONST<1>) (NOP) (CONST<2>) (NOP)))");

  std::cerr << tests << " tests total, " << failed << " failures.\n";

  if (argc > 1)
    {
      yyscan_t sc;
      if (yylex_init (&sc) != 0)
	throw std::runtime_error ("can't init lexer");

      yy_scan_string (argv[1], sc);
      std::unique_ptr <tree> t;
      if (yyparse (t, sc) == 0)
	{
	  t->dump (std::cerr);
	  std::cerr << std::endl;
	}
    }

  return 0;
}
