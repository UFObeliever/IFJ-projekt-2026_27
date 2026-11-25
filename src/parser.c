/**
 * @file parser.c
 *
 * Implementace překladače imperativního jazyka IFJ26
 *
 * @brief Syntaktická a sémantická analýza rekurzivním sestupem.
 *
 * Parser čte tokeny z lexikálního analyzátoru a ukládá je do vyrovnávací
 * paměti, aby se mohl vracet k dříve přečteným místům. Nejprve projde
 * hlavičky všech funkcí a rozšiřujících metod a zapíše je do tabulky
 * symbolů, potom analyzuje jejich těla. Příkazy zpracovává rekurzivním
 * sestupem podle LL gramatiky, výrazy předává precedenční analýze
 * (precedence.c). Během analýzy kontroluje typy a vytváří AST, které
 * předá generátoru. Cyklus for převádí na cyklus while.
 *
 * @author Alex Oľšavský (xolsava00)
 * @author Jaroslav Vančura (xvancuj00)
 * @author Matěj Okruhlica (xokruhm00)
 * @author Klára Vantuchová (xvantuk00)
 */

#include "parser.h"
#include "precedence.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Ukončí volající funkci s chybou 1, je-li token lexikální chybou
 *
 * @param token: kontrolovaný token
 */
#define TOKEN_CHECK(token)                                                     \
  do {                                                                         \
    if ((token).token_type == T_ERROR) {                                       \
      return LEX_ANAL_ERR;                                                     \
    }                                                                          \
  } while (0)

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
  int indent;
} FuncHeader;

typedef struct {
  char *name;
  TokenType type;
  int indent;
} ExtRecv;

static void advance(ParserCtx *ctx);
static int match(ParserCtx *ctx, TokenType expected);
static int parse_prolog(ParserCtx *ctx);
static int parse_function_header(ParserCtx *ctx, FuncHeader *h);
static int parse_function_list(ParserCtx *ctx);
static int parse_defsingle(ParserCtx *ctx, const ExtRecv *recv);
static int parse_extension(ParserCtx *ctx);
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
static int parse_body(ParserCtx *ctx, int current_indent, StmtInfo *out,
                      ASTNode **blk);
static int parse_for_statement(ParserCtx *ctx, int current_indent,
                               StmtInfo *info);
static int parse_if_common(ParserCtx *ctx, int current_indent, StmtInfo *info,
                           bool as_expr);
static TokenType kw_from_expr_type(ExprType t);
static bool types_compatible(TokenType declared, ExprType actual);
static Token raw_token(ParserCtx *ctx, size_t idx);

/**
 * @brief Převede typ z klíčového slova na typ výrazu
 *
 * @param t: token typu (Int, Double, String, Unit, Boolean)
 */
static ExprType expr_from_kw(TokenType t) {
  if (t == T_TYPE_BOOLEAN)
    return EXPR_TYPE_BOOL;
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

/**
 * @brief Připojí uzel (případně celý řetězec uzlů) na konec seznamu
 *
 * @param head: začátek seznamu
 * @param tail: poslední prvek seznamu, po připojení se posune na konec
 * @param n: připojovaný uzel, NULL se ignoruje
 */
static void list_append(ASTNode **head, ASTNode **tail, ASTNode *n) {
  if (!n)
    return;
  if (*tail)
    (*tail)->next = n;
  else
    *head = n;
  while (n->next)
    n = n->next;
  *tail = n;
}

/**
 * @brief Vrátí token na daném indexu vyrovnávací paměti
 *
 * @param ctx: kontext parseru
 * @param idx: index tokenu
 */
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
        fprintf(stderr, "Chyba alokace pameti pro tokeny\n");
        exit(COMPILER_ERR);
      }

      ctx->tokens = tmp;
      ctx->tokens_cap = cap;
    }
    ctx->tokens[ctx->tokens_len++] = get_next_token(ctx->lexer);
  }
  return ctx->tokens[idx];
}

/**
 * @brief Uloží aktuální pozici čtení, aby se k ní šlo vrátit
 *
 * @param ctx: kontext parseru
 */
static SourceMark mark_get(const ParserCtx *ctx) {
  SourceMark m = {ctx->pos, ctx->current_token, ctx->line_indent, ctx->at_bol};
  return m;
}

/**
 * @brief Vrátí čtení na dříve uloženou pozici
 *
 * @param ctx: kontext parseru
 * @param m: pozice získaná funkcí mark_get
 */
static void mark_set(ParserCtx *ctx, const SourceMark *m) {
  ctx->pos = m->pos;
  ctx->current_token = m->tok;
  ctx->line_indent = m->line_indent;
  ctx->at_bol = m->at_bol;
}

/**
 * @brief Přečte další token a aktualizuje odsazení aktuálního řádku
 *
 * @param ctx: kontext parseru
 */
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

/**
 * @brief Posune se na další významný token
 *
 * @param ctx: kontext parseru
 */
static void advance(ParserCtx *ctx) {
  ctx->current_token = fetch_token(ctx);
  while (ctx->current_token.token_type == T_WHITESPACE_SEP ||
         ctx->current_token.token_type == T_INDENTS ||
         ctx->current_token.token_type == T_TAB) {
    ctx->current_token = fetch_token(ctx);
  }
}

/**
 * @brief Posune se na další token, odsazení ale nepřeskakuje
 *
 * @param ctx: kontext parseru
 */
static void advance_(ParserCtx *ctx) {
  ctx->current_token = fetch_token(ctx);
  while (ctx->current_token.token_type == T_WHITESPACE_SEP) {
    ctx->current_token = fetch_token(ctx);
  }
}

/**
 * @brief Ověří typ aktuálního tokenu a posune se za něj
 *
 * @param ctx: kontext parseru
 * @param expected: očekávaný typ tokenu
 */
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
  return SYN_ANAL_ERR;
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
  for (size_t i = 0; i < ctx->tokens_len; i++) {
    switch (ctx->tokens[i].token_type) {
    case T_INT_LITERAL:
    case T_DOUBLE_LITERAL:
    case T_STRING_LITERAL:
    case T_IDENTIFICATOR:
      free((void *)ctx->tokens[i].token_lexeme.lexeme);
      break;
    default:
      break;
    }
  }
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
  TOKEN_CHECK(ctx->current_token);
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

