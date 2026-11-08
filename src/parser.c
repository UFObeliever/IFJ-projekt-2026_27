#include "parser.h"
#include "precedence.h"
#include <assert.h>
#include <stdbool.h>

#define ERR_SYNTAX 2
#define TOKEN_CHEK(token)                                                      \
  do {                                                                         \
    if (token.token_type == T_ERROR) {                                         \
      return LEX_ANAL_ERR;                                                     \
    }                                                                          \
  } while (0);

typedef enum { SK_OTHER, SK_EXPR, SK_RETURN, SK_IF } StmtKind;

typedef struct {
  StmtKind kind;
  ExprType type;
  bool returns;
} StmtInfo;

static void advance(ParserCtx *ctx);
static int match(ParserCtx *ctx, TokenType expected);
static int parse_prolog(ParserCtx *ctx);
static int parse_function_list(ParserCtx *ctx);
static int parse_defsingle(ParserCtx *ctx);
static int parse_statement_list(ParserCtx *ctx, int current_indent,
                                StmtInfo *out);
static int parse_statement(ParserCtx *ctx, int current_indent, StmtInfo *info);
static int parse_valdef(ParserCtx *ctx);
static int parse_vardef(ParserCtx *ctx);
static int parse_assignment(ParserCtx *ctx);
static int parse_if_statement(ParserCtx *ctx, int current_indent,
                              StmtInfo *info);
static int parse_while_statement(ParserCtx *ctx, int current_indent);
static int parse_return_statement(ParserCtx *ctx, StmtInfo *info);
static int parse_block(ParserCtx *ctx, int current_indent, StmtInfo *out);
static TokenType kw_from_expr_type(ExprType t);
static bool types_compatible(TokenType declared, ExprType actual);

static Token fetch_token(ParserCtx *ctx) {
  Token t;
  if (ctx->has_peek) {
    ctx->has_peek = false;
    t = ctx->peek;
  } else {
    t = get_next_token(ctx->lexer);
  }
  if (t.token_type == T_EOL) {
    ctx->line_indent = 0;
    ctx->at_bol = true;
  } else if (t.token_type == T_INDENTS) {
    if (ctx->at_bol)
      ctx->line_indent = t.token_indents.count;
    ctx->at_bol = false;
  } else {
    ctx->at_bol = false;
  }
  return t;
}

static void advance(ParserCtx *ctx) {
  ctx->current_token = fetch_token(ctx);
  while (ctx->current_token.token_type == T_WHITESPACE_SEP ||
         ctx->current_token.token_type == T_INDENTS ||
         ctx->current_token.token_type == T_TAB) {
    ctx->current_token = fetch_token(ctx);
  }
}

static void advance_(ParserCtx *ctx) {
  ctx->current_token = fetch_token(ctx);
  while (ctx->current_token.token_type == T_WHITESPACE_SEP) {
    ctx->current_token = fetch_token(ctx);
  }
}

static int match(ParserCtx *ctx, TokenType expected) {
  if (ctx->current_token.token_type == expected) {
    fprintf(stderr, "match: token %d does match expected %d\n",
            ctx->current_token.token_type, expected);
    if (expected == T_EOL) {
      advance_(ctx);
    } else if (expected != T_EOF) {
      advance(ctx);
    }
    return 0;
  }

  fprintf(stderr, "match error: token %d does not match expected %d\n",
          ctx->current_token.token_type, expected);
  if (ctx->current_token.token_type == T_ERROR) {
    return LEX_ANAL_ERR;
  }
  return ERR_SYNTAX;
}

Token look_next_eol(ParserCtx *parser) { return parser->current_token; }

Token consume_token(ParserCtx *parser) {
  Token consumed = parser->current_token;
  advance(parser);
  return consumed;
}

Token peek_token(ParserCtx *parser) {
  if (!parser->has_peek) {
    Token t = get_next_token(parser->lexer);
    while (t.token_type == T_WHITESPACE_SEP || t.token_type == T_INDENTS ||
           t.token_type == T_TAB) {
      t = get_next_token(parser->lexer);
    }
    parser->peek = t;
    parser->has_peek = true;
  }
  return parser->peek;
}

int parse_program(ParserCtx *ctx) {
  ctx->has_peek = false;
  ctx->line_indent = 0;
  ctx->at_bol = true;
  advance_(ctx);
  TOKEN_CHEK(ctx->current_token)
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
  while (ctx->current_token.token_type == T_EOL ||
         ctx->current_token.token_type == T_WHITESPACE_SEP ||
         ctx->current_token.token_type == T_INDENTS) {
    advance(ctx);
    TOKEN_CHEK(ctx->current_token);
  }

  if ((status = match(ctx, T_KW_IMPORT)) != 0)
    return status;

  if ((status = match(ctx, T_KW_IFJ26)) != 0)
    return status;

  if ((status = match(ctx, T_DOT)) != 0)
    return status;
  if ((status = match(ctx, T_MULT_SIGN)))
    return status;
  return match(ctx, T_EOL);
}

