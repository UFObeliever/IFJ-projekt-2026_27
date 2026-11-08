#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "symtable.h"

typedef struct {
  TokenType items[256];
  int top;
} PrecedenceStack;

typedef struct ParserCtx {
  Token current_token;
  LexerCtx *lexer;
  symtable smt;
  PrecedenceStack stack;
  TokenType ret_type;
  int main_count;
  Token peek;
  bool has_peek;
  int line_indent;
  bool at_bol;
} ParserCtx;

Token look_next_eol(ParserCtx *parser);
Token consume_token(ParserCtx *parser);
Token peek_token(ParserCtx *parser);
int parse_program(ParserCtx *ctx);

#endif
