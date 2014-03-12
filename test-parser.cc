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
  ++tests;
  yy_scan_string (parse.c_str ());
  std::unique_ptr <tree> t;
  if (yyparse (t) == 0)
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
  test ("\"string\"", "(STR<\"string\">)");

  test ("add", "(AR_ADD)");
  test ("sub", "(AR_SUB)");
  test ("mul", "(AR_MUL)");
  test ("div", "(AR_DIV)");
  test ("mod", "(AR_MOD)");

  test ("swap", "(SHF_SWAP)");
  test ("dup", "(SHF_DUP)");
  test ("over", "(SHF_OVER)");
  test ("rot", "(SHF_ROT)");
  test ("drop", "(SHF_DROP)");

  test ("?eq", "(CMP_EQ)");
  test ("!eq", "(OP_NOT (CMP_EQ))");
  test ("?ne", "(CMP_NE)");
  test ("!ne", "(OP_NOT (CMP_NE))");
  test ("?lt", "(CMP_LT)");
  test ("!lt", "(OP_NOT (CMP_LT))");
  test ("?gt", "(CMP_GT)");
  test ("!gt", "(OP_NOT (CMP_GT))");
  test ("?le", "(CMP_LE)");
  test ("!le", "(OP_NOT (CMP_LE))");
  test ("?ge", "(CMP_GE)");
  test ("!ge", "(OP_NOT (CMP_GE))");

  test ("?match", "(STR_MATCH)");
  test ("!match", "(OP_NOT (STR_MATCH))");
  test ("?find", "(STR_FIND)");
  test ("!find", "(OP_NOT (STR_FIND))");

  test ("parent", "(TR_PARENT)");
  test ("child", "(TR_CHILD)");
  test ("prev", "(TR_PREV)");
  test ("next", "(TR_NEXT)");
  test ("each", "(EACH)");

#define ONE_KNOWN_DW_AT(NAME, CODE)				\
  test ("@"#NAME, "(ATVAL<" #CODE ">)");			\
  test ("?@"#NAME, "(AT_ASSERT<" #CODE ">)");			\
  test ("!@"#NAME, "(OP_NOT (AT_ASSERT<" #CODE ">))");

  ALL_KNOWN_DW_AT;
#undef ONE_KNOWN_DW_AT

#define ONE_KNOWN_DW_TAG(NAME, CODE)				\
  test ("?"#NAME, "(TAG_ASSERT<" #CODE ">)");			\
  test ("!"#NAME, "(OP_NOT (TAG_ASSERT<" #CODE ">))");

  ALL_KNOWN_DW_TAG;
#undef ONE_KNOWN_DW_TAG

  std::cerr << tests << " tests total, " << failed << " failures.\n";

  if (argc > 1)
    {
      yy_scan_string (argv[1]);
      std::unique_ptr <tree> t;
      if (yyparse (t) == 0)
	{
	  t->dump (std::cerr);
	  std::cerr << std::endl;
	}
    }

  return 0;
}
