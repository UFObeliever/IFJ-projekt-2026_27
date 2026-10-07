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

  advance(ctx);
  return 0;
}

int parse_program(LexerCtx *lexer) {
  ParserCtx ctx;
  ctx.lexer = lexer;

  int res = parse_prolog(&ctx);
  if (res != 0)
    return res;

  return parse_function_list(&ctx);
}

int parse_function_list(ParserCtx *ctx) {
  while (ctx->current_token.token_type != T_EOF) {
    if (ctx->current_token.token_type == T_FUNC_MAIN ||
        ctx->current_token.token_type == T_KW_DEF) {
      int res = parse_function_definition(ctx);
      if (res != 0)
        return res;
    } else {
      return ERR_SYNTAX;
    }
  }
  return 0;
}

int parse_function_definition(ParserCtx *ctx) {
  if (ctx->current_token.token_type == T_FUNC_MAIN) {
    advance(ctx);
    if (ctx->current_token.token_type != T_KW_DEF) {
      return ERR_SYNTAX;
    }
    advance(ctx);
    if (ctx->current_token.token_type != T_IDENTIFICATOR ||
        strcmp(ctx->current_token.token_lexeme.lexeme, "main") != 0) {
      return ERR_SYNTAX;
    }
    advance(ctx);
  } else {
    if (ctx->current_token.token_type != T_KW_DEF) {
      return ERR_SYNTAX;
    }
    advance(ctx);

    if (ctx->current_token.token_type != T_IDENTIFICATOR) {
      return ERR_SYNTAX;
    }
    advance(ctx);
  }

  if (ctx->current_token.token_type != T_OPEN_BRACKET) {
    return ERR_SYNTAX;
  }
  advance(ctx);

  while (ctx->current_token.token_type != T_CLOSE_BRACKET) {
    if (ctx->current_token.token_type != T_IDENTIFICATOR) {
      return ERR_SYNTAX;
    }
    advance(ctx);
    if (ctx->current_token.token_type != T_DOUBLE_DOT) {
      return ERR_SYNTAX;
    }
    advance(ctx);
    if (ctx->current_token.token_type != T_KW_INT &&
        ctx->current_token.token_type != T_KW_STRING &&
        ctx->current_token.token_type != T_KW_DOUBLE &&
        ctx->current_token.token_type != T_KW_UNIT) {
      return ERR_SYNTAX;
    }
    advance(ctx);
    if (ctx->current_token.token_type == T_COMMA) {
      advance(ctx);
      if (ctx->current_token.token_type == T_CLOSE_BRACKET) {
        return ERR_SYNTAX;
      }
    }
  }
  if (ctx->current_token.token_type != T_CLOSE_BRACKET) {
    return ERR_SYNTAX;
  }
  advance(ctx);

  if (ctx->current_token.token_type != T_DOUBLE_DOT) {
    return ERR_SYNTAX;
  }
  advance(ctx);

  if (ctx->current_token.token_type != T_KW_INT &&
      ctx->current_token.token_type != T_KW_STRING &&
      ctx->current_token.token_type != T_KW_DOUBLE &&
      ctx->current_token.token_type != T_KW_UNIT) {
    return ERR_SYNTAX;
  }
  advance(ctx);

  if (ctx->current_token.token_type != T_EQUAL) {
    return ERR_SYNTAX;
  }
  advance(ctx);

  parse_function_body(ctx);

  return 0;
}

parse_function_body(ParserCtx *ctx) { return 0; }
