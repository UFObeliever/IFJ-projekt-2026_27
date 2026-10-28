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
} ParserCtx;

Token look_next_eol(ParserCtx *parser);
Token consume_token(ParserCtx *parser);
int parse_program(ParserCtx *ctx);

#endif
