%code top { // -*-c++-*-
#include "parser.hh"
}

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
    bool const raw;

    explicit fmtlit (bool a_raw);

    void flush_str ();
    std::string yank_str ();
  };
}

%code provides {
  // These two are for sub-expression parsing.
  tree parse_query (std::string str);
  tree parse_query (char const *begin, char const *end);
}

%{
  #include <sstream>
  #include <iostream>

  #include "lexer.hh"
  #include "constant.hh"
  #include "vfcst.hh"
  #include "tree_cr.hh"

  namespace
  {
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
      return tree::create_cat <tree_type::CAT> (u, v);
    }

    constant
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

      if (str.buf[0] == '0' && str.len > 1)
	{
	  if (str.buf[1] == 'x' || str.buf[1] == 'X')
	    return constant (val, &hex_constant_dom);
	  else
	    return constant (val, &oct_constant_dom);
	}
      else
	return constant (val, &unsigned_constant_dom);
    }
  }

  fmtlit::fmtlit (bool a_raw)
    : t {tree_type::FORMAT}
    , level {0}
    , in_string {false}
    , raw {a_raw}
  {}

  void
  fmtlit::flush_str ()
  {
    t.take_child (tree::create_str <tree_type::STR> (str));
    str = "";
  }

  std::string
  fmtlit::yank_str ()
  {
    std::string tmp = str;
    str = "";
    return tmp;
  }
%}

%pure-parser
%error-verbose
%parse-param { std::unique_ptr <tree> &ret }
%parse-param { void *yyscanner }
%lex-param { yyscanner }

%token TOK_LPAREN TOK_RPAREN TOK_LBRACKET TOK_RBRACKET

%token TOK_QMARK_LBRACE TOK_BANG_LBRACE TOK_RBRACE
%token TOK_QMARK_ALL_LBRACE TOK_BANG_ALL_LBRACE

%token TOK_ASTERISK TOK_PLUS TOK_QMARK TOK_MINUS TOK_COMMA TOK_SLASH

%token TOK_ADD TOK_SUB TOK_MUL TOK_DIV TOK_MOD
%token TOK_PARENT TOK_CHILD TOK_ATTRIBUTE TOK_PREV
%token TOK_NEXT TOK_TYPE TOK_OFFSET TOK_NAME TOK_TAG
%token TOK_FORM TOK_VALUE TOK_POS TOK_COUNT TOK_EACH
%token TOK_LENGTH TOK_HEX TOK_OCT

%token TOK_SWAP TOK_DUP TOK_OVER TOK_ROT TOK_DROP TOK_IF TOK_ELSE

%token TOK_QMARK_EQ TOK_QMARK_NE TOK_QMARK_LT TOK_QMARK_GT TOK_QMARK_LE
%token TOK_QMARK_GE TOK_QMARK_MATCH TOK_QMARK_FIND TOK_QMARK_EMPTY
%token TOK_QMARK_ROOT

%token TOK_BANG_EQ TOK_BANG_NE TOK_BANG_LT TOK_BANG_GT TOK_BANG_LE
%token TOK_BANG_GE TOK_BANG_MATCH TOK_BANG_FIND TOK_BANG_EMPTY
%token TOK_BANG_ROOT

%token TOK_AT_WORD  TOK_QMARK_AT_WORD TOK_BANG_AT_WORD
%token TOK_QMARK_WORD TOK_BANG_WORD

%token TOK_CONSTANT TOK_LIT_STR TOK_LIT_INT

%token TOK_UNIVERSE TOK_SECTION TOK_UNIT TOK_WINFO

%token TOK_EOF

%union {
  tree *t;
  strlit s;
  fmtlit *f;
 }

%type <t> Program AltList StatementList Statement
%type <s> TOK_LIT_INT
%type <s> TOK_CONSTANT
%type <s> TOK_AT_WORD TOK_QMARK_AT_WORD TOK_BANG_AT_WORD
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
     $$ = tree::create_cat <tree_type::ALT> ($1, $3);
   }