/**
 * @brief Zpracuje prolog import ifj26.* (pravidlo 1 LL gramatiky)
 *
 * @param ctx: kontext parseru
 */
static int parse_prolog(ParserCtx *ctx) {
  int status;
  while (ctx->current_token.token_type == T_EOL ||
         ctx->current_token.token_type == T_WHITESPACE_SEP ||
         ctx->current_token.token_type == T_INDENTS) {
    advance(ctx);
    TOKEN_CHECK(ctx->current_token);
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

/**
 * @brief Zpracuje seznam definic funkcí a bloků extension (pravidla 4–7)
 *
 * @param ctx: kontext parseru
 */
static int parse_function_list(ParserCtx *ctx) {
  while (ctx->current_token.token_type == T_EOL ||
         (ctx->current_token.token_type == T_INDENTS &&
          ctx->current_token.token_indents.count == 0)) {
    advance(ctx);
    TOKEN_CHECK(ctx->current_token);
  }
  if (ctx->current_token.token_type == T_EOF) {
    return 0;
  }

  int status;
  if (ctx->current_token.token_type == T_FUNC_MAIN ||
      ctx->current_token.token_type == T_KW_DEF) {
    if (ctx->line_indent != 0)
      return SYN_ANAL_ERR;
    if ((status = parse_defsingle(ctx, NULL)) != 0)
      return status;
    return parse_function_list(ctx);
  }
  if (ctx->current_token.token_type == T_EXTENSION) {
    if (ctx->line_indent != 0)
      return SYN_ANAL_ERR;
    if ((status = parse_extension(ctx)) != 0)
      return status;
    return parse_function_list(ctx);
  }
  fprintf(stderr, "Neocekavany token %d mimo definici funkce\n",
          ctx->current_token.token_type);
  return SYN_ANAL_ERR;
}

/**
 * @brief Zpracuje jméno typu (pravidla 20–24)
 *
 * @param ctx: kontext parseru
 * @param out: načtený typ jako token klíčového slova
 */
static int parse_type(ParserCtx *ctx, TokenType *out) {
  switch (ctx->current_token.token_type) {
  case T_KW_INT:
  case T_KW_DOUBLE:
  case T_KW_STRING:
  case T_KW_UNIT:
  case T_KW_BOOLEAN:
    *out = ctx->current_token.token_type;
    advance(ctx);
    TOKEN_CHECK(ctx->current_token);
    return 0;
  default:
    fprintf(stderr, "Ocekavan nazev typu\n");
    return SYN_ANAL_ERR;
  }
}

/**
 * @brief Zpracuje jeden seznam parametrů v závorkách (pravidla 11–17)
 *
 * @param ctx: kontext parseru
 * @param params: pole parametrů funkce, podle potřeby se zvětší
 * @param count: počet parametrů v poli, zvýší se o načtené parametry
 */
static int parse_parameters(ParserCtx *ctx, f_param **params, int *count) {
  int status;

  if ((status = match(ctx, T_OPEN_BRACKET)) != 0)
    return status;
  if (ctx->current_token.token_type == T_CLOSE_BRACKET)
    return match(ctx, T_CLOSE_BRACKET);

  while (true) {
    if (ctx->current_token.token_type == T_EOL) {
      advance(ctx);
      TOKEN_CHECK(ctx->current_token);
    }
    if (ctx->current_token.token_type != T_IDENTIFICATOR &&
        ctx->current_token.token_type != T_CLOSE_BRACKET)
      return SYN_ANAL_ERR;
    if (ctx->current_token.token_type == T_CLOSE_BRACKET) {
      advance(ctx);
      TOKEN_CHECK(ctx->current_token);
      return 0;
    }
    const char *pname = ctx->current_token.token_lexeme.lexeme;
    for (int i = 0; i < *count; i++) {
      if (strcmp((*params)[i].name, pname) == 0)
        return REDEF_FN_VAR_ERR;
    }
    advance(ctx);
    TOKEN_CHECK(ctx->current_token);

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
      TOKEN_CHECK(ctx->current_token);
      if (ctx->current_token.token_type == T_CLOSE_BRACKET)
        return SYN_ANAL_ERR;
      continue;
    }
    break;
  }
  if (ctx->current_token.token_type == T_EOL) {
    advance(ctx);
    TOKEN_CHECK(ctx->current_token);
  }
  return match(ctx, T_CLOSE_BRACKET);
}

/**
 * @brief Uvolní pole parametrů včetně jejich jmen
 *
 * @param params: pole parametrů
 * @param count: počet parametrů v poli
 */
static void free_params(f_param *params, int count) {
  for (int i = 0; i < count; i++)
    free(params[i].name);
  free(params);
}

/**
 * @brief Zpracuje hlavičku funkce až po znak = (pravidla 8–12, 18–19)
 *
 * @param ctx: kontext parseru
 * @param h: načtená hlavička
 */
static int parse_function_header(ParserCtx *ctx, FuncHeader *h) {
  int status;
  h->name = NULL;
  h->params = NULL;
  h->param_count = 0;
  h->list_count = 0;
  h->list_sizes = NULL;
  h->ret_type = T_ERROR;
  h->is_main = false;
  h->indent = ctx->line_indent;

  if (ctx->current_token.token_type == T_FUNC_MAIN) {
    h->is_main = true;
    advance(ctx);
    TOKEN_CHECK(ctx->current_token);
    if (ctx->current_token.token_type == T_EOL) {
      advance(ctx);
      TOKEN_CHECK(ctx->current_token);
    }
    if (match(ctx, T_KW_DEF) != 0)
      return SYN_ANAL_ERR;
  } else if (ctx->current_token.token_type == T_KW_DEF) {
    advance(ctx);
    TOKEN_CHECK(ctx->current_token);
  } else {
    return SYN_ANAL_ERR;
  }

  if (ctx->current_token.token_type != T_IDENTIFICATOR)
    return SYN_ANAL_ERR;
  h->name = ctx->current_token.token_lexeme.lexeme;
  advance(ctx);
  TOKEN_CHECK(ctx->current_token);

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
    TOKEN_CHECK(ctx->current_token);
    if ((status = parse_type(ctx, &h->ret_type)) != 0)
      return status;
  }

  return match(ctx, T_ASSIGN);
}

