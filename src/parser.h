#include "lexer.h"

typedef struct ParserCtx {
  Token current_token;
  LexerCtx *lexer;
} ParserCtx;

int parse_program(LexerCtx *lexer);
int parse_function_list(ParserCtx *ctx);
int parse_function_definition(ParserCtx *ctx);
int parse_function_body(ParserCtx *ctx);