static int parse_function_list(ParserCtx *ctx) {
  while (ctx->current_token.token_type == T_EOL ||
         (ctx->current_token.token_type == T_INDENTS &&
          ctx->current_token.token_indents.count == 0)) {
    advance(ctx);
    TOKEN_CHEK(ctx->current_token)
  }
  if (ctx->current_token.token_type == T_EOF) {
    return 0;
  }

  int status;
  if (ctx->current_token.token_type == T_FUNC_MAIN ||
      ctx->current_token.token_type == T_KW_DEF) {
    if (ctx->line_indent != 0)
      return ERR_SYNTAX;
    if ((status = parse_defsingle(ctx)) != 0)
      return status;
    return parse_function_list(ctx);
  }
  fprintf(stderr, "parse_function_list error: token %d shit happens",
          ctx->current_token.token_type);
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
    TOKEN_CHEK(ctx->current_token)
    return 0;
  default:
    fprintf(stderr, "error: unknown type was returned!!");
    return ERR_SYNTAX;
  }
}

static int parse_parameters(ParserCtx *ctx, f_param **params, int *count) {
  int status;
  *params = NULL;
  *count = 0;

  if ((status = match(ctx, T_OPEN_BRACKET)) != 0)
    return status;
  if (ctx->current_token.token_type == T_CLOSE_BRACKET)
    return match(ctx, T_CLOSE_BRACKET);

  while (true) {
    if (ctx->current_token.token_type == T_EOL) {
      advance(ctx);
      TOKEN_CHEK(ctx->current_token)
    }
    if (ctx->current_token.token_type != T_IDENTIFICATOR &&
        ctx->current_token.token_type != T_CLOSE_BRACKET)
      return ERR_SYNTAX;
    // stop parsing parameters everything is OK
    if (ctx->current_token.token_type == T_CLOSE_BRACKET) {
      fprintf(stderr, "returning found )\n");
      advance(ctx);
      TOKEN_CHEK(ctx->current_token)
      return 0;
    }
    const char *pname = ctx->current_token.token_lexeme.lexeme;
    for (int i = 0; i < *count; i++) {
      if (strcmp((*params)[i].name, pname) == 0)
        return REDEF_FN_VAR_ERR;
    }
    advance(ctx);
    TOKEN_CHEK(ctx->current_token)

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
      TOKEN_CHEK(ctx->current_token)
      continue;
    }
    break;
  }
  if (ctx->current_token.token_type == T_EOL) {
    advance(ctx);
    TOKEN_CHEK(ctx->current_token)
  }
  return match(ctx, T_CLOSE_BRACKET);
}

