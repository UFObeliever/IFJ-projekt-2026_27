#include "parser.h"
#include "precedence.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define ERR_SYNTAX 2
#define TOKEN_CHEK(token)                                                      \
  do {                                                                         \
    if ((token).token_type == T_ERROR) {                                       \
      return LEX_ANAL_ERR;                                                     \
    }                                                                          \
  } while (0);

typedef enum { SK_OTHER, SK_EXPR, SK_RETURN, SK_IF } StmtKind;

typedef struct {
  StmtKind kind;
  ExprType type;
  bool returns;
  ASTNode *node;
} StmtInfo;

typedef struct {
  const char *name;
  f_param *params;
  int param_count;
  int list_count;
  int *list_sizes;
  TokenType ret_type;
  bool is_main;
} FuncHeader;

static void advance(ParserCtx *ctx);
static int match(ParserCtx *ctx, TokenType expected);
static int parse_prolog(ParserCtx *ctx);
static int parse_function_header(ParserCtx *ctx, FuncHeader *h);
static int parse_function_list(ParserCtx *ctx);
static int parse_defsingle(ParserCtx *ctx);
static int collect_signatures(ParserCtx *ctx);
static int parse_statement_list(ParserCtx *ctx, int current_indent,
                                StmtInfo *out, ASTNode **list);
static int parse_statement(ParserCtx *ctx, int current_indent, StmtInfo *info);
static int parse_valdef(ParserCtx *ctx, StmtInfo *info);
static int parse_vardef(ParserCtx *ctx, StmtInfo *info);
static int parse_assignment(ParserCtx *ctx, StmtInfo *info);
static int parse_if_statement(ParserCtx *ctx, int current_indent,
                              StmtInfo *info);
static int parse_while_statement(ParserCtx *ctx, int current_indent,
                                 StmtInfo *info);
static int parse_return_statement(ParserCtx *ctx, StmtInfo *info);
static int parse_block(ParserCtx *ctx, int current_indent, StmtInfo *out,
                       ASTNode **blk);
static TokenType kw_from_expr_type(ExprType t);
static bool types_compatible(TokenType declared, ExprType actual);
static Token raw_token(ParserCtx *ctx, size_t idx); // added for SCIENCE!!!

static ExprType expr_from_kw(TokenType t) {
  switch (t) {
  case T_KW_INT:
    return EXPR_TYPE_INT;
  case T_KW_DOUBLE:
    return EXPR_TYPE_DOUBLE;
  case T_KW_STRING:
    return EXPR_TYPE_STRING;
  case T_KW_UNIT:
    return EXPR_TYPE_UNIT;
  default:
    return EXPR_TYPE_UNKNOWN;
  }
}

static void list_append(ASTNode **head, ASTNode **tail, ASTNode *n) {
  if (!n)
    return;
  if (*tail)
    (*tail)->next = n;
  else
    *head = n;
  *tail = n;
}

static Token raw_token(ParserCtx *ctx, size_t idx) {
  while (idx >= ctx->tokens_len) {
    if (ctx->tokens_len > 0) {
      Token last = ctx->tokens[ctx->tokens_len - 1];
      if (last.token_type == T_EOF || last.token_type == T_ERROR) {
        return last;
      }
    }

    if (ctx->tokens_len == ctx->tokens_cap) {
      size_t cap = ctx->tokens_cap ? ctx->tokens_cap * 2 : 256;
      Token *tmp = realloc(ctx->tokens, cap * sizeof(Token));
      if (tmp == NULL) {
        fprintf(stderr, "...");
        exit(COMPILER_ERR);
      }

      ctx->tokens = tmp;
      ctx->tokens_cap = cap;
    }
    ctx->tokens[ctx->tokens_len++] = get_next_token(ctx->lexer);
  }
  return ctx->tokens[idx];
}

static SourceMark mark_get(const ParserCtx *ctx) {
  SourceMark m = {ctx->pos, ctx->current_token, ctx->line_indent, ctx->at_bol};
  return m;
}

static void mark_set(ParserCtx *ctx, const SourceMark *m) {
  ctx->pos = m->pos;
  ctx->current_token = m->tok;
  ctx->line_indent = m->line_indent;
  ctx->at_bol = m->at_bol;
}

