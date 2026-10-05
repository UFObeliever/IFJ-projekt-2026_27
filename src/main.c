#include "lexer.h"
#include "parser.h"
#include <stdio.h>
int main(void) {
  LexerCtx lexer_ctx;
  lexer_init(&lexer_ctx, stdin);

  int result = parse_program(&lexer_ctx);

  lexer_free(&lexer_ctx);
  return result;
}
