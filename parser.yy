%{ // -*-c++-*-
#include <sstream>
#include <iostream>

#include "lexer.hh"
#include "cst.hh"

  void
  yyerror (std::unique_ptr <tree> &t, yyscan_t lex, char const *s)
  {
    fprintf (stderr, "%s\n", s);
  }

  template <tree_type TT>
  tree *
  positive_assert ()
  {
    return tree::create_assert (tree::create_nullary <TT> ());
  }

  template <tree_type TT>
  tree *
  negative_assert ()
  {
    auto u = tree::create_neg (tree::create_nullary <TT> ());
    return tree::create_assert (u);
  }

  tree *
  ifelse (yytokentype tt)
  {
    auto t = tree::create_nullary <tree_type::PRED_EMPTY> ();
    if (tt == TOK_IF)
      t = tree::create_neg (t);
    else
      assert (tt == TOK_ELSE);
    auto u = tree::create_assert (t);

    auto v = tree::create_nullary <tree_type::SHF_DROP> ();
    return tree::create_pipe <tree_type::PIPE> (u, v);
  }

  cst
  parse_int (strlit str)
  {
    char *endptr = NULL;
    long int val = strtol (str.buf, &endptr, 0);
    assert (endptr >= str.buf);
    if (size_t (endptr - str.buf) != str.len)
      {
        std::string tmp (str.buf, str.len);
	throw std::runtime_error
	  (std::string ("Invalid integer literal: `") + tmp + "'");
      }
    return cst (val, &untyped_cst_dom);
   }
%}

%code requires {
#include <memory>
#include "tree.hh"

struct strlit
{
  const char *buf;
  size_t len;
};

// A helper structure that the lexer uses when parsing string
// literals.
struct fmtlit
{
  std::string str;
  tree t;
  size_t level;
  bool in_string;

  fmtlit ()
    : t { tree_type::FORMAT }
    , level { 0 }
    , in_string { false }
  {}

  void
  flush_str ()
  {
    t.take_child (tree::create_str <tree_type::STR> (str));
    str = "";
  }

  std::string
  yank_str ()
  {
    std::string tmp = str;
    str = "";
    return tmp;
  }
};
}

%code top {
#include "parser.hh"
}

%code provides {
  // This is top-level parser entry point.  Initial "universe" is
  // implied.
  tree parse_query (std::string str);

  // These two are for sub-expression parsing.  They don't add initial
  // "universe".
  tree parse_string (std::string str);
  tree parse_string (char const *begin, char const *end);
}

%pure-parser
%error-verbose
%parse-param { std::unique_ptr <tree> &ret }
%parse-param { void *yyscanner }
%lex-param { yyscanner }

%token TOK_LPAREN TOK_RPAREN TOK_LBRACKET TOK_RBRACKET

%token TOK_QMARK_LBRACE TOK_BANG_LBRACE TOK_RBRACE
%token TOK_QMARK_ALL_LBRACE TOK_BANG_ALL_LBRACE

%token TOK_ASTERISK TOK_PLUS TOK_QMARK TOK_CARET TOK_COMMA TOK_SLASH

%token TOK_ADD TOK_SUB TOK_MUL TOK_DIV TOK_MOD
%token TOK_PARENT TOK_CHILD TOK_ATTRIBUTE TOK_PREV
%token TOK_NEXT TOK_TYPE TOK_OFFSET TOK_NAME TOK_TAG
%token TOK_FORM TOK_VALUE TOK_POS TOK_COUNT TOK_EACH

%token TOK_PLUS_ADD TOK_PLUS_SUB TOK_PLUS_MUL TOK_PLUS_DIV TOK_PLUS_MOD
%token TOK_PLUS_PARENT TOK_PLUS_CHILD TOK_PLUS_ATTRIBUTE TOK_PLUS_PREV
%token TOK_PLUS_NEXT TOK_PLUS_TYPE TOK_PLUS_OFFSET TOK_PLUS_NAME TOK_PLUS_TAG
%token TOK_PLUS_FORM TOK_PLUS_VALUE TOK_PLUS_POS TOK_PLUS_COUNT TOK_PLUS_EACH

