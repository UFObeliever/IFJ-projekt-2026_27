#include "lexer.h"
#include "parser.h"
#include <stdio.h>
int main(void) {
  LexerCtx lexer_ctx;
  lexer_init(&lexer_ctx, stdin);

  ParserCtx parser;
  parser.lexer = &lexer_ctx;
  smt_init(&parser.smt);
  parser.stack.top = -1;
  parser.ret_type = T_ERROR;
  parser.main_count = 0;

  int result = parse_program(&parser);

  smt_dispose(&parser.smt);

  lexer_free(&lexer_ctx);
  return result;
}
