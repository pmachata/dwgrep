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
"all{" return TOK_ALL_LBRACE;
"}" return TOK_RBRACE;

"*" return TOK_ASTERISK;
"+" return TOK_PLUS;
"?" return TOK_QMARK;
"^" return TOK_CARET;
"," return TOK_COMMA;

"DW_AT_"{ID} return pass_string (yylval, TOK_DW_AT_ID);
"DW_TAG_"{ID} return pass_string (yylval, TOK_DW_TAG_ID);
"DW_FORM_"{ID} return pass_string (yylval, TOK_DW_FORM_ID);
"DW_OP_"{ID} return pass_string (yylval, TOK_DW_OP_ID);
"DW_ATE_"{ID} return pass_string (yylval, TOK_DW_ATE_ID); // XXX and more

"add" return TOK_WORD_ADD;
"sub" return TOK_WORD_SUB;
"mul" return TOK_WORD_MUL;
"div" return TOK_WORD_DIV;
"mod" return TOK_WORD_MOD;

"eq" return TOK_WORD_EQ;
"ne" return TOK_WORD_NE;
"lt" return TOK_WORD_LT;
"gt" return TOK_WORD_GT;
"le" return TOK_WORD_LE;
"ge" return TOK_WORD_GE;
"match" return TOK_WORD_MATCH;

"spread" return TOK_WORD_SPREAD;

"child" return TOK_WORD_CHILD;
"parent" return TOK_WORD_PARENT;
"next" return TOK_WORD_NEXT;
"prev" return TOK_WORD_PREV;

"swap" return TOK_WORD_SWAP;
"dup" return TOK_WORD_DUP;
"over" return TOK_WORD_OVER;
"rot" return TOK_WORD_ROT;
"drop" return TOK_WORD_DROP;

"."{ID} return pass_string (yylval, TOK_DOT_WORD, 1);
"@"{ID} return pass_string (yylval, TOK_AT_WORD, 1);
".@"{ID} return pass_string (yylval, TOK_DOT_AT_WORD, 2);
"/@"{ID} return pass_string (yylval, TOK_SLASH_AT_WORD, 2);
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