%token TOK_SWAP TOK_DUP TOK_OVER TOK_ROT TOK_DROP TOK_IF TOK_ELSE

%token TOK_QMARK_EQ TOK_QMARK_NE TOK_QMARK_LT TOK_QMARK_GT TOK_QMARK_LE
%token TOK_QMARK_GE TOK_QMARK_MATCH TOK_QMARK_FIND TOK_QMARK_EMPTY
%token TOK_QMARK_ROOT

%token TOK_BANG_EQ TOK_BANG_NE TOK_BANG_LT TOK_BANG_GT TOK_BANG_LE
%token TOK_BANG_GE TOK_BANG_MATCH TOK_BANG_FIND TOK_BANG_EMPTY
%token TOK_BANG_ROOT

%token TOK_AT_WORD TOK_PLUS_AT_WORD TOK_QMARK_AT_WORD TOK_BANG_AT_WORD
%token TOK_QMARK_WORD TOK_BANG_WORD

%token TOK_CONSTANT TOK_LIT_STR TOK_LIT_INT

%token TOK_UNIVERSE TOK_SECTION TOK_UNIT
%token TOK_PLUS_UNIVERSE TOK_PLUS_SECTION TOK_PLUS_UNIT

%token TOK_EOF

%union {
  tree *t;
  strlit s;
  fmtlit *f;
 }

%type <t> Program AltList StatementList Statement
%type <s> TOK_LIT_INT
%type <s> TOK_CONSTANT
%type <s> TOK_AT_WORD TOK_PLUS_AT_WORD TOK_QMARK_AT_WORD TOK_BANG_AT_WORD
%type <s> TOK_QMARK_WORD TOK_BANG_WORD
%type <t> TOK_LIT_STR

%%

Query: Program TOK_EOF
  {
    ret.reset ($1);
    YYACCEPT;
  }

Program: AltList
  {
    if ($1 == nullptr)
      $1 = tree::create_nullary <tree_type::NOP> ();
    $$ = $1;
  }

AltList:
   StatementList
   {
     $$ = $1;
   }

   | StatementList TOK_COMMA AltList
   {
     if ($1 == nullptr)
       $1 = tree::create_nullary <tree_type::NOP> ();
     if ($3 == nullptr)
       $3 = tree::create_nullary <tree_type::NOP> ();
     $$ = tree::create_pipe <tree_type::ALT> ($1, $3);
   }

StatementList:
  /* eps. */
  { $$ = nullptr; }

  | Statement StatementList
  {
    $$ = tree::create_pipe <tree_type::PIPE> ($1, $2);
  }

