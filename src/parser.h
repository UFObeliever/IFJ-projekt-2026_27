#include "lexer.h"

typedef struct ParserCtx {
  Token current_token;
  LexerCtx *lexer;
} ParserCtx;

int parse_program(LexerCtx *lexer);
