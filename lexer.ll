%{ // -*-c++-*-
#include <iostream>
#include <sstream>
#include "parser.hh"

  static yytokentype pass_string (yyscan_t yyscanner,
				  YYSTYPE *yylval, yytokentype toktype,
				  size_t ignore = 0);

  static char parse_esc_num (char const *str, int len, int ignore, int base);
%}

%option 8bit bison-bridge warn yylineno
%option outfile="lexer.cc" header-file="lexer.hh"
%option noyywrap nounput batch noinput

%option reentrant

ID  [_a-zA-Z][_a-zA-Z0-9]*
HEX [a-fA-F0-9]
DEC [0-9]
OCT [0-7]
BIN [01]

%x STRING
%x STRING_EMBEDDED

%%

"(" return TOK_LPAREN;
")" return TOK_RPAREN;
"?(" return TOK_QMARK_LPAREN;
"!(" return TOK_BANG_LPAREN;

"[" return TOK_LBRACKET;
"]" return TOK_RBRACKET;
"{" return TOK_LBRACE;
"}" return TOK_RBRACE;

"*" return TOK_ASTERISK;
"+" return TOK_PLUS;
"?" return TOK_QMARK;
"-" return TOK_MINUS;
"," return TOK_COMMA;
"/" return TOK_SLASH;
";" return TOK_SEMICOLON;
"->" return TOK_ARROW;
"||" return TOK_DOUBLE_VBAR;

"apply" return TOK_APPLY;
"if" return TOK_IF;
"then" return TOK_THEN;
"else" return TOK_ELSE;

"type" return TOK_TYPE;
"hex" return TOK_HEX;
"oct" return TOK_OCT;
"bin" return TOK_BIN;
"pos" return TOK_POS;
"universe" return TOK_UNIVERSE;
"section" return TOK_SECTION;
"\\dbg" return TOK_DEBUG;

"?match" return TOK_QMARK_MATCH;
"!match" return TOK_BANG_MATCH;
"?find" return TOK_QMARK_FIND;
"!find" return TOK_BANG_FIND;
"?empty" return TOK_QMARK_EMPTY;
"!empty" return TOK_BANG_EMPTY;

[?!]?[@]?{ID} return pass_string (yyscanner, yylval, TOK_WORD);

"\"" {
  BEGIN STRING;
  yylval->f = new fmtlit {false};
}

"r\"" {
  BEGIN STRING;
  yylval->f = new fmtlit {true};
}

<STRING>"\\"[0-3]{OCT}?{OCT}? {
  yylval->f->str += parse_esc_num (yyget_text (yyscanner),
				   yyget_leng (yyscanner), 1, 8);
}

<STRING>"\\x"{HEX}{HEX} {
  yylval->f->str += parse_esc_num (yyget_text (yyscanner),
				   yyget_leng (yyscanner), 2, 16);
}

<STRING>"\\". {
  if (yylval->f->raw)
    {
      yylval->f->str += yyget_text (yyscanner)[0];
      yylval->f->str += yyget_text (yyscanner)[1];
    }
  else
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
}

 /* String continuation: "blah"\ "foo" is the same as "blahfoo".  */
<STRING>"\"\\"[ \t\n]*"\"" {
  yylval->f->raw = false;
}
<STRING>"\"\\"[ \t\n]*"r\"" {
  yylval->f->raw = true;
}

<STRING>"\"" {
  BEGIN INITIAL;

  fmtlit *f = yylval->f;
  f->flush_str ();

  yylval->t = new tree {f->t};

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
  yylval->f->t.push_child (parse_query (""));
}

<STRING>"%x" {
  yylval->f->flush_str ();
  yylval->f->t.push_child (parse_query ("value hex"));
}

<STRING>"%o" {
  yylval->f->flush_str ();
  yylval->f->t.push_child (parse_query ("value oct"));
}

<STRING>"%b" {
  yylval->f->flush_str ();
  yylval->f->t.push_child (parse_query ("value bin"));
}

<STRING>"%d" {
  yylval->f->flush_str ();
  yylval->f->t.push_child (parse_query ("value"));
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
      yylval->f->t.push_child (parse_query (yylval->f->yank_str ()));
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

"-"?0[xX]{HEX}+ |
"-"?0[oO]?{OCT}+ |
"-"?0[bB]?{BIN}+ |
"-"?{DEC}+      return pass_string (yyscanner, yylval, TOK_LIT_INT);

[ \t\n]+ // Skip.

. {
  std::stringstream ss;
  ss << "invalid token `" << *yytext << "'";
  throw std::runtime_error (ss.str ());
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
