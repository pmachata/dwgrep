%{ // -*-c++-*-
#include <sstream>
#include <iostream>

#include "lexer.hh"
#include "cst.hh"

  void
  yyerror (std::unique_ptr <tree> &t, char const *s)
  {
    fprintf (stderr, "%s\n", s);
  }
%}

%code requires {
#include <memory>
#include "tree.hh"
}

%code top {
#include "parser.hh"
}

%pure-parser
%error-verbose
%parse-param { std::unique_ptr <tree> &t }

%token TOK_LPAREN TOK_RPAREN TOK_LBRACKET TOK_RBRACKET
%token TOK_QMARK_LBRACE TOK_BANG_LBRACE TOK_ALL_LBRACE TOK_RBRACE

%token TOK_ASTERISK TOK_PLUS TOK_QMARK TOK_CARET TOK_COMMA

%token TOK_CONSTANT

%token TOK_WORD_ADD TOK_WORD_SUB TOK_WORD_MUL TOK_WORD_DIV TOK_WORD_MOD
%token TOK_WORD_EQ TOK_WORD_NE TOK_WORD_LT TOK_WORD_GT TOK_WORD_LE TOK_WORD_GE
%token TOK_WORD_MATCH TOK_WORD_SPREAD
%token TOK_WORD_CHILD TOK_WORD_PARENT TOK_WORD_NEXT TOK_WORD_PREV
%token TOK_WORD_SWAP TOK_WORD_DUP TOK_WORD_OVER TOK_WORD_ROT TOK_WORD_DROP

%token TOK_DOT_WORD TOK_AT_WORD TOK_DOT_AT_WORD
%token TOK_SLASH_AT_WORD TOK_QMARK_WORD TOK_BANG_WORD
%token TOK_QMARK_AT_WORD TOK_BANG_AT_WORD

%token TOK_LIT_STR TOK_LIT_INT TOK_EOF

%union {
  tree *t;
  const char *str;
 }

%type <t> StatementList Statement
%type <str> TOK_LIT_INT TOK_LIT_STR
%type <str> TOK_CONSTANT
%type <str> TOK_DOT_WORD TOK_AT_WORD TOK_DOT_AT_WORD TOK_SLASH_AT_WORD
%type <str> TOK_QMARK_WORD TOK_BANG_WORD TOK_QMARK_AT_WORD TOK_BANG_AT_WORD
%%

Query: StatementList TOK_EOF
  {
    t.reset ($1);
    YYACCEPT;
  }

StatementList:
  /* eps. */
  { $$ = nullptr; }

  | Statement StatementList
  { $$ = tree::create_pipe ($1, $2); }