Statement:
  TOK_LPAREN Program TOK_RPAREN
  { $$ = $2; }

  | TOK_QMARK_LBRACE Program TOK_RBRACE
  {
    auto t = tree::create_unary <tree_type::PRED_SUBX_ANY> ($2);
    $$ = tree::create_assert (t);
  }
  | TOK_BANG_LBRACE Program TOK_RBRACE
  {
    auto t = tree::create_unary <tree_type::PRED_SUBX_ALL> ($2);
    auto u = tree::create_neg (t);
    $$ = tree::create_assert (u);
  }

  | TOK_QMARK_ALL_LBRACE Program TOK_RBRACE
  {
    auto t = tree::create_unary <tree_type::PRED_SUBX_ALL> ($2);
    $$ = tree::create_assert (t);
  }
  | TOK_BANG_ALL_LBRACE Program TOK_RBRACE
  {
    auto t = tree::create_unary <tree_type::PRED_SUBX_ANY> ($2);
    auto u = tree::create_neg (t);
    $$ = tree::create_assert (u);
  }

  | TOK_LBRACKET Program TOK_RBRACKET
  {
    $$ = tree::create_unary <tree_type::CAPTURE> ($2);
  }

  // XXX precedence.  X Y* must mean X (Y*)
  | Statement TOK_ASTERISK
  { $$ = tree::create_unary <tree_type::CLOSE_STAR> ($1); }

  | Statement TOK_PLUS
  { $$ = tree::create_unary <tree_type::CLOSE_PLUS> ($1); }

  | Statement TOK_QMARK
  { $$ = tree::create_unary <tree_type::MAYBE> ($1); }

  | TOK_LIT_INT TOK_SLASH Statement
  {
    auto t = tree::create_const <tree_type::CONST> (parse_int ($1));
    $$ = tree::create_binary <tree_type::TRANSFORM> (t, $3);
  }

  | TOK_LIT_INT
  { $$ = tree::create_const <tree_type::CONST> (parse_int ($1)); }

  | TOK_CONSTANT
  {
    std::string str {$1.buf, $1.len};
    $$ = tree::create_const <tree_type::CONST> (cst::parse (str));
  }

  | TOK_LIT_STR
  {
    // For string literals, we get back a tree_type::FMT node with
    // children that are a mix of tree_type::STR (which are actual
    // literals) and other node types with the embedded programs.
    // That comes directly from lexer, just return it.
    $$ = $1;
  }


  | TOK_ADD
  { $$ = tree::create_nullary <tree_type::F_ADD> (); }
  | TOK_PLUS_ADD
  { $$ = tree::create_protect (tree::create_nullary <tree_type::F_ADD> ()); }

  | TOK_SUB
  { $$ = tree::create_nullary <tree_type::F_SUB> (); }
  | TOK_PLUS_SUB
  { $$ = tree::create_protect (tree::create_nullary <tree_type::F_SUB> ()); }

  | TOK_MUL
  { $$ = tree::create_nullary <tree_type::F_MUL> (); }
  | TOK_PLUS_MUL
  { $$ = tree::create_protect (tree::create_nullary <tree_type::F_MUL> ()); }

  | TOK_DIV
  { $$ = tree::create_nullary <tree_type::F_DIV> (); }
  | TOK_PLUS_DIV
  { $$ = tree::create_protect (tree::create_nullary <tree_type::F_DIV> ()); }

  | TOK_MOD
  { $$ = tree::create_nullary <tree_type::F_MOD> (); }
  | TOK_PLUS_MOD
  { $$ = tree::create_protect (tree::create_nullary <tree_type::F_MOD> ()); }

  | TOK_PARENT
  { $$ = tree::create_nullary <tree_type::F_PARENT> (); }
  | TOK_PLUS_PARENT
  { $$ = tree::create_protect (tree::create_nullary <tree_type::F_PARENT> ()); }

  | TOK_CHILD
  { $$ = tree::create_nullary <tree_type::F_CHILD> (); }
  | TOK_PLUS_CHILD
  { $$ = tree::create_protect (tree::create_nullary <tree_type::F_CHILD> ()); }

  | TOK_ATTRIBUTE
  { $$ = tree::create_nullary <tree_type::F_ATTRIBUTE> (); }
  | TOK_PLUS_ATTRIBUTE
  {
    auto t = tree::create_nullary <tree_type::F_ATTRIBUTE> ();
    $$ = tree::create_protect (t);
  }

  | TOK_PREV
  { $$ = tree::create_nullary <tree_type::F_PREV> (); }
  | TOK_PLUS_PREV
  { $$ = tree::create_protect (tree::create_nullary <tree_type::F_PREV> ()); }

  | TOK_NEXT
  { $$ = tree::create_nullary <tree_type::F_NEXT> (); }
  | TOK_PLUS_NEXT
  { $$ = tree::create_protect (tree::create_nullary <tree_type::F_NEXT> ()); }

  | TOK_TYPE
  { $$ = tree::create_nullary <tree_type::F_TYPE> (); }
  | TOK_PLUS_TYPE
  { $$ = tree::create_protect (tree::create_nullary <tree_type::F_TYPE> ()); }

  | TOK_NAME
  { $$ = tree::create_nullary <tree_type::F_NAME> (); }
  | TOK_PLUS_NAME
  { $$ = tree::create_protect (tree::create_nullary <tree_type::F_NAME> ()); }

  | TOK_TAG
  { $$ = tree::create_nullary <tree_type::F_TAG> (); }
  | TOK_PLUS_TAG
  { $$ = tree::create_protect (tree::create_nullary <tree_type::F_TAG> ()); }

  | TOK_FORM
  { $$ = tree::create_nullary <tree_type::F_FORM> (); }
  | TOK_PLUS_FORM
  { $$ = tree::create_protect (tree::create_nullary <tree_type::F_FORM> ()); }

  | TOK_VALUE
  { $$ = tree::create_nullary <tree_type::F_VALUE> (); }
  | TOK_PLUS_VALUE
  { $$ = tree::create_protect (tree::create_nullary <tree_type::F_VALUE> ()); }

  | TOK_OFFSET
  { $$ = tree::create_nullary <tree_type::F_OFFSET> (); }
  | TOK_PLUS_OFFSET
  { $$ = tree::create_protect (tree::create_nullary <tree_type::F_OFFSET> ()); }


  | TOK_POS
  { $$ = tree::create_nullary <tree_type::F_POS> (); }
  | TOK_PLUS_POS
  { $$ = tree::create_protect (tree::create_nullary <tree_type::F_POS> ()); }

  | TOK_COUNT
  { $$ = tree::create_nullary <tree_type::F_COUNT> (); }
  | TOK_PLUS_COUNT
  { $$ = tree::create_protect (tree::create_nullary <tree_type::F_COUNT> ()); }

  | TOK_EACH
  { $$ = tree::create_nullary <tree_type::EACH> (); }
  | TOK_PLUS_EACH
  { $$ = tree::create_protect (tree::create_nullary <tree_type::EACH> ()); }


  | TOK_UNIVERSE
  { $$ = tree::create_nullary <tree_type::SEL_UNIVERSE> (); }
  | TOK_PLUS_UNIVERSE
  {
    auto t = tree::create_nullary <tree_type::SEL_UNIVERSE> ();
    $$ = tree::create_protect (t);
  }

  | TOK_SECTION
  { $$ = tree::create_nullary <tree_type::SEL_SECTION> (); }
  | TOK_PLUS_SECTION
  {
    auto t = tree::create_nullary <tree_type::SEL_SECTION> ();
    $$ = tree::create_protect (t);
  }

  | TOK_UNIT
  { $$ = tree::create_nullary <tree_type::SEL_UNIT> (); }
  | TOK_PLUS_UNIT
  {
    auto t = tree::create_nullary <tree_type::SEL_UNIT> ();
    $$ = tree::create_protect (t);
  }


  | TOK_SWAP
  { $$ = tree::create_nullary <tree_type::SHF_SWAP> (); }

  | TOK_DUP
  { $$ = tree::create_nullary <tree_type::SHF_DUP> (); }

  | TOK_OVER
  { $$ = tree::create_nullary <tree_type::SHF_OVER> (); }

  | TOK_ROT
  { $$ = tree::create_nullary <tree_type::SHF_ROT> (); }

  | TOK_DROP
  { $$ = tree::create_nullary <tree_type::SHF_DROP> (); }

  | TOK_QMARK_EQ
  { $$ = positive_assert <tree_type::PRED_EQ> (); }
  | TOK_BANG_EQ
  { $$ = negative_assert <tree_type::PRED_EQ> (); }

  | TOK_QMARK_NE
  { $$ = positive_assert <tree_type::PRED_NE> (); }
  | TOK_BANG_NE
  { $$ = negative_assert <tree_type::PRED_NE> (); }

  | TOK_QMARK_LT
  { $$ = positive_assert <tree_type::PRED_LT> (); }
  | TOK_BANG_LT
  { $$ = negative_assert <tree_type::PRED_LT> (); }

  | TOK_QMARK_GT
  { $$ = positive_assert <tree_type::PRED_GT> (); }
  | TOK_BANG_GT
  { $$ = negative_assert <tree_type::PRED_GT> (); }

  | TOK_QMARK_LE
  { $$ = positive_assert <tree_type::PRED_LE> (); }
  | TOK_BANG_LE
  { $$ = negative_assert <tree_type::PRED_LE> (); }

  | TOK_QMARK_GE
  { $$ = positive_assert <tree_type::PRED_GE> (); }
  | TOK_BANG_GE
  { $$ = negative_assert <tree_type::PRED_GE> (); }

  | TOK_QMARK_MATCH
  { $$ = positive_assert <tree_type::PRED_MATCH> (); }
  | TOK_BANG_MATCH
  { $$ = negative_assert <tree_type::PRED_MATCH> (); }

  | TOK_QMARK_FIND
  { $$ = positive_assert <tree_type::PRED_FIND> (); }
  | TOK_BANG_FIND
  { $$ = negative_assert <tree_type::PRED_FIND> (); }

  | TOK_QMARK_EMPTY
  { $$ = positive_assert <tree_type::PRED_EMPTY> (); }
  | TOK_BANG_EMPTY
  { $$ = negative_assert <tree_type::PRED_EMPTY> (); }

  | TOK_QMARK_ROOT
  { $$ = positive_assert <tree_type::PRED_ROOT> (); }
  | TOK_BANG_ROOT
  { $$ = negative_assert <tree_type::PRED_ROOT> (); }

  | TOK_IF
  { $$ = ifelse (TOK_IF); }
  | TOK_ELSE
  { $$ = ifelse (TOK_ELSE); }


  | TOK_AT_WORD
  {
    std::string str {$1.buf, $1.len};
    $$ = tree::create_const <tree_type::ATVAL> (cst::parse_attr (str));
  }
  | TOK_PLUS_AT_WORD
  {
    std::string str {$1.buf, $1.len};
    auto t = tree::create_const <tree_type::ATVAL> (cst::parse_attr (str));
    $$ = tree::create_protect (t);
  }
  | TOK_QMARK_AT_WORD
  {
    std::string str {$1.buf, $1.len};
    auto t = tree::create_const <tree_type::PRED_AT> (cst::parse_attr (str));
    $$ = tree::create_assert (t);
  }
  | TOK_BANG_AT_WORD
  {
    std::string str {$1.buf, $1.len};
    auto t = tree::create_const <tree_type::PRED_AT> (cst::parse_attr (str));
    auto u = tree::create_neg (t);
    $$ = tree::create_assert (u);
  }

  | TOK_QMARK_WORD
  {
    std::string str {$1.buf, $1.len};
    auto t = tree::create_const <tree_type::PRED_TAG> (cst::parse_tag (str));
    $$ = tree::create_assert (t);
  }
  | TOK_BANG_WORD
  {
    std::string str {$1.buf, $1.len};
    auto t = tree::create_const <tree_type::PRED_TAG> (cst::parse_tag (str));
    auto u = tree::create_neg (t);
    $$ = tree::create_assert (u);
  }

%%

struct lexer
{
  yyscan_t m_sc;

  explicit lexer (char const *begin, char const *end)
  {
    if (yylex_init (&m_sc) != 0)
      throw std::runtime_error ("Can't init lexer.");
    yy_scan_bytes (begin, end - begin, m_sc);
  }

  ~lexer ()
  {
    yylex_destroy (m_sc);
  }

  lexer (lexer const &that) = delete;
};

tree
parse_string (std::string str)
{
  char const *buf = str.c_str ();
  return parse_string (buf, buf + str.length ());
}

tree
parse_string (char const *begin, char const *end)
{
  lexer lex (begin, end);
  std::unique_ptr <tree> t;
  if (yyparse (t, lex.m_sc) == 0)
    return *t;
  throw std::runtime_error ("syntax error");
}

tree
parse_query (std::string str)
{
  tree *t = new tree { parse_string (str) };
  tree *u = tree::create_nullary <tree_type::SEL_UNIVERSE> ();
  auto v = std::unique_ptr <tree>
    { tree::create_pipe <tree_type::PIPE> (u, t) };
  return *v;
}
