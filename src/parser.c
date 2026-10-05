#include "parser.h"
#include <string.h>
#define ERR_SYNTAX 2

static void advance(ParserCtx *ctx) {
  Token tok = get_next_token(ctx->lexer);

  while (tok.token_type == T_WHITESPACE_SEP) {
    tok = get_next_token(ctx->lexer);
  }
  ctx->current_token = tok;
}

int parse_prolog(ParserCtx *ctx) {
  advance(ctx);
  if (ctx->current_token.token_type != T_KW_IMPORT) {
    return ERR_SYNTAX;
  }

  advance(ctx);
  if (ctx->current_token.token_type != T_IDENTIFICATOR ||
      strcmp(ctx->current_token.token_lexeme.lexeme, "ifj26") != 0) {
    return ERR_SYNTAX;
  }

  advance(ctx);
  if (ctx->current_token.token_type != T_DOT) {
    return ERR_SYNTAX;
  }

  advance(ctx);
  if (ctx->current_token.token_type != T_MULT_SIGN) {
    return ERR_SYNTAX;
  }

  return 0;
}

int parse_program(LexerCtx *lexer) {
  ParserCtx ctx;
  ctx.lexer = lexer;

  return parse_prolog(&ctx);
}
