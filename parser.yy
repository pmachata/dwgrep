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

%token TOK_QMARK_LBRACE TOK_BANG_LBRACE TOK_RBRACE
%token TOK_QMARK_ALL_LBRACE TOK_BANG_ALL_LBRACE

%token TOK_ASTERISK TOK_PLUS TOK_QMARK TOK_CARET TOK_COMMA

%token TOK_CONSTANT

%token TOK_WORD_ADD TOK_WORD_SUB TOK_WORD_MUL TOK_WORD_DIV TOK_WORD_MOD
%token TOK_QMARK_EQ TOK_QMARK_NE TOK_QMARK_LT TOK_QMARK_GT TOK_QMARK_LE
%token TOK_QMARK_GE TOK_QMARK_MATCH TOK_QMARK_FIND TOK_QMARK_ROOT
%token TOK_BANG_EQ TOK_BANG_NE TOK_BANG_LT TOK_BANG_GT TOK_BANG_LE
%token TOK_BANG_GE TOK_BANG_MATCH TOK_BANG_FIND TOK_BANG_ROOT
%token TOK_WORD_EACH
%token TOK_WORD_CHILD TOK_WORD_PARENT TOK_WORD_NEXT TOK_WORD_PREV
%token TOK_WORD_SWAP TOK_WORD_DUP TOK_WORD_OVER TOK_WORD_ROT TOK_WORD_DROP

%token TOK_AT_WORD
%token TOK_QMARK_WORD TOK_BANG_WORD
%token TOK_QMARK_AT_WORD TOK_BANG_AT_WORD

%token TOK_LIT_STR TOK_LIT_INT TOK_EOF

%union {
  tree *t;
  const char *str;
 }

%type <t> StatementList Statement
%type <str> TOK_LIT_INT TOK_LIT_STR
%type <str> TOK_CONSTANT
%type <str> TOK_AT_WORD
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
  { $$ = tree::create_unary <tree_type::SUBX_ANY> ($2); }
  | TOK_BANG_LBRACE StatementList TOK_RBRACE
  { $$ = tree::create_neg (tree::create_unary <tree_type::SUBX_ALL> ($2)); }

  | TOK_QMARK_ALL_LBRACE StatementList TOK_RBRACE
  { $$ = tree::create_unary <tree_type::SUBX_ALL> ($2); }
  | TOK_BANG_ALL_LBRACE StatementList TOK_RBRACE
  { $$ = tree::create_neg (tree::create_unary <tree_type::SUBX_ANY> ($2)); }

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
    $$ = tree::create_const <tree_type::CONST> (cst (val, &untyped_cst_dom));
  }

  | TOK_CONSTANT
  {
    $$ = tree::create_const <tree_type::CONST> (cst::parse ($1));
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


  | TOK_QMARK_EQ
  { $$ = tree::create_nullary <tree_type::CMP_EQ> (); }
  | TOK_BANG_EQ
  { $$ = tree::create_neg (tree::create_nullary <tree_type::CMP_EQ> ()); }

  | TOK_QMARK_NE
  { $$ = tree::create_nullary <tree_type::CMP_NE> (); }
  | TOK_BANG_NE
  { $$ = tree::create_neg (tree::create_nullary <tree_type::CMP_NE> ()); }

  | TOK_QMARK_LT
  { $$ = tree::create_nullary <tree_type::CMP_LT> (); }
  | TOK_BANG_LT
  { $$ = tree::create_neg (tree::create_nullary <tree_type::CMP_LT> ()); }

  | TOK_QMARK_GT
  { $$ = tree::create_nullary <tree_type::CMP_GT> (); }
  | TOK_BANG_GT
  { $$ = tree::create_neg (tree::create_nullary <tree_type::CMP_GT> ()); }

  | TOK_QMARK_LE
  { $$ = tree::create_nullary <tree_type::CMP_LE> (); }
  | TOK_BANG_LE
  { $$ = tree::create_neg (tree::create_nullary <tree_type::CMP_LE> ()); }

  | TOK_QMARK_GE
  { $$ = tree::create_nullary <tree_type::CMP_GE> (); }
  | TOK_BANG_GE
  { $$ = tree::create_neg (tree::create_nullary <tree_type::CMP_GE> ()); }

  | TOK_QMARK_MATCH
  { $$ = tree::create_nullary <tree_type::STR_MATCH> (); }
  | TOK_BANG_MATCH
  { $$ = tree::create_neg (tree::create_nullary <tree_type::STR_MATCH> ()); }

  | TOK_QMARK_FIND
  { $$ = tree::create_nullary <tree_type::STR_FIND> (); }
  | TOK_BANG_FIND
  { $$ = tree::create_neg (tree::create_nullary <tree_type::STR_FIND> ()); }

  | TOK_WORD_PARENT
  { $$ = tree::create_nullary <tree_type::TR_PARENT> (); }

  | TOK_WORD_CHILD
  { $$ = tree::create_nullary <tree_type::TR_CHILD> (); }

  | TOK_WORD_PREV
  { $$ = tree::create_nullary <tree_type::TR_PREV> (); }

  | TOK_WORD_NEXT
  { $$ = tree::create_nullary <tree_type::TR_NEXT> (); }

  | TOK_WORD_EACH
  { $$ = tree::create_nullary <tree_type::EACH> (); }

  | TOK_AT_WORD
  { $$ = tree::create_const <tree_type::ATVAL> (cst::parse_attr ($1)); }
  | TOK_QMARK_AT_WORD
  { $$ = tree::create_const <tree_type::AT_ASSERT> (cst::parse_attr ($1)); }
  | TOK_BANG_AT_WORD
  {
    auto t = tree::create_const <tree_type::AT_ASSERT> (cst::parse_attr ($1));
    $$ = tree::create_neg (t);
  }

  | TOK_QMARK_WORD
  { $$ = tree::create_const <tree_type::TAG_ASSERT> (cst::parse_tag ($1)); }
  | TOK_BANG_WORD
  {
    auto t = tree::create_const <tree_type::TAG_ASSERT> (cst::parse_tag ($1));
    $$ = tree::create_neg (t);
  }

%%
