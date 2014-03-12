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
OCT [0-7]

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

{ID} return pass_string (yylval, TOK_CONSTANT);

"@"{ID} return pass_string (yylval, TOK_AT_WORD, 1);
"+@"{ID} return pass_string (yylval, TOK_PLUS_AT_WORD, 2);
"?@"{ID} return pass_string (yylval, TOK_QMARK_AT_WORD, 2);
"!@"{ID} return pass_string (yylval, TOK_BANG_AT_WORD, 2);
"?"{ID} return pass_string (yylval, TOK_QMARK_WORD, 1);
"!"{ID} return pass_string (yylval, TOK_BANG_WORD, 1);

\"(\\.|[^\\\"])*\" return pass_string (yylval, TOK_LIT_STR);

0[xX]{HEX}+ |
0{OCT}+     |
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