Statement:
  TOK_LPAREN StatementList TOK_RPAREN
  { $$ = $2; }

  | Statement TOK_COMMA Statement StatementList

  | TOK_QMARK_LBRACE StatementList TOK_RBRACE
  { $$ = tree::create_unary <tree_type::SUBX_A_ANY> ($2); }

  | TOK_BANG_LBRACE StatementList TOK_RBRACE
  { $$ = tree::create_unary <tree_type::SUBX_A_NONE> ($2); }

  | TOK_ALL_LBRACE StatementList TOK_RBRACE
  { $$ = tree::create_unary <tree_type::SUBX_A_ALL> ($2); }

  // XXX precedence.  X Y* must mean X (Y*)
  | Statement TOK_ASTERISK
  { $$ = tree::create_unary <tree_type::CLOSE_STAR> ($1); }

  | Statement TOK_PLUS
  { $$ = tree::create_unary <tree_type::CLOSE_PLUS> ($1); }

  | Statement TOK_QMARK
  { $$ = tree::create_unary <tree_type::MAYBE> ($1); }

  | TOK_LIT_INT
  {
    char *endptr = NULL;
    long int val = strtol ($1, &endptr, 0);
    if (*endptr != '\0')
      std::cerr << $1 << " is not a valid string literal\n";
    $$ = tree::create_int <tree_type::CONST> (cst (val, &untyped_cst_dom));
  }

  | TOK_CONSTANT
  {
    $$ = tree::create_int <tree_type::CONST> (cst::parse ($1));
  }

  | TOK_LIT_STR
  { $$ = tree::create_str <tree_type::STR> ($1); }

  | TOK_WORD_ADD
  { $$ = tree::create_nullary <tree_type::AR_ADD> (); }

  | TOK_WORD_SUB
  { $$ = tree::create_nullary <tree_type::AR_SUB> (); }

  | TOK_WORD_MUL
  { $$ = tree::create_nullary <tree_type::AR_MUL> (); }

  | TOK_WORD_DIV
  { $$ = tree::create_nullary <tree_type::AR_DIV> (); }

  | TOK_WORD_MOD
  { $$ = tree::create_nullary <tree_type::AR_MOD> (); }

  | TOK_WORD_SWAP
  { $$ = tree::create_nullary <tree_type::SHF_SWAP> (); }

  | TOK_WORD_DUP
  { $$ = tree::create_nullary <tree_type::SHF_DUP> (); }

  | TOK_WORD_OVER
  { $$ = tree::create_nullary <tree_type::SHF_OVER> (); }

  | TOK_WORD_ROT
  { $$ = tree::create_nullary <tree_type::SHF_ROT> (); }

  | TOK_WORD_DROP
  { $$ = tree::create_nullary <tree_type::SHF_DROP> (); }

  | TOK_WORD_EQ
  { $$ = tree::create_nullary <tree_type::CMP_EQ> (); }

  | TOK_WORD_NE
  { $$ = tree::create_nullary <tree_type::CMP_NE> (); }

  | TOK_WORD_LT
  { $$ = tree::create_nullary <tree_type::CMP_LT> (); }

  | TOK_WORD_GT
  { $$ = tree::create_nullary <tree_type::CMP_GT> (); }

  | TOK_WORD_LE
  { $$ = tree::create_nullary <tree_type::CMP_LE> (); }

  | TOK_WORD_GE
  { $$ = tree::create_nullary <tree_type::CMP_GE> (); }

  | TOK_WORD_MATCH
  { $$ = tree::create_nullary <tree_type::CMP_MATCH> (); }

  | TOK_WORD_PARENT
  { $$ = tree::create_nullary <tree_type::TR_PARENT> (); }

  | TOK_WORD_CHILD
  { $$ = tree::create_nullary <tree_type::TR_CHILD> (); }

  | TOK_WORD_PREV
  { $$ = tree::create_nullary <tree_type::TR_PREV> (); }

  | TOK_WORD_NEXT
  { $$ = tree::create_nullary <tree_type::TR_NEXT> (); }

  | TOK_WORD_SPREAD
  { $$ = tree::create_nullary <tree_type::SPREAD> (); }

  | TOK_AT_WORD
  { $$ = tree::create_str <tree_type::ATVAL> ($1); }

  | TOK_DOT_WORD
  { $$ = tree::create_str <tree_type::PROPREF> ($1); }

  | TOK_DOT_AT_WORD
  { $$ = tree::create_str <tree_type::ATREF> ($1); }

  | TOK_QMARK_WORD
  { $$ = tree::create_str <tree_type::TAG_A_YES> ($1); }

  | TOK_BANG_WORD
  { $$ = tree::create_str <tree_type::TAG_A_NO> ($1); }

%%

void
test (std::string parse, std::string expect)
{
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
	}
    }
  else
    std::cerr << "can't parse: «" << parse << "»" << std::endl;
}

int
main (int argc, char *argv[])
{
  test ("17", "(CONST<17>)");
  test ("\"string\"", "(STR<\"string\">)");
  test ("DW_AT_name", "(CONST<DW_AT_name>)");
  test ("child", "(TR_CHILD)");
  test ("prev", "(TR_PREV)");
  test ("parent", "(TR_PARENT)");
  test ("next", "(TR_NEXT)");

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
