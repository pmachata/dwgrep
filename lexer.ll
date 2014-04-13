%{ // -*-c++-*-
#include <iostream>
#include "parser.hh"

  static yytokentype pass_string (yyscan_t yyscanner,
				  YYSTYPE *yylval, yytokentype toktype,
				  size_t ignore = 0);

  static char parse_esc_num (char const *str, int len, int ignore, int base);
%}

%option 8bit bison-bridge
%option warn nodefault
%option yylineno
%option outfile="lexer.cc" header-file="lexer.hh"
%option noyywrap nounput batch noinput

%option reentrant

ID  [_a-zA-Z][_a-zA-Z0-9]*
HEX [a-fA-F0-9]
DEC [0-9]
OCT [0-7]

%x STRING
%x STRING_EMBEDDED

%%

"(" return TOK_LPAREN;
")" return TOK_RPAREN;
"[" return TOK_LBRACKET;
"]" return TOK_RBRACKET;

"?{" return TOK_QMARK_LBRACE;
"!{" return TOK_BANG_LBRACE;
"?all{" return TOK_QMARK_ALL_LBRACE;
"!all{" return TOK_BANG_ALL_LBRACE;
"}" return TOK_RBRACE;

"*" return TOK_ASTERISK;
"+" return TOK_PLUS;
"?" return TOK_QMARK;
"," return TOK_COMMA;
"/" return TOK_SLASH;

"swap" return TOK_SWAP;
"dup" return TOK_DUP;
"over" return TOK_OVER;
"rot" return TOK_ROT;
"drop" return TOK_DROP;
"if" return TOK_IF;
"else" return TOK_ELSE;

"add" return TOK_ADD;
"sub" return TOK_SUB;
"mul" return TOK_MUL;
"div" return TOK_DIV;
"mod" return TOK_MOD;
"parent" return TOK_PARENT;
"child" return TOK_CHILD;
"attribute" return TOK_ATTRIBUTE;
"prev" return TOK_PREV;
"next" return TOK_NEXT;
"type" return TOK_TYPE;
"offset" return TOK_OFFSET;
"name" return TOK_NAME;
"tag" return TOK_TAG;
"form" return TOK_FORM;
"value" return TOK_VALUE;
"pos" return TOK_POS;
"count" return TOK_COUNT;
"each" return TOK_EACH;
"length" return TOK_LENGTH;
"universe" return TOK_UNIVERSE;
"section" return TOK_SECTION;
"unit" return TOK_UNIT;

"+add" return TOK_PLUS_ADD;
"+sub" return TOK_PLUS_SUB;
"+mul" return TOK_PLUS_MUL;
"+div" return TOK_PLUS_DIV;
"+mod" return TOK_PLUS_MOD;
"+parent" return TOK_PLUS_PARENT;
"+child" return TOK_PLUS_CHILD;
"+attribute" return TOK_PLUS_ATTRIBUTE;
"+prev" return TOK_PLUS_PREV;
"+next" return TOK_PLUS_NEXT;
"+type" return TOK_PLUS_TYPE;
"+offset" return TOK_PLUS_OFFSET;
"+name" return TOK_PLUS_NAME;
"+tag" return TOK_PLUS_TAG;
"+form" return TOK_PLUS_FORM;
"+value" return TOK_PLUS_VALUE;
"+pos" return TOK_PLUS_POS;
"+count" return TOK_PLUS_COUNT;
"+each" return TOK_PLUS_EACH;
"+length" return TOK_PLUS_LENGTH;
"+universe" return TOK_PLUS_UNIVERSE;
"+section" return TOK_PLUS_SECTION;
"+unit" return TOK_PLUS_UNIT;

"?eq" return TOK_QMARK_EQ;
"!eq" return TOK_BANG_EQ;
"?ne" return TOK_QMARK_NE;
"!ne" return TOK_BANG_NE;
"?lt" return TOK_QMARK_LT;
"!lt" return TOK_BANG_LT;
"?gt" return TOK_QMARK_GT;
"!gt" return TOK_BANG_GT;
"?le" return TOK_QMARK_LE;
"!le" return TOK_BANG_LE;
"?ge" return TOK_QMARK_GE;
"!ge" return TOK_BANG_GE;

"?match" return TOK_QMARK_MATCH;
"!match" return TOK_BANG_MATCH;
"?find" return TOK_QMARK_FIND;
"!find" return TOK_BANG_FIND;
"?empty" return TOK_QMARK_EMPTY;
"!empty" return TOK_BANG_EMPTY;

"?root" return TOK_QMARK_ROOT;
"!root" return TOK_BANG_ROOT;

{ID} return pass_string (yyscanner, yylval, TOK_CONSTANT);

"@"{ID} return pass_string (yyscanner, yylval, TOK_AT_WORD, 1);
"+@"{ID} return pass_string (yyscanner, yylval, TOK_PLUS_AT_WORD, 2);
"?@"{ID} return pass_string (yyscanner, yylval, TOK_QMARK_AT_WORD, 2);
"!@"{ID} return pass_string (yyscanner, yylval, TOK_BANG_AT_WORD, 2);
"?"{ID} return pass_string (yyscanner, yylval, TOK_QMARK_WORD, 1);
"!"{ID} return pass_string (yyscanner, yylval, TOK_BANG_WORD, 1);

