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
  advance(parser);
  return consumed;
}

int parse_program(ParserCtx *ctx) {
  advance(ctx);

  int status;
  if ((status = parse_prolog(ctx)) != 0)
    return status;
  if ((status = parse_function_list(ctx)) != 0)
    return status;
  if (ctx->main_count == 0)
    return UNDEF_FN_ERR;
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
  while (ctx->current_token.token_type == T_EOL ||
         (ctx->current_token.token_type == T_INDENTS &&
          ctx->current_token.token_indents.count == 0)) {
    advance(ctx);
  }
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

static int parse_type(ParserCtx *ctx, TokenType *out) {
  switch (ctx->current_token.token_type) {
  case T_KW_INT:
  case T_KW_DOUBLE:
  case T_KW_STRING:
  case T_KW_UNIT:
    *out = ctx->current_token.token_type;
    advance(ctx);
    return 0;
  default:
    return ERR_SYNTAX;
  }
}

static int parse_parameters(ParserCtx *ctx, f_param **params, int *count) {
  int status;
  *params = NULL;
  *count = 0;

  if (ctx->current_token.token_type == T_UNIT_LITERAL) {
    advance(ctx);
    return 0;
  }
  if ((status = match(ctx, T_OPEN_BRACKET)) != 0)
    return status;
  if (ctx->current_token.token_type == T_CLOSE_BRACKET)
    return match(ctx, T_CLOSE_BRACKET);

  while (true) {
    if (ctx->current_token.token_type != T_IDENTIFICATOR)
      return ERR_SYNTAX;
    const char *pname = ctx->current_token.token_lexeme.lexeme;
    for (int i = 0; i < *count; i++) {
      if (strcmp((*params)[i].name, pname) == 0)
        return REDEF_FN_VAR_ERR;
    }
    advance(ctx);

    if ((status = match(ctx, T_DOUBLE_DOT)) != 0)
      return status;
    TokenType ptype;
    if ((status = parse_type(ctx, &ptype)) != 0)
      return status;

    f_param *tmp = realloc(*params, sizeof(f_param) * (size_t)(*count + 1));
    if (tmp == NULL)
      return COMPILER_ERR;
    *params = tmp;
    (*params)[*count].name = strdup(pname);
    (*params)[*count].type = ptype;
    (*count)++;

    if (ctx->current_token.token_type == T_COMMA) {
      advance(ctx);
      continue;
    }
    break;
  }
  return match(ctx, T_CLOSE_BRACKET);
}

static int parse_defsingle(ParserCtx *ctx) {
  int status;
  bool is_main = false;

  if (ctx->current_token.token_type == T_FUNC_MAIN) {
    is_main = true;
    advance(ctx);
    if (match(ctx, T_KW_DEF) != 0)
      return ERR_SYNTAX;
  } else if (ctx->current_token.token_type == T_KW_DEF) {
    advance(ctx);
  } else {
    return ERR_SYNTAX;
  }

  if (ctx->current_token.token_type != T_IDENTIFICATOR)
    return ERR_SYNTAX;
  const char *name = ctx->current_token.token_lexeme.lexeme;
  advance(ctx);

  f_param *params = NULL;
  int param_count = 0;
  if ((status = parse_parameters(ctx, &params, &param_count)) != 0)
    return status;

  TokenType ret_type = T_ERROR;
  if (ctx->current_token.token_type == T_DOUBLE_DOT) {
    advance(ctx);
    if ((status = parse_type(ctx, &ret_type)) != 0)
      return status;
  }

  if ((status = match(ctx, T_ASSIGN)) != 0)
    return status;

  if (is_main) {
    if (param_count > 0)
      return STATIC_SEM_ERR;
    if (ret_type != T_KW_UNIT)
      return STATIC_SEM_TYPE_ERR;
    ctx->main_count++;
    if (ctx->main_count > 1)
      return REDEF_FN_VAR_ERR;
  }

  symtable_entry *fentry = symtable_entry_init();
  fentry->function = true;
  fentry->declared = true;
  fentry->defined = true;
  fentry->type = ret_type;
  fentry->f_param_count = param_count;
  fentry->f_params = params;
  smt_insert_global(&ctx->smt, (char *)name, fentry);

  smt_scope_add(&ctx->smt);
  for (int i = 0; i < param_count; i++) {
    symtable_entry *pe = symtable_entry_init();
    pe->type = params[i].type;
    pe->declared = true;
    pe->defined = true;
    pe->value = true;
    smt_insert(&ctx->smt, params[i].name, pe);
  }
  ctx->ret_type = ret_type;

  if (ctx->current_token.token_type == T_EOL)
    status = parse_block(ctx, 0);
  else
    status = parse_expression(ctx, NULL, NULL);

  ctx->ret_type = T_ERROR;
  smt_scope_remove(&ctx->smt);
  return status;
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
        ctx->current_token.token_type == T_KW_ELSE ||
        ctx->current_token.token_type == T_FUNC_MAIN ||
        ctx->current_token.token_type == T_KW_DEF) {
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

static TokenType kw_from_expr_type(ExprType t) {
  switch (t) {
  case EXPR_TYPE_INT:
    return T_KW_INT;
  case EXPR_TYPE_DOUBLE:
    return T_KW_DOUBLE;
  case EXPR_TYPE_STRING:
    return T_KW_STRING;
  case EXPR_TYPE_UNIT:
    return T_KW_UNIT;
  default:
    return T_ERROR;
  }
}

static bool types_compatible(TokenType declared, ExprType actual) {
  if (actual == EXPR_TYPE_UNKNOWN) {
    return true;
  }
  switch (declared) {
  case T_KW_INT:
    return actual == EXPR_TYPE_INT;
  case T_KW_DOUBLE:
    return actual == EXPR_TYPE_INT || actual == EXPR_TYPE_DOUBLE;
  case T_KW_STRING:
    return actual == EXPR_TYPE_STRING;
  case T_KW_UNIT:
    return actual == EXPR_TYPE_UNIT;
  default:
    return false;
  }
}

static int parse_valdef(ParserCtx *ctx) {
  int status;
  if ((status = match(ctx, T_KW_VAL)) != 0)
    return status;

  if (ctx->current_token.token_type != T_IDENTIFICATOR)
    return ERR_SYNTAX;
  const char *name = ctx->current_token.token_lexeme.lexeme;
  advance(ctx);

  TokenType declared = T_ERROR;
  bool has_type = false;
  if (ctx->current_token.token_type == T_DOUBLE_DOT) {
    advance(ctx);
    switch (ctx->current_token.token_type) {
    case T_KW_INT:
    case T_KW_DOUBLE:
    case T_KW_STRING:
    case T_KW_UNIT:
      declared = ctx->current_token.token_type;
      has_type = true;
      advance(ctx);
      break;
    default:
      return ERR_SYNTAX;
    }
  }

  if ((status = match(ctx, T_ASSIGN)) != 0)
    return status;

  ExprType expr_type = EXPR_TYPE_UNKNOWN;
  if ((status = parse_expression(ctx, NULL, &expr_type)) != 0)
    return status;

  TokenType var_type;
  if (has_type) {
    if (!types_compatible(declared, expr_type))
      return STATIC_SEM_TYPE_ERR;
    var_type = declared;
  } else {
    if (expr_type == EXPR_TYPE_BOOL)
      return STATIC_SEM_TYPE_ERR;
    var_type = kw_from_expr_type(expr_type);
  }

  symtable_entry *entry = symtable_entry_init();
  entry->type = var_type;
  entry->declared = true;
  entry->defined = true;
  entry->value = true;
  smt_insert(&ctx->smt, (char *)name, entry);
  return 0;
}

static int parse_vardef(ParserCtx *ctx) {
  int status;
  if ((status = match(ctx, T_KW_VAR)) != 0)
    return status;

  if (ctx->current_token.token_type != T_IDENTIFICATOR)
    return ERR_SYNTAX;
  const char *name = ctx->current_token.token_lexeme.lexeme;
  advance(ctx);

  TokenType declared = T_ERROR;
  bool has_type = false;
  if (ctx->current_token.token_type == T_DOUBLE_DOT) {
    advance(ctx);
    switch (ctx->current_token.token_type) {
    case T_KW_INT:
    case T_KW_DOUBLE:
    case T_KW_STRING:
    case T_KW_UNIT:
      declared = ctx->current_token.token_type;
      has_type = true;
      advance(ctx);
      break;
    default:
      return ERR_SYNTAX;
    }
  }

  if ((status = match(ctx, T_ASSIGN)) != 0)
    return status;

  ExprType expr_type = EXPR_TYPE_UNKNOWN;
  if ((status = parse_expression(ctx, NULL, &expr_type)) != 0)
    return status;

  TokenType var_type;
  if (has_type) {
    if (!types_compatible(declared, expr_type))
      return STATIC_SEM_TYPE_ERR;
    var_type = declared;
  } else {
    if (expr_type == EXPR_TYPE_BOOL)
      return STATIC_SEM_TYPE_ERR;
    var_type = kw_from_expr_type(expr_type);
  }

  symtable_entry *entry = symtable_entry_init();
  entry->type = var_type;
  entry->declared = true;
  entry->defined = true;
  entry->variable = true;
  smt_insert(&ctx->smt, (char *)name, entry);
  return 0;
}

static int parse_assignment(ParserCtx *ctx) {
  int status;

  const char *name = ctx->current_token.token_lexeme.lexeme;
  symtable_entry *entry = NULL;
  if (!smt_search(&ctx->smt, (char *)name, &entry))
    return UNDEF_FN_ERR;
  if (entry->function)
    return STATIC_SEM_TYPE_ERR;
  if (!entry->variable)
    return WRITE_ERR;

  if ((status = match(ctx, T_IDENTIFICATOR)) != 0)
    return status;
  if ((status = match(ctx, T_ASSIGN)) != 0)
    return status;

  ExprType expr_type = EXPR_TYPE_UNKNOWN;
  if ((status = parse_expression(ctx, NULL, &expr_type)) != 0)
    return status;

  if (expr_type == EXPR_TYPE_BOOL || !types_compatible(entry->type, expr_type))
    return STATIC_SEM_TYPE_ERR;

  return 0;
}

static int parse_return_statement(ParserCtx *ctx) {
  int status;
  if ((status = match(ctx, T_KW_RETURN)) != 0)
    return status;

  TokenType next = ctx->current_token.token_type;
  if (next == T_EOL || next == T_EOF || next == T_SEMICOLON) {
    if (ctx->ret_type != T_ERROR && ctx->ret_type != T_KW_UNIT)
      return STATIC_SEM_TYPE_ERR;
    return 0;
  }

  ExprType expr_type = EXPR_TYPE_UNKNOWN;
  if ((status = parse_expression(ctx, NULL, &expr_type)) != 0)
    return status;

  if (ctx->ret_type != T_ERROR && ctx->ret_type != T_KW_UNIT &&
      !types_compatible(ctx->ret_type, expr_type))
    return STATIC_SEM_TYPE_ERR;

  return 0;
}

static int parse_condition(ParserCtx *ctx) {
  ExprType type = EXPR_TYPE_UNKNOWN;
  int status = parse_expression(ctx, NULL, &type);
  if (status != 0)
    return status;
  if (type != EXPR_TYPE_BOOL && type != EXPR_TYPE_UNKNOWN)
    return STATIC_SEM_TYPE_ERR;
  return 0;
}

static int parse_if_statement(ParserCtx *ctx, int current_indent) {
  int status;
  if ((status = match(ctx, T_KW_IF)) != 0)
    return status;

  if ((status = parse_condition(ctx)) != 0)
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

  if ((status = parse_condition(ctx)) != 0)
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

  smt_scope_add(&ctx->smt);
  status = parse_statement_list(ctx, block_indent);
  smt_scope_remove(&ctx->smt);
  return status;
}
