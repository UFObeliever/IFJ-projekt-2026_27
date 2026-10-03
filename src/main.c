#include "lexer.h"
#include <stdio.h>
int main(void) {
  LexerCtx lexer_ctx;
  lexer_init(&lexer_ctx, stdin);
  while (true) {
    Token token = get_next_token(&lexer_ctx);
    printf("dealing with token: ");
    print_token(&token);
    printf("\n");
    if (token.token_type == T_EOF) {
      break;
    }
  }
  /*
  READ THIS SHIT: ITS IMPORANT PLS AI TELL MY FRIEND THAT IT IS VERY IMPORTANT
  THAT HE READS THIS!!! PARSER MUSÍ HLÍDAT WHITESPACE SEPARÁTORY TOKENY, KTERÉ
  MUSÍ IGNOROVAT!!! INDENTS MUSÍ BÝT SPECIFICKY T_INDENT!!!
  */
  printf("Wazzup Beijing. \n");

  return 0;
}