/**
 * @brief Vloží funkci do globální tabulky symbolů
 *
 * @param ctx: kontext parseru
 * @param h: hlavička funkce, záznam převezme její parametry
 * @param def_pos: pozice tokenu def v proudu tokenů
 */
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
  en->def_indent = h->indent;
  en->body_start = mark_get(ctx);
  smt_insert_global(&ctx->smt, (char *)h->name, en);
  return en;
}

/**
 * @brief Říká, zda poslední příkaz bloku dává hodnotu
 *
 * @param s: informace o posledním příkazu bloku
 */
static bool is_value_stmt(const StmtInfo *s) {
  return s->kind == SK_EXPR ||
         (s->kind == SK_IF && s->type != EXPR_TYPE_UNKNOWN);
}

/**
 * @brief Ověří, že za tělem funkce správně začíná další definice
 *
 * @param ctx: kontext parseru
 * @param base_indent: odsazení řádku s def (v bloku extension nenulové)
 */
static bool body_ends_here(const ParserCtx *ctx, int base_indent) {
  TokenType t = ctx->current_token.token_type;
  if (t == T_EOF)
    return true;
  if (ctx->line_indent == 0)
    return t == T_FUNC_MAIN || t == T_KW_DEF || t == T_EXTENSION;
  return (base_indent > 0 && ctx->line_indent == base_indent && t == T_KW_DEF);
}

