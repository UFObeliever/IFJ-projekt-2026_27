#include "parser.h"
#include "precedence.h"
#include <assert.h>
#include <stdbool.h>

#define ERR_SYNTAX 2

static void advance(ParserCtx *ctx);
static int match(ParserCtx *ctx, TokenType expected);
static int parse_prolog(ParserCtx *ctx);
static int parse_function_list(ParserCtx *ctx);
static int parse_defsingle(ParserCtx *ctx);
static int parse_statement_list(ParserCtx *ctx, int current_indent);
static int parse_statement(ParserCtx *ctx, int current_indent);
static int parse_valdef(ParserCtx *ctx);
static int parse_vardef(ParserCtx *ctx);
static int parse_assignment(ParserCtx *ctx);
static int parse_if_statement(ParserCtx *ctx, int current_indent);
static int parse_while_statement(ParserCtx *ctx, int current_indent);
static int parse_return_statement(ParserCtx *ctx);
static int parse_block(ParserCtx *ctx, int current_indent);

static void advance(ParserCtx *ctx) {
  Token tok = get_next_token(ctx->lexer);
  while (tok.token_type == T_WHITESPACE_SEP) {
    tok = get_next_token(ctx->lexer);
  }
  ctx->current_token = tok;
}

static int match(ParserCtx *ctx, TokenType expected) {
  if (ctx->current_token.token_type == expected) {
    if (expected != T_EOF) {
      advance(ctx);
    }
    return 0;
  }
  return ERR_SYNTAX;
}

Token look_next_eol(ParserCtx *parser) { return parser->current_token; }

Token consume_token(ParserCtx *parser) {
  Token consumed = parser->current_token;
  parser->current_token = get_next_token(parser->lexer);
  return consumed;
}

int parse_program(ParserCtx *ctx) {
  advance(ctx);

  int status;
  if ((status = parse_prolog(ctx)) != 0)
    return status;
  if ((status = parse_function_list(ctx)) != 0)
    return status;

  return match(ctx, T_EOF);
}

static int parse_prolog(ParserCtx *ctx) {
  int status;
  if ((status = match(ctx, T_KW_IMPORT)) != 0)
    return status;
  if ((status = match(ctx, T_IDENTIFICATOR)) != 0)
    return status;
  if ((status = match(ctx, T_DOT)) != 0)
    return status;
  return match(ctx, T_MULT_SIGN);
}

static int parse_function_list(ParserCtx *ctx) {
  if (ctx->current_token.token_type == T_EOF) {
    return 0;
  }

  int status;
  if (ctx->current_token.token_type == T_FUNC_MAIN ||
      ctx->current_token.token_type == T_KW_DEF) {
    if ((status = parse_defsingle(ctx)) != 0)
      return status;
    return parse_function_list(ctx);
  }

  return ERR_SYNTAX;
}

static int parse_defsingle(ParserCtx *ctx) {
  if (ctx->current_token.token_type == T_FUNC_MAIN) {
    advance(ctx);
    if (match(ctx, T_KW_DEF) != 0)
      return ERR_SYNTAX;
  } else if (ctx->current_token.token_type == T_KW_DEF) {
    advance(ctx);
  } else {
    return ERR_SYNTAX;
  }

  while (ctx->current_token.token_type != T_EOL &&
         ctx->current_token.token_type != T_EOF) {
    advance(ctx);
  }
  return parse_block(ctx, 0);
}

static int parse_statement_list(ParserCtx *ctx, int current_indent) {
  while (true) {
    if (ctx->current_token.token_type == T_INDENTS &&
        ctx->current_token.token_indents.count < current_indent) {
      break;
    }

    if (ctx->current_token.token_type == T_EOL ||
        (ctx->current_token.token_type == T_INDENTS &&
         ctx->current_token.token_indents.count == current_indent)) {
      advance(ctx);
      continue;
    }

    if (ctx->current_token.token_type == T_EOF ||
        ctx->current_token.token_type == T_KW_ELSE) {
      break;
    }

    int status = parse_statement(ctx, current_indent);
    if (status != 0)
      return status;
  }
  return 0;
}

static int parse_statement(ParserCtx *ctx, int current_indent) {
  switch (ctx->current_token.token_type) {
  case T_KW_VAL:
    return parse_valdef(ctx);
  case T_KW_VAR:
    return parse_vardef(ctx);
  case T_KW_IF:
    return parse_if_statement(ctx, current_indent);
  case T_KW_WHILE:
    return parse_while_statement(ctx, current_indent);
  case T_KW_RETURN:
    return parse_return_statement(ctx);
  case T_IDENTIFICATOR:
    return parse_assignment(ctx);
  default:
    assert(false && "Parser switch fell-through in parse_statement");
    return ERR_SYNTAX;
  }
}

static int parse_valdef(ParserCtx *ctx) {
  int status;
  if ((status = match(ctx, T_KW_VAL)) != 0)
    return status;
  if ((status = match(ctx, T_IDENTIFICATOR)) != 0)
    return status;
  if ((status = match(ctx, T_ASSIGN)) != 0)
    return status;

  return parse_expression(ctx, NULL, NULL);
}

static int parse_vardef(ParserCtx *ctx) {
  int status;
  if ((status = match(ctx, T_KW_VAR)) != 0)
    return status;
  if ((status = match(ctx, T_IDENTIFICATOR)) != 0)
    return status;
  if ((status = match(ctx, T_ASSIGN)) != 0)
    return status;

  return parse_expression(ctx, NULL, NULL);
}

static int parse_assignment(ParserCtx *ctx) {
  int status;
  if ((status = match(ctx, T_IDENTIFICATOR)) != 0)
    return status;
  if ((status = match(ctx, T_ASSIGN)) != 0)
    return status;

  return parse_expression(ctx, NULL, NULL);
}

static int parse_return_statement(ParserCtx *ctx) {
  int status;
  if ((status = match(ctx, T_KW_RETURN)) != 0)
    return status;

  return parse_expression(ctx, NULL, NULL);
}

static int parse_if_statement(ParserCtx *ctx, int current_indent) {
  int status;
  if ((status = match(ctx, T_KW_IF)) != 0)
    return status;

  if ((status = parse_expression(ctx, NULL, NULL)) != 0)
    return status;

  if ((status = match(ctx, T_KW_THEN)) != 0)
    return status;

  if ((status = parse_block(ctx, current_indent)) != 0)
    return status;

  while (ctx->current_token.token_type == T_EOL ||
         (ctx->current_token.token_type == T_INDENTS &&
          ctx->current_token.token_indents.count == current_indent)) {
    advance(ctx);
  }

  if ((status = match(ctx, T_KW_ELSE)) != 0)
    return status;
  return parse_block(ctx, current_indent);
}

static int parse_while_statement(ParserCtx *ctx, int current_indent) {
  int status;
  if ((status = match(ctx, T_KW_WHILE)) != 0)
    return status;

  if ((status = parse_expression(ctx, NULL, NULL)) != 0)
    return status;

  if ((status = match(ctx, T_KW_DO)) != 0)
    return status;

  return parse_block(ctx, current_indent);
}

static int parse_block(ParserCtx *ctx, int expected_indent) {
  int status;

  if ((status = match(ctx, T_EOL)) != 0)
    return status;

  if (ctx->current_token.token_type != T_INDENTS)
    return ERR_SYNTAX;
  if (ctx->current_token.token_indents.count <= expected_indent)
    return ERR_SYNTAX;

  int block_indent = ctx->current_token.token_indents.count;
  advance(ctx);

  return parse_statement_list(ctx, block_indent);
}