static int parse_defsingle(ParserCtx *ctx) {
  int status;
  bool is_main = false;

  if (ctx->current_token.token_type == T_FUNC_MAIN) {
    is_main = true;
    advance(ctx);
    TOKEN_CHEK(ctx->current_token)
    // optional T_EOL before def
    if (ctx->current_token.token_type == T_EOL) {
      advance(ctx);
      TOKEN_CHEK(ctx->current_token)
    }
    if (match(ctx, T_KW_DEF) != 0)
      return ERR_SYNTAX;
  } else if (ctx->current_token.token_type == T_KW_DEF) {
    advance(ctx);
    TOKEN_CHEK(ctx->current_token)
  } else {
    return ERR_SYNTAX;
  }

  if (ctx->current_token.token_type != T_IDENTIFICATOR)
    return ERR_SYNTAX;
  const char *name = ctx->current_token.token_lexeme.lexeme;
  advance(ctx);
  TOKEN_CHEK(ctx->current_token)

  f_param *params = NULL;
  int param_count = 0;
  if ((status = parse_parameters(ctx, &params, &param_count)) != 0)
    return status;

  TokenType ret_type = T_ERROR;
  if (ctx->current_token.token_type == T_DOUBLE_DOT) {
    advance(ctx);
    TOKEN_CHEK(ctx->current_token)
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

  if (ctx->current_token.token_type == T_EOL) {
    StmtInfo info = {SK_OTHER, EXPR_TYPE_UNKNOWN, false};
    status = parse_block(ctx, 0, &info);
    if (status == 0) {
      TokenType t = ctx->current_token.token_type;
      if (t == T_ERROR)
        status = LEX_ANAL_ERR;
      else if (t != T_EOF &&
               (ctx->line_indent != 0 || (t != T_FUNC_MAIN && t != T_KW_DEF)))
        status = ERR_SYNTAX;
    }
    if (status == 0 && ret_type != T_ERROR && ret_type != T_KW_UNIT &&
        !info.returns &&
        !(info.kind == SK_EXPR && types_compatible(ret_type, info.type)))
      status = STATIC_SEM_TYPE_ERR;
  } else {
    ExprType body_type = EXPR_TYPE_UNKNOWN;
    status = parse_expression(ctx, NULL, &body_type);
    if (status == 0 && ctx->current_token.token_type != T_EOL &&
        ctx->current_token.token_type != T_EOF)
      status = (ctx->current_token.token_type == T_ERROR) ? LEX_ANAL_ERR
                                                          : ERR_SYNTAX;
    if (status == 0 && ret_type != T_ERROR && ret_type != T_KW_UNIT &&
        !types_compatible(ret_type, body_type))
      status = STATIC_SEM_TYPE_ERR;
    if (status == 0 && ret_type == T_ERROR) {
      TokenType inferred = kw_from_expr_type(body_type);
      if (body_type == EXPR_TYPE_BOOL || inferred == T_ERROR)
        status = STATIC_SEM_TYPE_ERR;
      else
        fentry->type = inferred;
    }
  }

  ctx->ret_type = T_ERROR;
  smt_scope_remove(&ctx->smt);
  return status;
}

static int parse_statement_list(ParserCtx *ctx, int current_indent,
                                StmtInfo *out) {
  StmtInfo last = {SK_OTHER, EXPR_TYPE_UNKNOWN, false};
  while (true) {
    if (ctx->current_token.token_type == T_EOL ||
        (ctx->current_token.token_type == T_INDENTS &&
         ctx->current_token.token_indents.count == current_indent)) {
      advance(ctx);
      TOKEN_CHEK(ctx->current_token)
      continue;
    }
    TOKEN_CHEK(ctx->current_token)

    if (ctx->current_token.token_type == T_EOF)
      break;
    if (ctx->line_indent < current_indent)
      break;
    if (ctx->line_indent > current_indent)
      return ERR_SYNTAX;

    if (ctx->current_token.token_type == T_KW_ELSE ||
        ctx->current_token.token_type == T_FUNC_MAIN ||
        ctx->current_token.token_type == T_KW_DEF) {
      break;
    }

    StmtInfo cur = {SK_OTHER, EXPR_TYPE_UNKNOWN, false};
    int status = parse_statement(ctx, current_indent, &cur);
    if (status != 0)
      return status;
    last = cur;
  }
  if (out != NULL)
    *out = last;
  return 0;
}

static int parse_expr_statement(ParserCtx *ctx, StmtInfo *info) {
  ExprType t = EXPR_TYPE_UNKNOWN;
  int status = parse_expression(ctx, NULL, &t);
  if (status == 0) {
    info->kind = SK_EXPR;
    info->type = t;
  }
  return status;
}

static int end_of_simple_statement(ParserCtx *ctx, int status) {
  if (status != 0)
    return status;
  TOKEN_CHEK(ctx->current_token)
  if (ctx->current_token.token_type == T_SEMICOLON) {
    advance(ctx);
    TOKEN_CHEK(ctx->current_token)
    return 0;
  }
  if (ctx->current_token.token_type != T_EOL &&
      ctx->current_token.token_type != T_EOF)
    return ERR_SYNTAX;
  return 0;
}

static int parse_statement(ParserCtx *ctx, int current_indent, StmtInfo *info) {
  switch (ctx->current_token.token_type) {
  case T_KW_VAL:
    return end_of_simple_statement(ctx, parse_valdef(ctx));
  case T_KW_VAR:
    return end_of_simple_statement(ctx, parse_vardef(ctx));
  case T_KW_IF:
    return parse_if_statement(ctx, current_indent, info);
  case T_KW_WHILE:
    return parse_while_statement(ctx, current_indent);
  case T_KW_RETURN:
    return end_of_simple_statement(ctx, parse_return_statement(ctx, info));
  case T_IDENTIFICATOR:
    if (peek_token(ctx).token_type == T_ASSIGN)
      return end_of_simple_statement(ctx, parse_assignment(ctx));
    return end_of_simple_statement(ctx, parse_expr_statement(ctx, info));
  case T_INT_LITERAL:
  case T_DOUBLE_LITERAL:
  case T_STRING_LITERAL:
  case T_OPEN_BRACKET:
    return end_of_simple_statement(ctx, parse_expr_statement(ctx, info));
  default:
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
  TOKEN_CHEK(ctx->current_token)
  if (ctx->current_token.token_type != T_IDENTIFICATOR)
    return ERR_SYNTAX;
  const char *name = ctx->current_token.token_lexeme.lexeme;
  advance(ctx);
  TOKEN_CHEK(ctx->current_token)

  TokenType declared = T_ERROR;
  bool has_type = false;
  if (ctx->current_token.token_type == T_DOUBLE_DOT) {
    advance(ctx);
    TOKEN_CHEK(ctx->current_token)
    switch (ctx->current_token.token_type) {
    case T_KW_INT:
    case T_KW_DOUBLE:
    case T_KW_STRING:
    case T_KW_UNIT:
      declared = ctx->current_token.token_type;
      has_type = true;
      advance(ctx);
      TOKEN_CHEK(ctx->current_token)
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
  TOKEN_CHEK(ctx->current_token)

  TokenType declared = T_ERROR;
  bool has_type = false;
  if (ctx->current_token.token_type == T_DOUBLE_DOT) {
    advance(ctx);
    TOKEN_CHEK(ctx->current_token)
    switch (ctx->current_token.token_type) {
    case T_KW_INT:
    case T_KW_DOUBLE:
    case T_KW_STRING:
    case T_KW_UNIT:
      declared = ctx->current_token.token_type;
      has_type = true;
      advance(ctx);
      TOKEN_CHEK(ctx->current_token)
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

static int parse_return_statement(ParserCtx *ctx, StmtInfo *info) {
  int status;
  if ((status = match(ctx, T_KW_RETURN)) != 0)
    return status;
  info->kind = SK_RETURN;
  info->returns = true;

  TokenType next = ctx->current_token.token_type;
  TOKEN_CHEK(ctx->current_token)
  if (next == T_EOL || next == T_EOF || next == T_SEMICOLON) {
    if (ctx->ret_type != T_ERROR && ctx->ret_type != T_KW_UNIT)
      return STATIC_SEM_TYPE_ERR;
    return 0;
  }

  ExprType expr_type = EXPR_TYPE_UNKNOWN;
  if ((status = parse_expression(ctx, NULL, &expr_type)) != 0)
    return status;

  if (ctx->ret_type == T_ERROR)
    return STATIC_SEM_TYPE_ERR;
  if (ctx->ret_type != T_KW_UNIT && !types_compatible(ctx->ret_type, expr_type))
    return STATIC_SEM_TYPE_ERR;

  return 0;
}

static int parse_condition(ParserCtx *ctx, TokenType terminator) {
  ExprType type = EXPR_TYPE_UNKNOWN;
  int status = parse_expression(ctx, NULL, &type);
  if (status != 0)
    return status;
  if (ctx->current_token.token_type != terminator)
    return ERR_SYNTAX;
  if (type != EXPR_TYPE_BOOL && type != EXPR_TYPE_UNKNOWN)
    return STATIC_SEM_TYPE_ERR;
  return 0;
}

static int parse_if_statement(ParserCtx *ctx, int current_indent,
                              StmtInfo *info) {
  int status;
  StmtInfo then_info = {SK_OTHER, EXPR_TYPE_UNKNOWN, false};
  StmtInfo else_info = {SK_OTHER, EXPR_TYPE_UNKNOWN, false};
  if ((status = match(ctx, T_KW_IF)) != 0)
    return status;

  if ((status = parse_condition(ctx, T_KW_THEN)) != 0)
    return status;

  if ((status = match(ctx, T_KW_THEN)) != 0)
    return status;

  if ((status = parse_block(ctx, current_indent, &then_info)) != 0)
    return status;

  while (ctx->current_token.token_type == T_EOL)
    advance(ctx);
  if (ctx->current_token.token_type != T_KW_ELSE ||
      ctx->line_indent != current_indent)
    return ERR_SYNTAX;
  if ((status = match(ctx, T_KW_ELSE)) != 0)
    return status;
  if ((status = parse_block(ctx, current_indent, &else_info)) != 0)
    return status;

  info->kind = SK_IF;
  info->returns = then_info.returns && else_info.returns;
  return 0;
}

static int parse_while_statement(ParserCtx *ctx, int current_indent) {
  int status;
  if ((status = match(ctx, T_KW_WHILE)) != 0)
    return status;

  if ((status = parse_condition(ctx, T_KW_DO)) != 0)
    return status;

  if ((status = match(ctx, T_KW_DO)) != 0)
    return status;

  return parse_block(ctx, current_indent, NULL);
}

// Upravit block aby řešil tabulatory (začátek řádku x někde jinde)
static int parse_block(ParserCtx *ctx, int expected_indent, StmtInfo *out) {
  int status;

  if ((status = match(ctx, T_EOL)) != 0)
    return status;
  TOKEN_CHEK(ctx->current_token)
  if (ctx->current_token.token_type == T_TAB) {
    return LEX_ANAL_ERR;
  }
  if (ctx->current_token.token_type != T_INDENTS)
    return ERR_SYNTAX;

  if (ctx->current_token.token_indents.count <= expected_indent)
    return ERR_SYNTAX;

  int block_indent = ctx->current_token.token_indents.count;
  advance(ctx);

  smt_scope_add(&ctx->smt);
  status = parse_statement_list(ctx, block_indent, out);
  smt_scope_remove(&ctx->smt);
  return status;
}
