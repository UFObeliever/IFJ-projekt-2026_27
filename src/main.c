#include "lexer.h"
#include <stdio.h>
int main(void) {
  LexerCtx lexer_ctx;
  lexer_init(&lexer_ctx, stdin);
  while (true) {
    Token token = get_next_token(&lexer_ctx);
    printf("dealing with token %d\n", token.token_type);
    if (token.token_type == T_EOF) {
      break;
    }
  }

  printf("Wazzup Beijing. \n");

  return 0;
}