/**
 * @brief Analyzuje tělo funkce a vytvoří její uzel AST
 *
 * @param ctx: kontext parseru, aktuální token je za znakem =
 * @param name: jméno funkce
 * @param fentry: záznam funkce v tabulce symbolů
 */
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
    status =
        parse_block(ctx, fentry->def_indent, &info, &fnode->data.func_def.body);
    if (status == 0) {
      TokenType t = ctx->current_token.token_type;
      if (t == T_ERROR)
        status = LEX_ANAL_ERR;
      else if (!body_ends_here(ctx, fentry->def_indent))
        status = SYN_ANAL_ERR;
    }
    if (status == 0 && ret_type != T_ERROR && ret_type != T_KW_UNIT &&
        !info.returns &&
        !(is_value_stmt(&info) && types_compatible(ret_type, info.type)))
      status = STATIC_SEM_TYPE_ERR;
    if (status == 0 && ret_type == T_ERROR) {
      if (info.returns)
        status = STATIC_SEM_TYPE_ERR;
      else if (is_value_stmt(&info)) {
        TokenType inf = kw_from_expr_type(info.type);
        if (inf == T_ERROR)
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
                                                          : SYN_ANAL_ERR;
    if (status == 0 && ret_type != T_ERROR && ret_type != T_KW_UNIT &&
        !types_compatible(ret_type, body_type))
      status = STATIC_SEM_TYPE_ERR;
    if (status == 0 && ret_type == T_ERROR) {
      TokenType inferred = kw_from_expr_type(body_type);
      if (inferred == T_ERROR)
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

/**
 * @brief Přeskočí tělo funkce bez analýzy
 *
 * @param ctx: kontext parseru
 * @param base_indent: odsazení řádku s def
 */
static void skip_function_body(ParserCtx *ctx, int base_indent) {
  while (true) {
    TokenType t = ctx->current_token.token_type;
    if (t == T_EOF || t == T_ERROR)
      return;
    if ((t == T_KW_DEF || t == T_FUNC_MAIN || t == T_EXTENSION) &&
        ctx->line_indent <= base_indent)
      return;
    advance(ctx);
  }
}

/**
 * @brief Vrátí jméno typu příjemce rozšiřující metody
 *
 * @param t: token typu
 */
static const char *ext_type_name(TokenType t) {
  switch (t) {
  case T_KW_INT:
    return "Int";
  case T_KW_DOUBLE:
    return "Double";
  case T_KW_STRING:
    return "String";
  case T_KW_UNIT:
    return "Unit";
  case T_KW_BOOLEAN:
    return "Boolean";
  default:
    return NULL;
  }
}

/**
 * @brief Zpracuje hlavičku extension (jméno: Typ) (pravidlo 27)
 *
 * @param ctx: kontext parseru, aktuální token je extension
 * @param r: načtený příjemce, jméno je třeba uvolnit
 */
static int parse_extension_header(ParserCtx *ctx, ExtRecv *r) {
  int status;
  f_param *ps = NULL;
  int cnt = 0;
  r->name = NULL;

  advance(ctx);
  TOKEN_CHECK(ctx->current_token);
  if ((status = parse_parameters(ctx, &ps, &cnt)) != 0) {
    free_params(ps, cnt);
    return status;
  }
  if (cnt != 1) {
    free_params(ps, cnt);
    return SYN_ANAL_ERR;
  }
  r->name = ps[0].name;
  r->type = ps[0].type;
  free(ps);

  if (ctx->current_token.token_type != T_EOL) {
    free(r->name);
    r->name = NULL;
    return (ctx->current_token.token_type == T_ERROR) ? LEX_ANAL_ERR
                                                      : SYN_ANAL_ERR;
  }
  while (ctx->current_token.token_type == T_EOL)
    advance(ctx);
  if (ctx->current_token.token_type != T_KW_DEF || ctx->line_indent == 0) {
    free(r->name);
    r->name = NULL;
    return (ctx->current_token.token_type == T_ERROR) ? LEX_ANAL_ERR
                                                      : SYN_ANAL_ERR;
  }
  r->indent = ctx->line_indent;
  return 0;
}

/**
 * @brief Převede rozšiřující metodu na běžnou funkci
 *
 * @param h: hlavička metody, upraví se jméno a parametry
 * @param r: příjemce z hlavičky extension
 */
static int apply_receiver(FuncHeader *h, const ExtRecv *r) {
  for (int i = 0; i < h->param_count; i++) {
    if (strcmp(h->params[i].name, r->name) == 0)
      return REDEF_FN_VAR_ERR;
  }
  const char *tn = ext_type_name(r->type);
  if (tn == NULL)
    return SYN_ANAL_ERR;
  size_t len = strlen(tn) + 1 + strlen(h->name) + 1;
  char *mangled = malloc(len);
  f_param *ps = malloc(sizeof(f_param) * (size_t)(h->param_count + 1));
  if (mangled == NULL || ps == NULL) {
    free(mangled);
    free(ps);
    return COMPILER_ERR;
  }
  snprintf(mangled, len, "%s$%s", tn, h->name);
  ps[0].name = strdup(r->name);
  ps[0].type = r->type;
  for (int i = 0; i < h->param_count; i++)
    ps[i + 1] = h->params[i];
  free(h->params);
  h->params = ps;
  h->param_count++;
  h->name = mangled;
  return 0;
}

/**
 * @brief Při prvním průchodu zaregistruje všechny metody bloku extension
 *
 * @param ctx: kontext parseru, aktuální token je extension
 */
static int collect_extension(ParserCtx *ctx) {
  ExtRecv r;
  int status = parse_extension_header(ctx, &r);
  if (status != 0)
    return status;

  while (true) {
    while (ctx->current_token.token_type == T_EOL)
      advance(ctx);
    if (ctx->current_token.token_type != T_KW_DEF ||
        ctx->line_indent != r.indent)
      break;

    const size_t def_pos = ctx->pos - 1;
    FuncHeader h;
    status = parse_function_header(ctx, &h);
    if (status == 0)
      status = apply_receiver(&h, &r);
    if (status != 0)
      break;
    symtable_entry *existing = NULL;
    if (smt_search_global(&ctx->smt, (char *)h.name, &existing)) {
      free_params(h.params, h.param_count);
      free(h.list_sizes);
      break;
    }
    register_function(ctx, &h, def_pos);
    skip_function_body(ctx, r.indent);
  }
  free(r.name);
  return status;
}

/**
 * @brief První průchod: zaregistruje hlavičky všech funkcí a metod
 *
 * @param ctx: kontext parseru
 */
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
    if (t == T_EXTENSION && ctx->line_indent == 0) {
      int st = collect_extension(ctx);
      if (st == LEX_ANAL_ERR || st == SYN_ANAL_ERR) {
        result = st;
        break;
      }
      if (st != 0)
        break;
      continue;
    }
    if ((t != T_KW_DEF && t != T_FUNC_MAIN) || ctx->line_indent != 0)
      break;

    const size_t def_pos = ctx->pos - 1;
    FuncHeader h;
    int status = parse_function_header(ctx, &h);
    if (status != 0) {
      if (status == LEX_ANAL_ERR || status == SYN_ANAL_ERR)
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
    skip_function_body(ctx, 0);
  }

  mark_set(ctx, &start);
  return result;
}

/**
 * @brief Zpracuje definici funkce nebo metody (pravidla 8–10)
 *
 * @param ctx: kontext parseru
 * @param recv: příjemce u metody bloku extension, jinak NULL
 */
static int parse_defsingle(ParserCtx *ctx, const ExtRecv *recv) {
  int status;
  const size_t def_pos = ctx->pos - 1;

  FuncHeader h;
  if ((status = parse_function_header(ctx, &h)) != 0)
    return status;
  if (recv != NULL && (status = apply_receiver(&h, recv)) != 0)
    return status;

  if (h.is_main) {
    status = 0;
    if (h.param_count > 0)
      status = STATIC_SEM_ERR;
    else if (h.ret_type != T_KW_UNIT)
      status = STATIC_SEM_TYPE_ERR;
    else if (++ctx->main_count > 1)
      status = REDEF_FN_VAR_ERR;
    if (status != 0) {
      free_params(h.params, h.param_count);
      free(h.list_sizes);
      return status;
    }
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

/**
 * @brief Zpracuje blok extension s jednou či více metodami (pravidla 27–30)
 *
 * @param ctx: kontext parseru, aktuální token je extension
 */
static int parse_extension(ParserCtx *ctx) {
  ExtRecv r;
  int status = parse_extension_header(ctx, &r);
  if (status != 0)
    return status;

  while (true) {
    if ((status = parse_defsingle(ctx, &r)) != 0)
      break;
    while (ctx->current_token.token_type == T_EOL)
      advance(ctx);
    TokenType t = ctx->current_token.token_type;
    if (t == T_ERROR) {
      status = LEX_ANAL_ERR;
      break;
    }
    if (t == T_EOF || ctx->line_indent < r.indent)
      break;
    if (ctx->line_indent > r.indent || t != T_KW_DEF) {
      status = SYN_ANAL_ERR;
      break;
    }
  }
  free(r.name);
  return status;
}

/**
 * @brief Zpracuje posloupnost příkazů bloku (pravidla 31–37)
 *
 * @param ctx: kontext parseru
 * @param current_indent: odsazení příkazů bloku
 * @param out: informace o posledním příkazu bloku, může být NULL
 * @param list: seznam uzlů AST příkazů, může být NULL
 */
static int parse_statement_list(ParserCtx *ctx, int current_indent,
                                StmtInfo *out, ASTNode **list) {
  StmtInfo last = {SK_OTHER, EXPR_TYPE_UNKNOWN, false, NULL};
  ASTNode *head = NULL, *tail = NULL;
  while (true) {
    if (ctx->current_token.token_type == T_EOL ||
        (ctx->current_token.token_type == T_INDENTS &&
         ctx->current_token.token_indents.count == current_indent)) {
      advance(ctx);
      TOKEN_CHECK(ctx->current_token);
      continue;
    }
    TOKEN_CHECK(ctx->current_token);

    if (ctx->current_token.token_type == T_EOF)
      break;
    if (ctx->line_indent < current_indent)
      break;
    if (ctx->line_indent > current_indent)
      return SYN_ANAL_ERR;

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

/**
 * @brief Zpracuje výraz jako příkaz (pravidlo 41)
 *
 * @param ctx: kontext parseru
 * @param info: informace o příkazu
 */
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

/**
 * @brief Ověří ukončení jednoduchého příkazu
 *
 * @param ctx: kontext parseru
 * @param status: výsledek analýzy příkazu, chyba se jen předá dál
 */
static int end_of_simple_statement(ParserCtx *ctx, int status) {
  if (status != 0)
    return status;
  TOKEN_CHECK(ctx->current_token);
  if (ctx->current_token.token_type == T_SEMICOLON) {
    advance(ctx);
    TOKEN_CHECK(ctx->current_token);
    return 0;
  }
  if (ctx->current_token.token_type != T_EOL &&
      ctx->current_token.token_type != T_EOF &&
      ctx->current_token.token_type != T_KW_ELSE)
    return SYN_ANAL_ERR;
  return 0;
}

/**
 * @brief Zpracuje jeden příkaz podle prvního tokenu (pravidla 38–45)
 *
 * @param ctx: kontext parseru
 * @param current_indent: odsazení řádku s příkazem
 * @param info: informace o příkazu
 */
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
  case T_KW_FOR:
    return parse_for_statement(ctx, current_indent, info);
  case T_IDENTIFICATOR:
    if (peek_token(ctx).token_type == T_ASSIGN)
      return end_of_simple_statement(ctx, parse_assignment(ctx, info));
    return end_of_simple_statement(ctx, parse_expr_statement(ctx, info));
  case T_INT_LITERAL:
  case T_DOUBLE_LITERAL:
  case T_STRING_LITERAL:
  case T_KW_TRUE:
  case T_KW_FALSE:
  case T_NOT:
  case T_MINUS_SIGN:
  case T_OPEN_BRACKET:
    return end_of_simple_statement(ctx, parse_expr_statement(ctx, info));
  default:
    return SYN_ANAL_ERR;
  }
}

/**
 * @brief Převede typ výrazu na typ jako token klíčového slova
 *
 * @param t: typ výrazu
 */
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
  case EXPR_TYPE_BOOL:
    return T_TYPE_BOOLEAN;
  default:
    return T_ERROR;
  }
}

/**
 * @brief Ověří, zda lze hodnotu daného typu uložit do deklarovaného typu
 *
 * @param declared: deklarovaný typ
 * @param actual: typ hodnoty, EXPR_TYPE_UNKNOWN je kompatibilní vždy
 */
static bool types_compatible(TokenType declared, ExprType actual) {
  if (actual == EXPR_TYPE_UNKNOWN) {
    return true;
  }
  if (declared == T_TYPE_BOOLEAN)
    return actual == EXPR_TYPE_BOOL;
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

/**
 * @brief Zpracuje definici nemodifikovatelné proměnné val (pravidlo 38)
 *
 * @param ctx: kontext parseru
 * @param info: informace o příkazu
 */
static int parse_valdef(ParserCtx *ctx, StmtInfo *info) {
  int status;
  if ((status = match(ctx, T_KW_VAL)) != 0)
    return status;
  TOKEN_CHECK(ctx->current_token);
  if (ctx->current_token.token_type != T_IDENTIFICATOR)
    return SYN_ANAL_ERR;
  const char *name = ctx->current_token.token_lexeme.lexeme;
  advance(ctx);
  TOKEN_CHECK(ctx->current_token);

  TokenType declared = T_ERROR;
  bool has_type = false;
  if (ctx->current_token.token_type == T_DOUBLE_DOT) {
    advance(ctx);
    TOKEN_CHECK(ctx->current_token);
    if ((status = parse_type(ctx, &declared)) != 0)
      return status;
    has_type = true;
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

/**
 * @brief Zpracuje definici modifikovatelné proměnné var (pravidlo 39)
 *
 * @param ctx: kontext parseru
 * @param info: informace o příkazu
 */
static int parse_vardef(ParserCtx *ctx, StmtInfo *info) {
  int status;
  if ((status = match(ctx, T_KW_VAR)) != 0)
    return status;
  if (ctx->current_token.token_type != T_IDENTIFICATOR)
    return SYN_ANAL_ERR;
  const char *name = ctx->current_token.token_lexeme.lexeme;
  advance(ctx);
  TOKEN_CHECK(ctx->current_token);

  TokenType declared = T_ERROR;
  bool has_type = false;
  if (ctx->current_token.token_type == T_DOUBLE_DOT) {
    advance(ctx);
    TOKEN_CHECK(ctx->current_token);
    if ((status = parse_type(ctx, &declared)) != 0)
      return status;
    has_type = true;
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

/**
 * @brief Zpracuje přiřazení id = výraz (pravidlo 40)
 *
 * @param ctx: kontext parseru
 * @param info: informace o příkazu
 */
static int parse_assignment(ParserCtx *ctx, StmtInfo *info) {
  int status;

  const char *name = ctx->current_token.token_lexeme.lexeme;
  char *id = strdup(name);
  symtable_entry *entry = NULL;
  if (!smt_search(&ctx->smt, (char *)name, &entry)) {
    free(id);
    return UNDEF_FN_ERR;
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

  if (!types_compatible(entry->type, expr_type)) {
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

/**
 * @brief Zpracuje návrat z funkce return (pravidla 42, 48–49)
 *
 * @param ctx: kontext parseru
 * @param info: informace o příkazu
 */
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
  TOKEN_CHECK(ctx->current_token);
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

/**
 * @brief Zpracuje podmínku if nebo while
 *
 * @param ctx: kontext parseru
 * @param terminator: token, kterým podmínka končí (then nebo do)
 * @param cond: uzel AST podmínky
 */
static int parse_condition(ParserCtx *ctx, TokenType terminator,
                           ASTNode **cond) {
  ExprType type = EXPR_TYPE_UNKNOWN;
  int status = parse_expression(ctx, cond, &type);
  if (status != 0)
    return status;
  if (ctx->current_token.token_type != terminator)
    return SYN_ANAL_ERR;
  if (type != EXPR_TYPE_BOOL && type != EXPR_TYPE_UNKNOWN)
    return STATIC_SEM_TYPE_ERR;
  return 0;
}

/**
 * @brief Zjistí, zda větev if dává hodnotu, a jakého typu
 *
 * @param b: informace o posledním příkazu větve
 * @param as_expr: true pro if jako výraz
 * @param t: typ hodnoty větve
 */
static bool branch_value(const StmtInfo *b, bool as_expr, ExprType *t) {
  switch (b->kind) {
  case SK_EXPR:
    *t = b->type;
    return true;
  case SK_IF:
    if (b->type != EXPR_TYPE_UNKNOWN) {
      *t = b->type;
      return true;
    }
    if (as_expr && !b->returns) {
      *t = EXPR_TYPE_UNIT;
      return true;
    }
    return false;
  case SK_OTHER:
    if (!as_expr)
      return false;
    *t = EXPR_TYPE_UNIT;
    return true;
  default:
    return false;
  }
}

/**
 * @brief Sjednotí typy dvou větví if
 *
 * @param a: typ větve then
 * @param b: typ větve else
 * @param out: společný typ
 */
static bool unify_types(ExprType a, ExprType b, ExprType *out) {
  if (a == b || b == EXPR_TYPE_UNKNOWN) {
    *out = a;
    return true;
  }
  if (a == EXPR_TYPE_UNKNOWN) {
    *out = b;
    return true;
  }
  if ((a == EXPR_TYPE_INT && b == EXPR_TYPE_DOUBLE) ||
      (a == EXPR_TYPE_DOUBLE && b == EXPR_TYPE_INT)) {
    *out = EXPR_TYPE_DOUBLE;
    return true;
  }
  return false;
}

/**
 * @brief Zpracuje větev if: blok, nebo jednořádkový příkaz či výraz
 *
 * @param ctx: kontext parseru
 * @param current_indent: odsazení řádku s if
 * @param out: informace o posledním příkazu větve
 * @param blk: uzel AST_BLOCK větve
 * @param as_expr: true pro if jako výraz
 */
static int parse_branch(ParserCtx *ctx, int current_indent, StmtInfo *out,
                        ASTNode **blk, bool as_expr) {
  if (!as_expr || ctx->current_token.token_type == T_EOL)
    return parse_body(ctx, current_indent, out, blk);

  ExprType t = EXPR_TYPE_UNKNOWN;
  ASTNode *e = NULL;
  int status = parse_expression(ctx, &e, &t);
  if (status != 0)
    return status;
  *blk = ast_create_node(AST_BLOCK);
  (*blk)->data.block.statements = e;
  out->kind = SK_EXPR;
  out->type = t;
  out->node = e;
  return 0;
}

/**
 * @brief Společná část if jako příkazu a if jako výrazu (pravidla 43, 50–52,
 * 66–70)
 *
 * @param ctx: kontext parseru, aktuální token je if
 * @param current_indent: odsazení řádku s if
 * @param info: informace o příkazu
 * @param as_expr: true pro if jako výraz
 */
static int parse_if_common(ParserCtx *ctx, int current_indent, StmtInfo *info,
                           bool as_expr) {
  int status;
  StmtInfo then_info = {SK_OTHER, EXPR_TYPE_UNKNOWN, false, NULL};
  StmtInfo else_info = {SK_OTHER, EXPR_TYPE_UNKNOWN, false, NULL};
  if ((status = match(ctx, T_KW_IF)) != 0)
    return status;

  ASTNode *n = ast_create_node(AST_IF);
  n->data.if_stmt.else_body = NULL;
  n->expr_type = EXPR_TYPE_UNKNOWN;

  if ((status = parse_condition(ctx, T_KW_THEN, &n->data.if_stmt.condition)) !=
      0)
    return status;

  if ((status = match(ctx, T_KW_THEN)) != 0)
    return status;

  bool inline_then = ctx->current_token.token_type != T_EOL;
  if ((status = parse_branch(ctx, current_indent, &then_info,
                             &n->data.if_stmt.body, as_expr)) != 0)
    return status;

  bool has_else = inline_then && ctx->current_token.token_type == T_KW_ELSE;
  if (!has_else) {
    while (ctx->current_token.token_type == T_EOL)
      advance(ctx);
    has_else = ctx->current_token.token_type == T_KW_ELSE &&
               ctx->line_indent == current_indent;
  }
  if (has_else) {
    if ((status = match(ctx, T_KW_ELSE)) != 0)
      return status;
    if (ctx->current_token.token_type == T_KW_IF) {
      if ((status =
               parse_if_common(ctx, current_indent, &else_info, as_expr)) != 0)
        return status;
      n->data.if_stmt.else_body = ast_create_node(AST_BLOCK);
      n->data.if_stmt.else_body->data.block.statements = else_info.node;
    } else if ((status = parse_branch(ctx, current_indent, &else_info,
                                      &n->data.if_stmt.else_body, as_expr)) !=
               0) {
      return status;
    }
  } else if (as_expr) {
    return SYN_ANAL_ERR;
  }

  ExprType tt = EXPR_TYPE_UNKNOWN, et = EXPR_TYPE_UNKNOWN;
  ExprType rt = EXPR_TYPE_UNKNOWN;
  bool value = false;
  if (has_else) {
    bool tv = branch_value(&then_info, as_expr, &tt);
    bool ev = branch_value(&else_info, as_expr, &et);
    if (tv && ev) {
      value = unify_types(tt, et, &rt);
      if (!value && as_expr)
        return STATIC_SEM_TYPE_ERR;
    } else if (tv && else_info.returns) {
      value = true;
      rt = tt;
    } else if (ev && then_info.returns) {
      value = true;
      rt = et;
    }
  }

  n->expr_type = value ? rt : EXPR_TYPE_UNKNOWN;
  info->kind = SK_IF;
  info->type = n->expr_type;
  info->returns = then_info.returns && has_else && else_info.returns;
  info->node = n;
  return 0;
}

/**
 * @brief Zpracuje podmíněný příkaz if
 *
 * @param ctx: kontext parseru, aktuální token je if
 * @param current_indent: odsazení řádku s if
 * @param info: informace o příkazu
 */
static int parse_if_statement(ParserCtx *ctx, int current_indent,
                              StmtInfo *info) {
  return parse_if_common(ctx, current_indent, info, false);
}

/**
 * @brief Po if zakončeném blokem vrátí čtení na konec řádku
 *
 * @param ctx: kontext parseru
 */
static void rewind_to_eol(ParserCtx *ctx) {
  if (ctx->tokens_len == 0 || ctx->pos == 0)
    return;
  size_t i = ctx->pos - 1;
  if (i >= ctx->tokens_len)
    i = ctx->tokens_len - 1;
  size_t first = 0;
  bool found = false;
  while (i > 0) {
    TokenType t = ctx->tokens[i - 1].token_type;
    if (t == T_EOL) {
      first = i - 1;
      found = true;
    } else if (t != T_INDENTS && t != T_WHITESPACE_SEP && t != T_TAB) {
      break;
    }
    i--;
  }
  if (!found)
    return;
  ctx->pos = first + 1;
  ctx->current_token = ctx->tokens[first];
  ctx->line_indent = 0;
  ctx->at_bol = true;
}

int parse_if_expression(ParserCtx *ctx, ASTNode **out, int *out_type) {
  StmtInfo info = {SK_OTHER, EXPR_TYPE_UNKNOWN, false, NULL};
  int status = parse_if_common(ctx, ctx->line_indent, &info, true);
  if (status != 0)
    return status;
  rewind_to_eol(ctx);
  *out = info.node;
  *out_type = (int)info.type;
  return 0;
}

/**
 * @brief Zpracuje cyklus while (pravidlo 44)
 *
 * @param ctx: kontext parseru, aktuální token je while
 * @param current_indent: odsazení řádku s while
 * @param info: informace o příkazu
 */
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

  status = parse_body(ctx, current_indent, NULL, &n->data.while_loop.body);
  if (status == 0)
    info->node = n;
  return status;
}

/**
 * @brief Zpracuje odsazený blok (pravidlo 31)
 *
 * @param ctx: kontext parseru, aktuální token je konec řádku
 * @param expected_indent: odsazení nadřazeného příkazu
 * @param out: informace o posledním příkazu bloku, může být NULL
 * @param blk: vytvořený uzel AST_BLOCK, může být NULL
 */
static int parse_block(ParserCtx *ctx, int expected_indent, StmtInfo *out,
                       ASTNode **blk) {
  int status;

  if ((status = match(ctx, T_EOL)) != 0)
    return status;
  TOKEN_CHECK(ctx->current_token);
  if (ctx->current_token.token_type == T_TAB) {
    return LEX_ANAL_ERR;
  }
  if (ctx->current_token.token_type != T_INDENTS)
    return SYN_ANAL_ERR;

  if (ctx->current_token.token_indents.count <= expected_indent)
    return SYN_ANAL_ERR;

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

/**
 * @brief Zpracuje tělo větve nebo cyklu: blok, nebo jeden příkaz (pravidla
 * 53–54)
 *
 * @param ctx: kontext parseru
 * @param current_indent: odsazení řádku s nadřazeným příkazem
 * @param out: informace o posledním příkazu těla, může být NULL
 * @param blk: vytvořený uzel AST_BLOCK
 */
static int parse_body(ParserCtx *ctx, int current_indent, StmtInfo *out,
                      ASTNode **blk) {
  if (ctx->current_token.token_type == T_EOL)
    return parse_block(ctx, current_indent, out, blk);

  StmtInfo info = {SK_OTHER, EXPR_TYPE_UNKNOWN, false, NULL};
  smt_scope_add(&ctx->smt);
  int status = parse_statement(ctx, current_indent, &info);
  smt_scope_remove(&ctx->smt);
  if (status != 0)
    return status;
  if (out)
    *out = info;
  *blk = ast_create_node(AST_BLOCK);
  (*blk)->data.block.statements = info.node;
  return 0;
}

/**
 * @brief Ověří, zda je aktuální token identifikátor s daným lexémem
 *
 * @param ctx: kontext parseru
 * @param word: hledaný lexém
 */
static bool is_word(const ParserCtx *ctx, const char *word) {
  return ctx->current_token.token_type == T_IDENTIFICATOR &&
         strcmp(ctx->current_token.token_lexeme.lexeme, word) == 0;
}

/**
 * @brief Vytvoří uzel identifikátoru typu Int
 *
 * @param name: jméno proměnné
 */
static ASTNode *int_ident(const char *name) {
  ASTNode *n = ast_create_node(AST_IDENTIFIER);
  n->data.string_val = strdup(name);
  n->expr_type = EXPR_TYPE_INT;
  return n;
}

/**
 * @brief Vytvoří uzel binární operace s daným typem výsledku
 *
 * @param op: operátor
 * @param l: levý operand
 * @param r: pravý operand
 * @param t: typ výsledku
 */
static ASTNode *int_binop(int op, ASTNode *l, ASTNode *r, ExprType t) {
  ASTNode *n = ast_make_binop(op, l, r);
  n->expr_type = t;
  return n;
}

/**
 * @brief Vytvoří celočíselný literál
 *
 * @param v: hodnota
 */
static ASTNode *int_literal(int v) {
  ASTNode *n = ast_make_int(v);
  n->expr_type = EXPR_TYPE_INT;
  return n;
}

/**
 * @brief Vytvoří definici nebo přiřazení proměnné typu Int
 *
 * @param type: AST_VAR_DECL, nebo AST_ASSIGNMENT
 * @param name: jméno proměnné
 * @param e: přiřazovaný výraz
 */
static ASTNode *int_store(ASTNodeType type, const char *name, ASTNode *e) {
  ASTNode *n = ast_create_node(type);
  n->data.var_assign.identifier = strdup(name);
  n->data.var_assign.expression = e;
  n->expr_type = EXPR_TYPE_INT;
  return n;
}

/**
 * @brief Zabalí seznam příkazů do uzlu AST_BLOCK
 *
 * @param statements: první příkaz seznamu
 */
static ASTNode *block_of(ASTNode *statements) {
  ASTNode *b = ast_create_node(AST_BLOCK);
  b->data.block.statements = statements;
  return b;
}

/**
 * @brief Zpracuje výraz a ověří jeho typ
 *
 * @param ctx: kontext parseru
 * @param want: požadovaný typ výrazu
 * @param e: uzel AST výrazu
 */
static int parse_typed_expr(ParserCtx *ctx, ExprType want, ASTNode **e) {
  ExprType t = EXPR_TYPE_UNKNOWN;
  int status = parse_expression(ctx, e, &t);
  if (status == 0 && t != want && t != EXPR_TYPE_UNKNOWN)
    return STATIC_SEM_TYPE_ERR;
  return status;
}

/**
 * @brief Vrátí operand výrazu bez unárních operátorů
 *
 * @param e: výraz
 */
static ASTNode *strip_unary(ASTNode *e) {
  while (e->type == AST_UNARY_OP)
    e = e->data.unary_op.operand;
  return e;
}

/**
 * @brief Zpracuje cyklus for a převede jej na cyklus while (pravidla 45, 55–65)
 *
 * @param ctx: kontext parseru, aktuální token je for
 * @param current_indent: odsazení řádku s for
 * @param info: informace o příkazu, uzel je řetězec příkazů
 */
static int parse_for_statement(ParserCtx *ctx, int current_indent,
                               StmtInfo *info) {
  int status;
  advance(ctx);
  TOKEN_CHECK(ctx->current_token);

  bool multiline = ctx->current_token.token_type == T_EOL;
  int gen_indent = current_indent;
  if (multiline) {
    while (ctx->current_token.token_type == T_EOL)
      advance(ctx);
    TOKEN_CHECK(ctx->current_token);
    if (ctx->line_indent <= current_indent)
      return SYN_ANAL_ERR;
    gen_indent = ctx->line_indent;
  }

  if (ctx->current_token.token_type != T_IDENTIFICATOR)
    return SYN_ANAL_ERR;
  const char *var = ctx->current_token.token_lexeme.lexeme;
  advance(ctx);
  if ((status = match(ctx, T_LT)) != 0 ||
      (status = match(ctx, T_MINUS_SIGN)) != 0)
    return status;

  ASTNode *from = NULL, *to = NULL, *step = NULL;
  if ((status = parse_typed_expr(ctx, EXPR_TYPE_INT, &from)) != 0)
    return status;
  bool inclusive = is_word(ctx, "to");
  if (!inclusive && !is_word(ctx, "until"))
    return SYN_ANAL_ERR;
  advance(ctx);
  if ((status = parse_typed_expr(ctx, EXPR_TYPE_INT, &to)) != 0)
    return status;
  if (is_word(ctx, "by")) {
    advance(ctx);
    if ((status = parse_typed_expr(ctx, EXPR_TYPE_INT, &step)) != 0)
      return status;
    ASTNode *lit = strip_unary(step);
    if (lit->type == AST_LITERAL_INT && lit->data.int_val == 0)
      return ERR_OTHERS;
  }

  smt_scope_add(&ctx->smt);
  symtable_entry *ve = symtable_entry_init();
  ve->type = T_KW_INT;
  ve->declared = true;
  ve->defined = true;
  ve->value = true;
  smt_insert(&ctx->smt, (char *)var, ve);

  ASTNode **guards = NULL;
  int guard_count = 0;
  while (status == 0) {
    if (multiline) {
      if (ctx->current_token.token_type != T_EOL) {
        status = SYN_ANAL_ERR;
        break;
      }
      while (ctx->current_token.token_type == T_EOL)
        advance(ctx);
      if (ctx->current_token.token_type == T_KW_DO &&
          ctx->line_indent == current_indent)
        break;
      if (ctx->current_token.token_type != T_KW_IF ||
          ctx->line_indent != gen_indent) {
        status = SYN_ANAL_ERR;
        break;
      }
    } else if (ctx->current_token.token_type == T_KW_DO) {
      break;
    } else if (ctx->current_token.token_type != T_KW_IF) {
      status = SYN_ANAL_ERR;
      break;
    }
    advance(ctx);
    ASTNode *g = NULL;
    if ((status = parse_typed_expr(ctx, EXPR_TYPE_BOOL, &g)) != 0)
      break;
    ASTNode **tmp =
        realloc(guards, sizeof(*guards) * (size_t)(guard_count + 1));
    if (tmp == NULL) {
      status = COMPILER_ERR;
      break;
    }
    guards = tmp;
    guards[guard_count++] = g;
  }

  ASTNode *body = NULL;
  if (status == 0)
    status = match(ctx, T_KW_DO);
  if (status == 0)
    status = parse_body(ctx, current_indent, NULL, &body);
  smt_scope_remove(&ctx->smt);
  if (status != 0) {
    free(guards);
    return status;
  }

  char n_i[32], n_end[32], n_step[32];
  int id = ctx->for_count++;
  snprintf(n_i, sizeof n_i, "$for%d_i", id);
  snprintf(n_end, sizeof n_end, "$for%d_end", id);
  snprintf(n_step, sizeof n_step, "$for%d_step", id);

  ASTNode *core = body;
  for (int i = guard_count - 1; i >= 0; i--)
    core = block_of(ast_make_if(guards[i], core, NULL));
  free(guards);

  ASTNode *incr = int_store(
      AST_ASSIGNMENT, n_i,
      int_binop(T_PLUS_SIGN, int_ident(n_i), int_ident(n_step), EXPR_TYPE_INT));
  ASTNode *loop_var = int_store(AST_VAR_DECL, var, int_ident(n_i));
  loop_var->next = core->data.block.statements;
  free(core);
  ASTNode *last = loop_var;
  while (last->next)
    last = last->next;
  last->next = incr;

  ASTNode *diff =
      int_binop(T_MINUS_SIGN, int_ident(n_i), int_ident(n_end), EXPR_TYPE_INT);
  ASTNode *cond =
      int_binop(inclusive ? T_LTE : T_LT,
                int_binop(T_MULT_SIGN, diff, int_ident(n_step), EXPR_TYPE_INT),
                int_literal(0), EXPR_TYPE_BOOL);
  ASTNode *loop = ast_make_while(cond, block_of(loop_var));

  ASTNode *decl_i = int_store(AST_VAR_DECL, n_i, from);
  ASTNode *decl_end = int_store(AST_VAR_DECL, n_end, to);
  ASTNode *decl_step =
      int_store(AST_VAR_DECL, n_step, step ? step : int_literal(1));
  decl_i->next = decl_end;
  decl_end->next = decl_step;
  decl_step->next = loop;

  if (step && strip_unary(step)->type != AST_LITERAL_INT) {
    ASTNode *empty = ast_make_string(strdup(""));
    empty->expr_type = EXPR_TYPE_STRING;
    empty->next = int_literal(1);
    empty->next->next = int_literal(0);
    ASTNode *fail = ast_create_node(AST_FUNC_CALL);
    fail->data.func_call.name = strdup("subStr");
    fail->data.func_call.arguments = empty;
    fail->expr_type = EXPR_TYPE_STRING;
    ASTNode *zero =
        int_binop(T_EQUAL, int_ident(n_step), int_literal(0), EXPR_TYPE_BOOL);
    ASTNode *check = ast_make_if(zero, block_of(fail), NULL);
    decl_step->next = check;
    check->next = loop;
  }

  info->kind = SK_OTHER;
  info->node = decl_i;
  return 0;
}
