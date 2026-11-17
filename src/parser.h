#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"
#include "symtable.h"

typedef struct {
  TokenType items[1024];
  int top;
} PrecedenceStack;

typedef struct ParserCtx {
  Token current_token;
  LexerCtx *lexer;
  symtable smt;
  PrecedenceStack stack;
  TokenType ret_type;
  int main_count;
  // Token peek; možná nepotřebné, když budu předělávat?
  // bool has_peek; same here
  int line_indent;
  bool at_bol;
  ASTNode *funcs; // ja radek 24 25
  ASTNode *funcs_tail;
  Token *tokens; // řada načtených tokenů
  size_t tokens_len;
  size_t tokens_cap;
  size_t pos;
} ParserCtx;

Token look_next_eol(ParserCtx *parser);
Token consume_token(ParserCtx *parser);
Token peek_token(ParserCtx *parser);
int parse_program(ParserCtx *ctx);
void parser_free(ParserCtx *ctx);
int resolve_function(ParserCtx *ctx, const char *name, symtable_entry *fentry);

#endif