"\"" {
  BEGIN STRING;
  yylval->f = new fmtlit {false};
}

"+\"" {
  BEGIN STRING;
  yylval->f = new fmtlit {true};
}

<STRING>"\\"[0-3]{OCT}?{OCT}? {
  yylval->f->str += parse_esc_num (yyget_text (yyscanner),
				   yyget_leng (yyscanner), 1, 8);
  BEGIN STRING;
}

<STRING>"\\x"{HEX}{HEX} {
  yylval->f->str += parse_esc_num (yyget_text (yyscanner),
				   yyget_leng (yyscanner), 2, 16);
  BEGIN STRING;
}

<STRING>"\\". {
  switch (yyget_text (yyscanner)[1])
    {
    case 'a': yylval->f->str += '\a'; break;
    case 'b': yylval->f->str += '\b'; break;
    case 'e': yylval->f->str += '\e'; break;
    case 't': yylval->f->str += '\t'; break;
    case 'n': yylval->f->str += '\n'; break;
    case 'v': yylval->f->str += '\v'; break;
    case 'f': yylval->f->str += '\f'; break;
    case 'r': yylval->f->str += '\r'; break;
    case '\n': break; // Ignore the newline.

    default:
      yylval->f->str += yyget_text (yyscanner)[1];
      break;
    }

  BEGIN STRING;
}

<STRING>"\"" {
  BEGIN INITIAL;

  fmtlit *f = yylval->f;
  f->flush_str ();

  yylval->t = new tree {f->t};
  if (f->protect)
    yylval->t = tree::create_protect (yylval->t);

  delete f;

  return TOK_LIT_STR;
}

<STRING>"%%" yylval->f->str += '%';

<STRING>"%(" {
  yylval->f->flush_str ();
  BEGIN STRING_EMBEDDED;
}

<STRING>"%{" {
  std::cerr << "Warning: %{ and %} have no special meaning in a format string.\n"
	    << "Did you mean %( and %)?\n";
  yylval->f->str += "%{";
}

<STRING>"%s" {
  yylval->f->flush_str ();
  yylval->f->t.push_back (parse_string (""));
}

<STRING>. {
  yylval->f->str += *yyget_text (yyscanner);
}

<STRING_EMBEDDED>[\(\[\{] {
  yylval->f->str += *yyget_text (yyscanner);
  if (! yylval->f->in_string)
    ++yylval->f->level;
}

<STRING_EMBEDDED>[\)\]\}] {
  yylval->f->str += *yyget_text (yyscanner);
  if (! yylval->f->in_string)
    if (yylval->f->level-- == 0)
      throw std::runtime_error
	("too many closing parentheses in embedded expression");
}

<STRING_EMBEDDED>"\\\"" {
  yylval->f->str += "\\\"";
}

<STRING_EMBEDDED>"\"" {
  yylval->f->in_string = ! yylval->f->in_string;
  yylval->f->str += '"';
}

<STRING_EMBEDDED>"%(" {
  yylval->f->in_string = false;
  yylval->f->str += "%(";
  ++yylval->f->level;
}

<STRING_EMBEDDED>"%)" {
  yylval->f->in_string = true;
  if (yylval->f->level == 0)
    {
      yylval->f->t.push_back (parse_string (yylval->f->yank_str ()));
      BEGIN STRING;
    }
  else
    {
      --yylval->f->level;
      yylval->f->str += "%)";
    }
}

<STRING_EMBEDDED>. {
  yylval->f->str += *yyget_text (yyscanner);
}

<STRING_EMBEDDED><<EOF>> {
  throw std::runtime_error
    ("too few closing parentheses in embedded expression");
}

0[xX]{HEX}+ |
0{OCT}+     |
{DEC}+      return pass_string (yyscanner, yylval, TOK_LIT_INT);

[ \t\n]+ // Skip.

. {
  std::cerr << "Invalid token `" << *yytext << "'\n";
}

<<EOF>> return TOK_EOF;

%%

yytokentype
pass_string (yyscan_t sc, YYSTYPE *val, yytokentype toktype, size_t ignore)
{
  int len = yyget_leng (sc);
  assert (len >= 0 && size_t (len) >= ignore);
  val->s = strlit { yyget_text (sc) + ignore, size_t (len - ignore) };
  return toktype;
}

char
parse_esc_num (char const *str, int len, int ignore, int base)
{
  assert (ignore < len);
  str += ignore;
  len -= ignore;

  char buf[len + 1];
  memcpy (buf, str, len);
  buf[len] = 0;

  char *endptr = nullptr;
  unsigned long int i = strtoul (buf, &endptr, base);
  assert (endptr >= buf);

  if (endptr - buf != len)
    throw std::runtime_error
      (std::string ("Invalid escape: `") + buf + "'");

  assert (i <= 255);
  return (char) (unsigned char) i;
}
