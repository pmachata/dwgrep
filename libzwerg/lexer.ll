%{ // -*-c++-*-
/*
   Copyright (C) 2017 Petr Machata
   Copyright (C) 2014 Red Hat, Inc.
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

#include <iostream>
#include <sstream>
#include <cctype>
#include "parser.hh"

  static yytokentype pass_string (yyscan_t yyscanner,
				  YYSTYPE *yylval, yytokentype toktype,
				  size_t ignore = 0);

  static char parse_esc_num (char const *str, int len, int ignore, int base);
%}

%option 8bit bison-bridge warn yylineno
%option noyywrap nounput batch noinput

%option reentrant
%option extra-type="vocabulary const *"

ALNUM [_a-zA-Z0-9]*
ID  [_a-zA-Z]{ALNUM}
INT [0-9]{ALNUM}
HEX [a-fA-F0-9]
OCT [0-7]

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

"," return TOK_COMMA;
"||" return TOK_DOUBLE_VBAR;
"|" return TOK_VBAR;

":" return TOK_COLON;
";" return TOK_SEMICOLON;
"->" return TOK_ARROW;
":=" return TOK_ASSIGN;

"if" return TOK_IF;
"then" return TOK_THEN;
"else" return TOK_ELSE;
"let" return TOK_LET;

"\\dbg" return TOK_DEBUG;

[?!@.\\]?{ID} return pass_string (yyscanner, yylval, TOK_WORD);
[?!]{INT} {
    return pass_string (yyscanner, yylval, TOK_NUMWORD);
}

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

<STRING>"\\"(.|[\n]) {
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

<STRING>"%s" {
  yylval->f->flush_str ();
  yylval->f->t.push_child (parse_subquery (*yyextra, ""));
}

<STRING>"%x" {
  yylval->f->flush_str ();
  yylval->f->t.push_child (parse_subquery (*yyextra, "value hex"));
}

<STRING>"%o" {
  yylval->f->flush_str ();
  yylval->f->t.push_child (parse_subquery (*yyextra, "value oct"));
}

<STRING>"%b" {
  yylval->f->flush_str ();
  yylval->f->t.push_child (parse_subquery (*yyextra, "value bin"));
}

<STRING>"%d" {
  yylval->f->flush_str ();
  yylval->f->t.push_child (parse_subquery (*yyextra, "value"));
}

<STRING>(.|[\n]) {
  yylval->f->str += *yyget_text (yyscanner);
}

<STRING><<EOF>> {
  throw std::runtime_error ("string literal not terminated");
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
      yylval->f->t.push_child (parse_subquery (*yyextra,
					       yylval->f->yank_str ()));
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

"-"?{INT} {
    return pass_string (yyscanner, yylval, TOK_LIT_INT);
}

[ \t\n]+ // Skip whitespace.

(#|[/][/])[^\n]* // Skip # or // comment.
[/][*]([^*]|[*][^/])*[*][/] // Skip /**/ comment.

[^[:space:][:alnum:]()[\]{}]+ {
    if (yyextra->find ({yyget_text (yyscanner),
			(size_t) yyget_leng (yyscanner)}) == nullptr)
      REJECT;
    return pass_string (yyscanner, yylval, TOK_OP);
}

. {
  char buf[5];
  if (std::isprint (*yytext))
    sprintf (buf, "%c", *yytext);
  else
    sprintf (buf, "0x%02x", (unsigned int) (unsigned char) *yytext);
  throw std::runtime_error
    (std::string ("Invalid character in input stream: `") + buf + "'");
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