StatementList:
  /* eps. */
  { $$ = nullptr; }

  | Statement StatementList
  {
    $$ = tree::create_cat <tree_type::CAT> ($1, $2);
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
    auto t = tree::create_unary <tree_type::PRED_SUBX_ANY> ($2);
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
    auto t = tree::create_unary <tree_type::PRED_SUBX_ALL> ($2);
    auto u = tree::create_neg (t);
    $$ = tree::create_assert (u);
  }

  | TOK_LBRACKET TOK_RBRACKET
  {
    $$ = tree::create_nullary <tree_type::EMPTY_LIST> ();
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

  | TOK_MINUS Statement
  { $$ = tree::create_protect ($2); }

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
    if (str.length () > 2 && str[0] == 'T' && str[1] == '_')
      {
	if (str == "T_CONST")
	  $$ = tree::create_const <tree_type::CONST>
	    (constant ((int) slot_type_id::T_CONST, &slot_type_dom));
	else if (str == "T_FLOAT")
	  $$ = tree::create_const <tree_type::CONST>
	    (constant ((int) slot_type_id::T_FLOAT, &slot_type_dom));
	else if (str == "T_STR")
	  $$ = tree::create_const <tree_type::CONST>
	    (constant ((int) slot_type_id::T_STR, &slot_type_dom));
	else if (str == "T_SEQ")
	  $$ = tree::create_const <tree_type::CONST>
	    (constant ((int) slot_type_id::T_SEQ, &slot_type_dom));
	else if (str == "T_NODE")
	  $$ = tree::create_const <tree_type::CONST>
	    (constant ((int) slot_type_id::T_NODE, &slot_type_dom));
	else if (str == "T_ATTR")
	  $$ = tree::create_const <tree_type::CONST>
	    (constant ((int) slot_type_id::T_ATTR, &slot_type_dom));
	else
	  throw std::runtime_error ("Unknown slot type constant.");
      }
    else if (str == "true")
      $$ = tree::create_const <tree_type::CONST>
	(constant (1, &bool_constant_dom));
    else if (str == "false")
      $$ = tree::create_const <tree_type::CONST>
	(constant (0, &bool_constant_dom));
    else
      $$ = tree::create_const <tree_type::CONST> (constant::parse (str));
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

  | TOK_SUB
  { $$ = tree::create_nullary <tree_type::F_SUB> (); }

  | TOK_MUL
  { $$ = tree::create_nullary <tree_type::F_MUL> (); }

  | TOK_DIV
  { $$ = tree::create_nullary <tree_type::F_DIV> (); }

  | TOK_MOD
  { $$ = tree::create_nullary <tree_type::F_MOD> (); }

  | TOK_PARENT
  { $$ = tree::create_nullary <tree_type::F_PARENT> (); }

  | TOK_CHILD
  { $$ = tree::create_nullary <tree_type::F_CHILD> (); }

  | TOK_ATTRIBUTE
  { $$ = tree::create_nullary <tree_type::F_ATTRIBUTE> (); }

  | TOK_PREV
  { $$ = tree::create_nullary <tree_type::F_PREV> (); }

  | TOK_NEXT
  { $$ = tree::create_nullary <tree_type::F_NEXT> (); }

  | TOK_TYPE
  { $$ = tree::create_nullary <tree_type::F_TYPE> (); }

  | TOK_NAME
  { $$ = tree::create_nullary <tree_type::F_NAME> (); }

  | TOK_TAG
  { $$ = tree::create_nullary <tree_type::F_TAG> (); }

  | TOK_FORM
  { $$ = tree::create_nullary <tree_type::F_FORM> (); }

  | TOK_VALUE
  { $$ = tree::create_nullary <tree_type::F_VALUE> (); }

  | TOK_OFFSET
  { $$ = tree::create_nullary <tree_type::F_OFFSET> (); }


  | TOK_HEX
  { $$ = tree::create_const <tree_type::F_CAST>
      ({0, &hex_constant_dom}); }
  | TOK_OCT
  { $$ = tree::create_const <tree_type::F_CAST>
      ({0, &oct_constant_dom}); }


  | TOK_POS
  { $$ = tree::create_nullary <tree_type::F_POS> (); }

  | TOK_COUNT
  { $$ = tree::create_nullary <tree_type::F_COUNT> (); }

  | TOK_EACH
  { $$ = tree::create_nullary <tree_type::F_EACH> (); }

  | TOK_LENGTH
  { $$ = tree::create_nullary <tree_type::F_LENGTH> (); }


  | TOK_UNIVERSE
  { $$ = tree::create_nullary <tree_type::SEL_UNIVERSE> (); }

  | TOK_WINFO
  { $$ = tree::create_nullary <tree_type::SEL_WINFO> (); }

  | TOK_SECTION
  { $$ = tree::create_nullary <tree_type::SEL_SECTION> (); }

  | TOK_UNIT
  { $$ = tree::create_nullary <tree_type::SEL_UNIT> (); }


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
    auto t = tree::create_const <tree_type::F_ATTR_NAMED>
      (constant::parse_attr (str));
    auto u = tree::create_nullary <tree_type::F_VALUE> ();
    $$ = tree::create_cat <tree_type::CAT> (t, u);
  }
  | TOK_QMARK_AT_WORD
  {
    std::string str {$1.buf, $1.len};
    auto t = tree::create_const <tree_type::PRED_AT>
      (constant::parse_attr (str));
    $$ = tree::create_assert (t);
  }
  | TOK_BANG_AT_WORD
  {
    std::string str {$1.buf, $1.len};
    auto t = tree::create_const <tree_type::PRED_AT>
      (constant::parse_attr (str));
    auto u = tree::create_neg (t);
    $$ = tree::create_assert (u);
  }

  | TOK_QMARK_WORD
  {
    std::string str {$1.buf, $1.len};
    auto t = tree::create_const <tree_type::PRED_TAG>
      (constant::parse_tag (str));
    $$ = tree::create_assert (t);
  }
  | TOK_BANG_WORD
  {
    std::string str {$1.buf, $1.len};
    auto t = tree::create_const <tree_type::PRED_TAG>
      (constant::parse_tag (str));
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
parse_query (std::string str)
{
  char const *buf = str.c_str ();
  return parse_query (buf, buf + str.length ());
}

tree
parse_query (char const *begin, char const *end)
{
  lexer lex (begin, end);
  std::unique_ptr <tree> t;
  if (yyparse (t, lex.m_sc) == 0)
    return *t;
  throw std::runtime_error ("syntax error");
}