static Token fetch_token(ParserCtx *ctx) {
  Token t = raw_token(ctx, ctx->pos++);

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
    DEBUG_LOG("match: token %d does match expected %d\n",
              ctx->current_token.token_type, expected);
    if (expected == T_EOL) {
      advance_(ctx);
    } else if (expected != T_EOF) {
      advance(ctx);
    }
    return 0;
  }

  DEBUG_LOG("match error: token %d does not match expected %d\n",
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
  size_t i = parser->pos;
  Token t = raw_token(parser, i);
  while (t.token_type == T_WHITESPACE_SEP || t.token_type == T_INDENTS ||
         t.token_type == T_TAB) {
    t = raw_token(parser, ++i);
  }
  return t;
}

void parser_free(ParserCtx *ctx) {
  free(ctx->tokens);
  ctx->tokens = NULL;
  ctx->tokens_len = ctx->tokens_cap = ctx->pos = 0;
}

int parse_program(ParserCtx *ctx) {
  ctx->tokens = NULL;
  ctx->tokens_len = 0;
  ctx->tokens_cap = 0;
  ctx->pos = 0;
  ctx->line_indent = 0;
  ctx->at_bol = true;
  advance_(ctx);
  TOKEN_CHEK(ctx->current_token)
  int status;
  if ((status = parse_prolog(ctx)) != 0)
    return status;
  if ((status = collect_signatures(ctx)) != 0)
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
      if (ctx->current_token.token_type == T_CLOSE_BRACKET)
        return ERR_SYNTAX;
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

static void free_params(f_param *params, int count) {
  for (int i = 0; i < count; i++)
    free(params[i].name);
  free(params);
}

static int parse_function_header(ParserCtx *ctx, FuncHeader *h) {
  int status;
  h->name = NULL;
  h->params = NULL;
  h->param_count = 0;
  h->list_count = 0;
  h->list_sizes = NULL;
  h->ret_type = T_ERROR;
  h->is_main = false;

  if (ctx->current_token.token_type == T_FUNC_MAIN) {
    h->is_main = true;
    advance(ctx);
    TOKEN_CHEK(ctx->current_token)
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
  h->name = ctx->current_token.token_lexeme.lexeme;
  advance(ctx);
  TOKEN_CHEK(ctx->current_token)

  while (ctx->current_token.token_type == T_OPEN_BRACKET) {
    const int before = h->param_count;
    if ((status = parse_parameters(ctx, &h->params, &h->param_count)) != 0)
      return status;
    int *tmp =
        realloc(h->list_sizes, sizeof(int) * (size_t)(h->list_count + 1));
    if (tmp == NULL)
      return COMPILER_ERR;
    h->list_sizes = tmp;
    h->list_sizes[h->list_count++] = h->param_count - before;
  }

  if (ctx->current_token.token_type == T_DOUBLE_DOT) {
    advance(ctx);
    TOKEN_CHEK(ctx->current_token)
    if ((status = parse_type(ctx, &h->ret_type)) != 0)
      return status;
  }

  return match(ctx, T_ASSIGN);
}

static symtable_entry *register_function(ParserCtx *ctx, const FuncHeader *h,
                                         size_t def_pos) {
  symtable_entry *en = symtable_entry_init();
  en->function = true;
  en->declared = true;
  en->defined = true;
  en->type = h->ret_type;
  en->f_param_count = h->param_count;
  en->f_params = h->params;
  en->f_list_count = h->list_count;
  en->f_list_sizes = h->list_sizes;
  en->body_state = BODY_PENDING;
  en->is_main = h->is_main;
  en->def_pos = def_pos;
  en->body_start = mark_get(ctx);
  smt_insert_global(&ctx->smt, (char *)h->name, en);
  return en;
}

static int analyze_function(ParserCtx *ctx, const char *name,
                            symtable_entry *fentry) {
  int status = 0;
  const TokenType ret_type = fentry->type;
  const f_param *params = fentry->f_params;
  const int param_count = fentry->f_param_count;
  fentry->body_state = BODY_IN_PROGRESS;

  ASTNode *fnode = ast_create_node(AST_FUNC_DEF);
  fnode->is_main = fentry->is_main;
  fnode->data.func_def.name = strdup(name);
  ASTNode *ph = NULL, *pt = NULL;
  for (int i = 0; i < param_count; i++) {
    ASTNode *p = ast_create_node(AST_IDENTIFIER);
    p->data.string_val = strdup(params[i].name);
    p->expr_type = expr_from_kw(params[i].type);
    list_append(&ph, &pt, p);
  }
  fnode->data.func_def.params = ph;

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
    StmtInfo info = {SK_OTHER, EXPR_TYPE_UNKNOWN, false, NULL};
    status = parse_block(ctx, 0, &info, &fnode->data.func_def.body);
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
    if (status == 0 && ret_type == T_ERROR) {
      if (info.returns)
        status = STATIC_SEM_TYPE_ERR;
      else if (info.kind == SK_EXPR) {
        TokenType inf = kw_from_expr_type(info.type);
        if (info.type == EXPR_TYPE_BOOL || inf == T_ERROR)
          status = STATIC_SEM_TYPE_ERR;
        else
          fentry->type = inf;
      } else
        fentry->type = T_KW_UNIT;
    }
  } else {
    ExprType body_type = EXPR_TYPE_UNKNOWN;
    ASTNode *e = NULL;
    status = parse_expression(ctx, &e, &body_type);
    if (status == 0) {
      fnode->data.func_def.body = ast_create_node(AST_BLOCK);
      fnode->data.func_def.body->data.block.statements = e;
    }
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

  if (status == 0) {
    fnode->expr_type = expr_from_kw(fentry->type);
    list_append(&ctx->funcs, &ctx->funcs_tail, fnode);
    fentry->body_state = BODY_DONE;
    fentry->body_end = mark_get(ctx);
  }

  ctx->ret_type = T_ERROR;
  smt_scope_remove(&ctx->smt);
  return status;
}

int resolve_function(ParserCtx *ctx, const char *name, symtable_entry *fentry) {
  const SourceMark saved_mark = mark_get(ctx);
  const TokenType saved_ret = ctx->ret_type;
  const symtable saved_smt = ctx->smt;

  symtable view;
  view.size = saved_smt.size;
  view.top = 0;
  view.table = malloc(sizeof(avl_tree *) * (size_t)view.size);
  if (view.table == NULL)
    return COMPILER_ERR;
  view.table[0] = saved_smt.table[0];

  ctx->smt = view;
  mark_set(ctx, &fentry->body_start);
  int status = analyze_function(ctx, name, fentry);

  free(ctx->smt.table);
  ctx->smt = saved_smt;
  ctx->ret_type = saved_ret;
  mark_set(ctx, &saved_mark);
  return status;
}

static void skip_function_body(ParserCtx *ctx) {
  while (true) {
    TokenType t = ctx->current_token.token_type;
    if (t == T_EOF || t == T_ERROR)
      return;
    if ((t == T_KW_DEF || t == T_FUNC_MAIN) && ctx->line_indent == 0)
      return;
    advance(ctx);
  }
}

static int collect_signatures(ParserCtx *ctx) {
  const SourceMark start = mark_get(ctx);
  int result = 0;

  while (true) {
    TokenType t = ctx->current_token.token_type;
    if (t == T_EOL ||
        (t == T_INDENTS && ctx->current_token.token_indents.count == 0)) {
      advance(ctx);
      continue;
    }
    if ((t != T_KW_DEF && t != T_FUNC_MAIN) || ctx->line_indent != 0)
      break;

    const size_t def_pos = ctx->pos - 1;
    FuncHeader h;
    int status = parse_function_header(ctx, &h);
    if (status != 0) {
      if (status == LEX_ANAL_ERR || status == ERR_SYNTAX)
        result = status;
      break;
    }
    symtable_entry *existing = NULL;
    if (smt_search_global(&ctx->smt, (char *)h.name, &existing)) {
      free_params(h.params, h.param_count);
      free(h.list_sizes);
      break;
    }
    register_function(ctx, &h, def_pos);
    skip_function_body(ctx);
  }

  mark_set(ctx, &start);
  return result;
}

static int parse_defsingle(ParserCtx *ctx) {
  int status;
  const size_t def_pos = ctx->pos - 1;

  FuncHeader h;
  if ((status = parse_function_header(ctx, &h)) != 0)
    return status;

  if (h.is_main) {
    if (h.param_count > 0)
      return STATIC_SEM_ERR;
    if (h.ret_type != T_KW_UNIT)
      return STATIC_SEM_TYPE_ERR;
    ctx->main_count++;
    if (ctx->main_count > 1)
      return REDEF_FN_VAR_ERR;
  }

  symtable_entry *fentry = NULL;
  if (smt_search_global(&ctx->smt, (char *)h.name, &fentry)) {
    if (fentry->body_state == BODY_NONE || fentry->def_pos != def_pos)
      return REDEF_FN_VAR_ERR;
    free_params(h.params, h.param_count);
    free(h.list_sizes);
    if (fentry->body_state == BODY_DONE) {
      mark_set(ctx, &fentry->body_end);
      return 0;
    }
  } else {
    fentry = register_function(ctx, &h, def_pos);
  }

  return analyze_function(ctx, h.name, fentry);
}

static int parse_statement_list(ParserCtx *ctx, int current_indent,
                                StmtInfo *out, ASTNode **list) {
  StmtInfo last = {SK_OTHER, EXPR_TYPE_UNKNOWN, false, NULL};
  ASTNode *head = NULL, *tail = NULL;
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

    StmtInfo cur = {SK_OTHER, EXPR_TYPE_UNKNOWN, false, NULL};
    int status = parse_statement(ctx, current_indent, &cur);
    if (status != 0)
      return status;
    list_append(&head, &tail, cur.node);
    last = cur;
  }
  if (out != NULL)
    *out = last;
  if (list)
    *list = head;
  return 0;
}

static int parse_expr_statement(ParserCtx *ctx, StmtInfo *info) {
  ExprType t = EXPR_TYPE_UNKNOWN;
  ASTNode *ast = NULL;
  int status = parse_expression(ctx, &ast, &t);
  if (status == 0) {
    info->kind = SK_EXPR;
    info->type = t;
    info->node = ast;
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
    return end_of_simple_statement(ctx, parse_valdef(ctx, info));
  case T_KW_VAR:
    return end_of_simple_statement(ctx, parse_vardef(ctx, info));
  case T_KW_IF:
    return parse_if_statement(ctx, current_indent, info);
  case T_KW_WHILE:
    return parse_while_statement(ctx, current_indent, info);
  case T_KW_RETURN:
    return end_of_simple_statement(ctx, parse_return_statement(ctx, info));
  case T_IDENTIFICATOR:
    if (peek_token(ctx).token_type == T_ASSIGN)
      return end_of_simple_statement(ctx, parse_assignment(ctx, info));
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

static int parse_valdef(ParserCtx *ctx, StmtInfo *info) {
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
  ASTNode *init = NULL;
  if ((status = parse_expression(ctx, &init, &expr_type)) != 0)
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

  ASTNode *n = ast_create_node(AST_VAR_DECL);
  n->data.var_assign.identifier = strdup(name);
  n->data.var_assign.expression = init;
  n->expr_type = expr_from_kw(var_type);
  info->node = n;

  return 0;
}

static int parse_vardef(ParserCtx *ctx, StmtInfo *info) {
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
  ASTNode *init = NULL;
  if ((status = parse_expression(ctx, &init, &expr_type)) != 0)
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

  ASTNode *n = ast_create_node(AST_VAR_DECL);
  n->data.var_assign.identifier = strdup(name);
  n->data.var_assign.expression = init;
  n->expr_type = expr_from_kw(var_type);
  info->node = n;

  return 0;
}

static int parse_assignment(ParserCtx *ctx, StmtInfo *info) {
  int status;

  const char *name = ctx->current_token.token_lexeme.lexeme;
  char *id = strdup(name);
  symtable_entry *entry = NULL;
  if (!smt_search(&ctx->smt, (char *)name, &entry)) {
    free(id);
    return UNDEF_FN_ERR;
  }
  if (entry->function) {
    free(id);
    return STATIC_SEM_TYPE_ERR;
  }
  if (!entry->variable) {
    free(id);
    return WRITE_ERR;
  }

  if ((status = match(ctx, T_IDENTIFICATOR)) != 0) {
    free(id);
    return status;
  }
  if ((status = match(ctx, T_ASSIGN)) != 0) {
    free(id);
    return status;
  }

  ExprType expr_type = EXPR_TYPE_UNKNOWN;
  ASTNode *rhs = NULL;
  if ((status = parse_expression(ctx, &rhs, &expr_type)) != 0) {
    free(id);
    return status;
  }

  if (expr_type == EXPR_TYPE_BOOL ||
      !types_compatible(entry->type, expr_type)) {
    free(id);
    return STATIC_SEM_TYPE_ERR;
  }

  ASTNode *n = ast_create_node(AST_ASSIGNMENT);
  n->data.var_assign.identifier = id;
  n->data.var_assign.expression = rhs;
  n->expr_type = expr_from_kw(entry->type);
  info->node = n;

  return 0;
}

static int parse_return_statement(ParserCtx *ctx, StmtInfo *info) {
  int status;
  if ((status = match(ctx, T_KW_RETURN)) != 0)
    return status;
  info->kind = SK_RETURN;
  info->returns = true;

  ASTNode *rn = ast_create_node(AST_RETURN);
  rn->data.return_stmt.expression = NULL;
  info->node = rn;

  TokenType next = ctx->current_token.token_type;
  TOKEN_CHEK(ctx->current_token)
  if (next == T_EOL || next == T_EOF || next == T_SEMICOLON) {
    if (ctx->ret_type != T_ERROR && ctx->ret_type != T_KW_UNIT)
      return STATIC_SEM_TYPE_ERR;
    return 0;
  }

  ExprType expr_type = EXPR_TYPE_UNKNOWN;
  if ((status = parse_expression(ctx, &rn->data.return_stmt.expression,
                                 &expr_type)) != 0)
    return status;

  if (ctx->ret_type == T_ERROR)
    return STATIC_SEM_TYPE_ERR;
  if (ctx->ret_type != T_KW_UNIT && !types_compatible(ctx->ret_type, expr_type))
    return STATIC_SEM_TYPE_ERR;

  return 0;
}

static int parse_condition(ParserCtx *ctx, TokenType terminator,
                           ASTNode **cond) {
  ExprType type = EXPR_TYPE_UNKNOWN;
  int status = parse_expression(ctx, cond, &type);
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
  StmtInfo then_info = {SK_OTHER, EXPR_TYPE_UNKNOWN, false, NULL};
  StmtInfo else_info = {SK_OTHER, EXPR_TYPE_UNKNOWN, false, NULL};
  if ((status = match(ctx, T_KW_IF)) != 0)
    return status;

  ASTNode *n = ast_create_node(AST_IF);
  n->data.if_stmt.else_body = NULL;

  if ((status = parse_condition(ctx, T_KW_THEN, &n->data.if_stmt.condition)) !=
      0)
    return status;

  if ((status = match(ctx, T_KW_THEN)) != 0)
    return status;

  if ((status = parse_block(ctx, current_indent, &then_info,
                            &n->data.if_stmt.body)) != 0)
    return status;

  while (ctx->current_token.token_type == T_EOL)
    advance(ctx);
  if (ctx->current_token.token_type != T_KW_ELSE ||
      ctx->line_indent != current_indent)
    return ERR_SYNTAX;
  if ((status = match(ctx, T_KW_ELSE)) != 0)
    return status;
  if ((status = parse_block(ctx, current_indent, &else_info,
                            &n->data.if_stmt.else_body)) != 0)
    return status;

  info->kind = SK_IF;
  info->returns = then_info.returns && else_info.returns;
  info->node = n;
  return 0;
}

static int parse_while_statement(ParserCtx *ctx, int current_indent,
                                 StmtInfo *info) {
  int status;
  if ((status = match(ctx, T_KW_WHILE)) != 0)
    return status;

  ASTNode *n = ast_create_node(AST_WHILE);

  if ((status = parse_condition(ctx, T_KW_DO, &n->data.while_loop.condition)) !=
      0)
    return status;

  if ((status = match(ctx, T_KW_DO)) != 0)
    return status;

  status = parse_block(ctx, current_indent, NULL, &n->data.while_loop.body);
  if (status == 0)
    info->node = n;
  return status;
}

static int parse_block(ParserCtx *ctx, int expected_indent, StmtInfo *out,
                       ASTNode **blk) {
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

  ASTNode *lst = NULL;
  smt_scope_add(&ctx->smt);
  status = parse_statement_list(ctx, block_indent, out, &lst);
  smt_scope_remove(&ctx->smt);
  if (status == 0 && blk) {
    *blk = ast_create_node(AST_BLOCK);
    (*blk)->data.block.statements = lst;
  }
  return status;
}
