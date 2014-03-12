%{ // -*-c++-*-
#include <iostream>
#include "parser.hh"

  static yytokentype pass_string (YYSTYPE *yylval, yytokentype toktype,
				  size_t ignore = 0);
%}

%option 8bit bison-bridge
%option warn nodefault
%option yylineno
%option outfile="lexer.cc" header-file="lexer.hh"
%option noyywrap nounput batch noinput

ID  [_a-zA-Z][_a-zA-Z0-9]*
HEX [a-fA-F0-9]
DEC [0-9]

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

"add" return TOK_WORD_ADD;
"sub" return TOK_WORD_SUB;
"mul" return TOK_WORD_MUL;
"div" return TOK_WORD_DIV;
"mod" return TOK_WORD_MOD;

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

"?root" return TOK_QMARK_ROOT;
"!root" return TOK_BANG_ROOT;

"each" return TOK_WORD_EACH;

"child" return TOK_WORD_CHILD;
"parent" return TOK_WORD_PARENT;
"next" return TOK_WORD_NEXT;
"prev" return TOK_WORD_PREV;

"swap" return TOK_WORD_SWAP;
"dup" return TOK_WORD_DUP;
"over" return TOK_WORD_OVER;
"rot" return TOK_WORD_ROT;
"drop" return TOK_WORD_DROP;

{ID} return pass_string (yylval, TOK_CONSTANT);

"@"{ID} return pass_string (yylval, TOK_AT_WORD, 1);
"?"{ID} return pass_string (yylval, TOK_QMARK_WORD, 1);
"!"{ID} return pass_string (yylval, TOK_BANG_WORD, 1);
"?@"{ID} return pass_string (yylval, TOK_QMARK_AT_WORD, 2);
"!@"{ID} return pass_string (yylval, TOK_BANG_AT_WORD, 2);

\"(\\.|[^\\\"])*\" return pass_string (yylval, TOK_LIT_STR);

0[xX]{HEX}+ |
{DEC}+      return pass_string (yylval, TOK_LIT_INT);

[ \t\n]+ // Skip.

. {
  std::cerr << "Invalid token `" << *yytext << "'\n";
}

<<EOF>> return TOK_EOF;

%%

yytokentype
pass_string (YYSTYPE *yylval, yytokentype toktype, size_t ignore)
{
  yylval->str = yyget_text () + ignore;
  return toktype;
}
