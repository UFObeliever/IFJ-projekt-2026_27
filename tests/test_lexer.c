#include "../src/lexer.h"

int main() {
  LexerCtx lexer;
  lexer_init(&lexer, stdin);
  while (1) {
    Token tok = get_next_token(&lexer);
    print_token(&tok);
    printf("\n");
    if (tok.token_type == T_EOF) {
      break;
    }
  }
  return 0;
}
