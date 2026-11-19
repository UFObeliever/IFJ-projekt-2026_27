#include "precedence.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TYPE_STACK_MAX 256
#define PREC_STACK_MAX                                                         \
  ((int)(sizeof(((PrecedenceStack *)0)->items) / sizeof(TokenType)))
#define T_UNARY_MINUS_MARK ((TokenType)1000)
#define CONSUME_TOKEN_GUARD(parser)                                            \
  do {                                                                         \
    Token tok = consume_token((parser));                                       \
    if (tok.token_type == T_ERROR) {                                           \
      return LEX_ANAL_ERR;                                                     \
    }                                                                          \
  } while (0);

typedef struct {
  ExprType items[TYPE_STACK_MAX];
  int top;
} TypeStack;

static void tstack_init(TypeStack *s) { s->top = -1; }
static bool tstack_push(TypeStack *s, ExprType t) {
  if (s->top >= TYPE_STACK_MAX - 1) {
    DEBUG_LOG("[DEBUG] tstack_push overflow!\n");
    return false;
  }
  DEBUG_LOG("[DEBUG] tstack_push: pushed type %d, new top = %d\n", t, s->top);
  s->items[++s->top] = t;
  return true;
}
static bool tstack_pop(TypeStack *s, ExprType *out) {
  if (s->top < 0) {
    DEBUG_LOG("[DEBUG] tstack_pop underflow!\n");
    return false;
  }
  DEBUG_LOG("[DEBUG] tstack_pop: popped type %d, new top = %d\n", *out, s->top);
  *out = s->items[s->top--];
  return true;
}
static bool tstack_peek(TypeStack *s, ExprType *out) {
  if (s->top < 0)
    return false;
  *out = s->items[s->top];
  return true;
}

typedef struct {
  ASTNode *items[TYPE_STACK_MAX];
  int top;
} ASTStack;

static void ast_stack_init(ASTStack *s) { s->top = -1; }
static bool ast_stack_push(ASTStack *s, ASTNode *node) {
  if (s->top >= TYPE_STACK_MAX - 1)
    return false;
  s->items[++s->top] = node;
  return true;
}
static bool ast_stack_pop(ASTStack *s, ASTNode **out) {
  if (s->top < 0)
    return false;
  *out = s->items[s->top--];
  return true;
}

static void stack_clear(PrecedenceStack *s) {
  s->top = -1;
  DEBUG_LOG("[DEBUG] precedence stack cleared\n");
}

static bool stack_push(PrecedenceStack *s, TokenType t) {
  if (s->top >= PREC_STACK_MAX - 1) {
    DEBUG_LOG("[DEBUG] precedence stack overflow!\n");
    return false;
  }
  DEBUG_LOG("[DEBUG] stack_push: token %d pushed, new stack top = %d\n", t,
            s->top);
  s->items[++s->top] = t;
  return true;
}

static bool stack_pop(PrecedenceStack *s) {
  if (s->top < 0) {
    DEBUG_LOG("[DEBUG] precedence stack underflow on pop!\n");
    return false;
  }
  DEBUG_LOG("[DEBUG] stack_pop: removing token %d from top = %d\n",
            s->items[s->top], s->top);
  s->top--;
  return true;
}

static ExprType expr_type_from_token(TokenType t) {
  switch (t) {
  case T_INT_LITERAL:
    return EXPR_TYPE_INT;
  case T_DOUBLE_LITERAL:
    return EXPR_TYPE_DOUBLE;
  case T_STRING_LITERAL:
    return EXPR_TYPE_STRING;
  default:
    return EXPR_TYPE_UNKNOWN;
  }
}

static ExprType expr_type_from_kw(TokenType t) {
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

static const PrecedenceAction precedence_table[EX_COUNT][EX_COUNT] = {
    /* STACK            +        -           *              /        == != < <=
       >           >=          is          (               )        , $ operand
     */
    /* + */ {PREC_REDUCE, PREC_REDUCE, PREC_SHIFT, PREC_SHIFT, PREC_REDUCE,
             PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE,
             PREC_REDUCE, PREC_SHIFT, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE,
             PREC_REDUCE},
    /* - */
    {PREC_REDUCE, PREC_REDUCE, PREC_SHIFT, PREC_SHIFT, PREC_REDUCE, PREC_REDUCE,
     PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE,
     PREC_SHIFT, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE},
    /* * */
    {PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE,
     PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE,
     PREC_REDUCE, PREC_SHIFT, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE,
     PREC_REDUCE},
    /* / */
    {PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE,
     PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE,
     PREC_REDUCE, PREC_SHIFT, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE,
     PREC_REDUCE},
    /* == */
    {PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_REDUCE, PREC_REDUCE,
     PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE,
     PREC_SHIFT, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE},
    /* != */
    {PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_REDUCE, PREC_REDUCE,
     PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE,
     PREC_SHIFT, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE},
    /* < */
    {PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT,
     PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE,
     PREC_SHIFT, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE},
    /* <= */
    {PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT,
     PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE,
     PREC_SHIFT, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE},
    /* > */
    {PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT,
     PREC_SHIFT, PREC_SHIFT, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_SHIFT,
     PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE},
    /* >= */
    {PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT,
     PREC_SHIFT, PREC_SHIFT, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_SHIFT,
     PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE},
    /* is */
    {PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT,
     PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_REDUCE, PREC_SHIFT,
     PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE},
    /* ( */
    {PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT,
     PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT,
     PREC_EQUAL, PREC_SHIFT, PREC_ERROR, PREC_SHIFT},
    /* ) */
    {PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE,
     PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE,
     PREC_REDUCE, PREC_ERROR, PREC_REDUCE, PREC_REDUCE, PREC_ERROR, PREC_ERROR},
    /* ,  */
    {PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT,
     PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT, PREC_SHIFT,
     PREC_ERROR, PREC_EQUAL, PREC_ERROR, PREC_SHIFT},
    /* $ */
    {PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE,
     PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE,
     PREC_REDUCE, PREC_ERROR, PREC_REDUCE, PREC_ERROR, PREC_ERROR, PREC_ERROR},
    /* op */
    {PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE,
     PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE, PREC_REDUCE,
     PREC_REDUCE, PREC_ERROR, PREC_REDUCE, PREC_ERROR, PREC_ERROR, PREC_ERROR}};

static Error check_identifier_in_expr(ParserCtx *parser, const Token *token,
                                      ExprType *type) {
  symtable_entry *entry = NULL;

  if (!smt_search(&parser->smt, (char *)token->token_lexeme.lexeme, &entry)) {
    DEBUG_LOG("[DEBUG] check_identifier_in_expr1: identifier '%s' not found "
              "(UNDEF_FN_ERR)\n",
              (char *)token->token_lexeme.lexeme);
    return UNDEF_FN_ERR;
  }
  if (entry->function) {
    DEBUG_LOG("[DEBUG] check_identifier_in_expr2: identifier '%s' not found "
              "(UNDEF_FN_ERR)\n",
              (char *)token->token_lexeme.lexeme);
    return STATIC_SEM_TYPE_ERR;
  }

  *type = expr_type_from_kw(entry->type);
  DEBUG_LOG("[DEBUG] check_identifier_in_expr3: identifier '%s' not found "
            "(UNDEF_FN_ERR)\n",
            (char *)token->token_lexeme.lexeme);
  return SUCCESS;
}

ExprColumn token_to_column(TokenType t) {
  switch (t) {
  case T_PLUS_SIGN:
    return EX_PLUS;
  case T_MINUS_SIGN:
    return EX_MINUS;
  case T_MULT_SIGN:
    return EX_MUL;
  case T_DIVIDE:
    return EX_DIV;
  case T_EQUAL:
    return EX_EQ;
  case T_NOT_EQUAL:
    return EX_NEQ;
  case T_LT:
    return EX_LT;
  case T_LTE:
    return EX_LTE;
  case T_GT:
    return EX_GT;
  case T_GTE:
    return EX_GTE;
  case T_OPEN_BRACKET:
    return EX_LPAREN;
  case T_CLOSE_BRACKET:
    return EX_RPAREN;
  case T_COMMA:
    return EX_COMMA;
  case T_IDENTIFICATOR:
  case T_INT_LITERAL:
  case T_DOUBLE_LITERAL:
  case T_STRING_LITERAL:
    return EX_OPERAND;
  default:
    return EX_COUNT;
  }
}

TokenType top_terminal(PrecedenceStack *stack) {
  for (int i = stack->top; i >= 0; i--) {
    TokenType t = stack->items[i];
    ExprColumn col = token_to_column(t);
    if (col != EX_OPERAND && col != EX_COUNT) {
      return t;
    }
  }
  return T_EOF;
}

static bool is_operand_token(TokenType t) {
  switch (t) {
  case T_IDENTIFICATOR:
  case T_INT_LITERAL:
  case T_DOUBLE_LITERAL:
  case T_STRING_LITERAL:
    return true;
  default:
    return false;
  }
}

static bool is_open_BRACKET(TokenType t) {
  return (t == T_OPEN_BRACKET || t == T_OPEN_BRACKET);
}

static bool is_close_BRACKET(TokenType t) {
  return (t == T_CLOSE_BRACKET || t == T_CLOSE_BRACKET);
}

static bool is_operator_token(TokenType t) {
  switch (t) {
  case T_PLUS_SIGN:
  case T_MINUS_SIGN:
  case T_MULT_SIGN:
  case T_DIVIDE:
  case T_EQUAL:
  case T_NOT_EQUAL:
  case T_LT:
  case T_LTE:
  case T_GT:
  case T_GTE:
    return true;
  default:
    return false;
  }
}

static bool is_numeric(ExprType t) {
  return t == EXPR_TYPE_INT || t == EXPR_TYPE_DOUBLE;
}

static ExprType int_or_double(ExprType l, ExprType r);

static Error reduce_unary(PrecedenceStack *st, TypeStack *ts, ASTStack *as) {
  while (st->top >= 0 && st->items[st->top] == T_UNARY_MINUS_MARK) {
    ExprType t = EXPR_TYPE_UNKNOWN;
    ASTNode *n = NULL;
    st->top--;
    if (!tstack_pop(ts, &t) || !ast_stack_pop(as, &n))
      return SYN_ANAL_ERR;
    if (t != EXPR_TYPE_UNKNOWN && !is_numeric(t))
      return STATIC_SEM_TYPE_ERR;
    // ja upraveno
    ASTNode *u = ast_make_unary(T_MINUS_SIGN, n);
    if (u)
      u->expr_type = t;
    if (!tstack_push(ts, t) || !ast_stack_push(as, u))
      return COMPILER_ERR;
    /*
    if (!tstack_push(ts, t) ||
        !ast_stack_push(as, ast_make_unary(T_MINUS_SIGN, n)))
      return COMPILER_ERR;*/
  }
  return SUCCESS;
}

static ExprType int_or_double(ExprType l, ExprType r) {
  if (l == EXPR_TYPE_DOUBLE) {
    return EXPR_TYPE_DOUBLE;
  }
  if (r == EXPR_TYPE_DOUBLE) {
    return EXPR_TYPE_DOUBLE;
  }
  return EXPR_TYPE_INT;
}

Error apply_operator(TokenType op, TypeStack *tstack, ASTStack *ast_stack) {
  ExprType right_type = EXPR_TYPE_UNKNOWN, left_type = EXPR_TYPE_UNKNOWN;
  ASTNode *right_node, *left_node;
  DEBUG_LOG("[DEBUG] apply_operator called for operator token %d\n", op);

  if (!tstack_pop(tstack, &right_type) || !tstack_pop(tstack, &left_type)) {
    DEBUG_LOG("[DEBUG] apply_operator: tstack_pop failed (SYN_ANAL_ERR)\n");
    return SYN_ANAL_ERR;
  }
  if (!ast_stack_pop(ast_stack, &right_node) ||
      !ast_stack_pop(ast_stack, &left_node)) {
    DEBUG_LOG("[DEBUG] apply_operator: ast_stack_pop failed (SYN_ANAL_ERR)\n");
    return SYN_ANAL_ERR;
  }

  ExprType result = EXPR_TYPE_UNKNOWN;

  bool known =
      left_type != EXPR_TYPE_UNKNOWN && right_type != EXPR_TYPE_UNKNOWN;

  switch (op) {
  case T_PLUS_SIGN:
    if (is_numeric(left_type) && is_numeric(right_type))
      result = int_or_double(left_type, right_type);
    else if (left_type == EXPR_TYPE_STRING && right_type == EXPR_TYPE_STRING)
      result = EXPR_TYPE_STRING;
    else if (known) {
      DEBUG_LOG(
          "[DEBUG] apply_operator (+) type mismatch (STATIC_SEM_TYPE_ERR)\n");
      return STATIC_SEM_TYPE_ERR;
    }
    break;
  case T_MINUS_SIGN:
  case T_MULT_SIGN:
  case T_DIVIDE:
    if (is_numeric(left_type) && is_numeric(right_type))
      result = int_or_double(left_type, right_type);
    else if (known) {
      DEBUG_LOG("[DEBUG] apply_operator (arithmetic) type mismatch "
                "(STATIC_SEM_TYPE_ERR)\n");
      return STATIC_SEM_TYPE_ERR;
    }
    break;
  case T_LT:
  case T_LTE:
  case T_GT:
  case T_GTE:
    if (known) {
      bool both_num = is_numeric(left_type) && is_numeric(right_type);
      bool both_str =
          left_type == EXPR_TYPE_STRING && right_type == EXPR_TYPE_STRING;
      if (!both_num && !both_str) {
        DEBUG_LOG("[DEBUG] apply_operator (relational) type mismatch "
                  "(STATIC_SEM_TYPE_ERR)\n");
        return STATIC_SEM_TYPE_ERR;
      }
    }
    result = EXPR_TYPE_BOOL;
    break;
  case T_EQUAL:
  case T_NOT_EQUAL:
    if (known) {
      bool both_num = is_numeric(left_type) && is_numeric(right_type);
      if (!both_num && left_type != right_type) {
        DEBUG_LOG("[DEBUG] apply_operator (equality) type mismatch "
                  "(STATIC_SEM_TYPE_ERR)\n");
        return STATIC_SEM_TYPE_ERR;
      }
    }
    result = EXPR_TYPE_BOOL;
    break;
  default:
    result = EXPR_TYPE_UNKNOWN;
    break;
  }

  DEBUG_LOG("[DEBUG] apply_operator result type = %d\n", result);

  if (!tstack_push(tstack, result)) {
    return COMPILER_ERR;
  }

  ASTNode *binop_node = ast_make_binop(op, left_node, right_node);
  // ja pridano radek 438
  if (!ast_stack_push(ast_stack, binop_node)) {
    return COMPILER_ERR;
  }
  binop_node->expr_type = result;

  return SUCCESS;
}

static bool arg_type_ok(TokenType param, ExprType actual) {
  if (actual == EXPR_TYPE_UNKNOWN)
    return true;
  if (actual == EXPR_TYPE_BOOL)
    return false;
  switch (param) {
  case T_KW_INT:
    return actual == EXPR_TYPE_INT;
  case T_KW_DOUBLE:
    return actual == EXPR_TYPE_INT || actual == EXPR_TYPE_DOUBLE;
  case T_KW_STRING:
    return actual == EXPR_TYPE_STRING;
  case T_KW_UNIT:
    return actual == EXPR_TYPE_UNIT;
  default:
    return true;
  }
}

static void skip_eols(ParserCtx *parser) {
  while (parser->current_token.token_type == T_EOL)
    (void)consume_token(parser);
}

static int list_size(const symtable_entry *fn, int i) {
  if (i >= fn->f_list_count)
    return -1;
  if (fn->f_list_sizes == NULL)
    return fn->f_param_count;
  return fn->f_list_sizes[i];
}

static Error finish_call(ParserCtx *parser, const char *name,
                         symtable_entry *fn, ASTNode *recv, ExprType *out_type,
                         ASTNode **out_node);

static Error parse_call(ParserCtx *parser, ExprType *out_type,
                        ASTNode **out_node) {
  const char *name = parser->current_token.token_lexeme.lexeme;
  symtable_entry *fn = NULL;
  if (!smt_search(&parser->smt, (char *)name, &fn) || !fn->function) {
    DEBUG_LOG("DEBUG: lexeme not found!: %s\n", name);
    return UNDEF_FN_ERR;
  }
  CONSUME_TOKEN_GUARD(parser)
  return finish_call(parser, name, fn, NULL, out_type, out_node);
}

// pro přepis z a.toString() -> toString(a)
static Error finish_call(ParserCtx *parser, const char *name,
                         symtable_entry *fn, ASTNode *recv, ExprType *out_type,
                         ASTNode **out_node) {
  Error deferred = SUCCESS;
  ASTNode *head = recv, *tail = recv; // ja
  int lists = 0;
  int offset = recv ? 1 : 0;

  while (parser->current_token.token_type == T_OPEN_BRACKET) {
    const int expected = list_size(fn, lists);
    int argc = 0;
    CONSUME_TOKEN_GUARD(parser)
    skip_eols(parser);

    if (parser->current_token.token_type == T_ERROR)
      return LEX_ANAL_ERR;
    if (parser->current_token.token_type == T_CLOSE_BRACKET) {
      CONSUME_TOKEN_GUARD(parser)
    } else {
      while (true) {
        ExprType at = EXPR_TYPE_UNKNOWN;
        // ja
        ASTNode *arg = NULL;
        Error err = parse_expression(parser, &arg, &at);
        if (err != SUCCESS)
          return err;
        if (tail)
          tail->next = arg;
        else
          head = arg;
        tail = arg;
        if (argc >= expected ||
            !arg_type_ok(fn->f_params[offset + argc].type, at))
          deferred = STATIC_SEM_ERR;
        argc++;

        skip_eols(parser);

        TokenType t = parser->current_token.token_type;
        if (t == T_ERROR)
          return LEX_ANAL_ERR;
        if (t == T_COMMA) {
          CONSUME_TOKEN_GUARD(parser)
          t = parser->current_token.token_type;
          if (t == T_ERROR)
            return LEX_ANAL_ERR;
          if (t == T_CLOSE_BRACKET)
            return SYN_ANAL_ERR;
          skip_eols(parser);
          if (parser->current_token.token_type == T_ERROR)
            return LEX_ANAL_ERR;
          if (parser->current_token.token_type == T_CLOSE_BRACKET) {
            CONSUME_TOKEN_GUARD(parser)
            break;
          }
          continue;
        }
        if (t == T_CLOSE_BRACKET) {
          CONSUME_TOKEN_GUARD(parser)
          break;
        }
        return SYN_ANAL_ERR;
      }
    }

    if (argc != expected)
      deferred = STATIC_SEM_ERR;
    if (expected > 0)
      offset += expected;
    lists++;
  }

  if (lists != fn->f_list_count)
    deferred = STATIC_SEM_ERR;
  if (deferred != SUCCESS)
    return deferred;
  if (fn->type == T_ERROR && fn->body_state == BODY_PENDING) {
    int status = resolve_function(parser, name, fn);
    if (status != SUCCESS)
      return (Error)status;
  }
  if (fn->type == T_ERROR)
    return STATIC_SEM_TYPE_ERR;
  *out_type = expr_type_from_kw(fn->type);
  ASTNode *node = ast_create_node(AST_FUNC_CALL);
  if (node != NULL) {
    node->data.func_call.name = strdup(name);
    node->data.func_call.arguments = head; // bylo NULL
    node->expr_type = *out_type;
  }
  *out_node = node;
  return SUCCESS;
}
// ja
static char *strip_us(const char *s) {
  char *r = malloc(strlen(s) + 1), *p = r;
  if (!r)
    return NULL;
  for (; *s; s++)
    if (*s != '_')
      *p++ = *s;
  *p = 0;
  return r;
}

static bool is_builtin_member(const char *m, ExprType t) {
  if (!strcmp(m, "toString"))
    return true;
  if (!strcmp(m, "toInt") || !strcmp(m, "floor"))
    return t == EXPR_TYPE_DOUBLE;
  if (!strcmp(m, "length"))
    return t == EXPR_TYPE_STRING;
  return false;
}

// klíč rozšiřující metody v tabulce symbolů jako je"Typ$metoda"
static char *ext_method_key(ExprType t, const char *m) {
  const char *tn;
  switch (t) {
  case EXPR_TYPE_INT:
    tn = "Int";
    break;
  case EXPR_TYPE_DOUBLE:
    tn = "Double";
    break;
  case EXPR_TYPE_STRING:
    tn = "String";
    break;
  case EXPR_TYPE_UNIT:
    tn = "Unit";
    break;
  default:
    return NULL;
  }
  size_t len = strlen(tn) + 1 + strlen(m) + 1;
  char *key = malloc(len);
  if (key != NULL)
    snprintf(key, len, "%s$%s", tn, m);
  return key;
}

static Error apply_member(ParserCtx *p, TypeStack *ts, ASTStack *as) {
  while (p->current_token.token_type == T_DOT) {
    CONSUME_TOKEN_GUARD(p)
    if (p->current_token.token_type == T_ERROR)
      return LEX_ANAL_ERR;
    if (p->current_token.token_type != T_IDENTIFICATOR)
      return SYN_ANAL_ERR;

    const char *m = p->current_token.token_lexeme.lexeme;
    ExprType t = EXPR_TYPE_UNKNOWN;
    ASTNode *recv = NULL;
    if (!tstack_pop(ts, &t) || !ast_stack_pop(as, &recv))
      return SYN_ANAL_ERR;

    if (!is_builtin_member(m, t)) {
      char *key = ext_method_key(t, m);
      symtable_entry *ext = NULL;
      if (key != NULL && smt_search_global(&p->smt, key, &ext) &&
          ext->function) {
        CONSUME_TOKEN_GUARD(p)
        ExprType rt = EXPR_TYPE_UNKNOWN;
        ASTNode *call = NULL;
        Error e = finish_call(p, key, ext, recv, &rt, &call);
        free(key);
        if (e != SUCCESS)
          return e;
        if (!tstack_push(ts, rt) || !ast_stack_push(as, call))
          return COMPILER_ERR;
        continue;
      }
      free(key);
    }

    const char *fn;
    ExprType res;
    bool known = (t != EXPR_TYPE_UNKNOWN);
    if (!strcmp(m, "toString")) {
      fn = "toStr";
      res = EXPR_TYPE_STRING;
      if (t == EXPR_TYPE_BOOL)
        return STATIC_SEM_TYPE_ERR;
    } else if (!strcmp(m, "toInt")) {
      fn = "toInt";
      res = EXPR_TYPE_INT;
      if (known && t != EXPR_TYPE_DOUBLE)
        return STATIC_SEM_TYPE_ERR;
    } else if (!strcmp(m, "floor")) {
      fn = "$floor";
      res = EXPR_TYPE_DOUBLE;
      if (known && t != EXPR_TYPE_DOUBLE)
        return STATIC_SEM_TYPE_ERR;
    } else if (!strcmp(m, "length")) {
      fn = "strLength";
      res = EXPR_TYPE_INT;
      if (known && t != EXPR_TYPE_STRING)
        return STATIC_SEM_TYPE_ERR;
    } else {
      return UNDEF_FN_ERR;
    }

    ASTNode *call = ast_create_node(AST_FUNC_CALL);
    if (!call)
      return COMPILER_ERR;
    call->data.func_call.name = strdup(fn);
    call->data.func_call.arguments = recv;
    call->expr_type = res;
    if (!tstack_push(ts, res) || !ast_stack_push(as, call))
      return COMPILER_ERR;

    CONSUME_TOKEN_GUARD(p)
  }
  return SUCCESS;
}

static Error parse_expression_core(ParserCtx *parser, ASTNode **out_ast,
                                   ExprType *out_type);

Error parse_expression(ParserCtx *parser, ASTNode **out_ast,
                       ExprType *out_type) {
  PrecedenceStack saved = parser->stack;
  Error err = parse_expression_core(parser, out_ast, out_type);
  parser->stack = saved;
  return err;
}

Error parse_expression_core(ParserCtx *parser, ASTNode **out_ast,
                            ExprType *out_type) {
  DEBUG_LOG("[DEBUG] === parse_expression START ===\n");
  stack_clear(&parser->stack);

  TypeStack tstack;
  tstack_init(&tstack);

  ASTStack ast_stack;
  ast_stack_init(&ast_stack);

  bool expect_operand = true;
  int BRACKET_depth = 0;
  const int start_indent = parser->line_indent;

  Token lookahead = look_next_eol(parser);

  DEBUG_LOG("[DEBUG] initial lookahead token_type = %d\n",
            lookahead.token_type);

  while (true) {
    if (lookahead.token_type == T_ERROR)
      return LEX_ANAL_ERR;
    DEBUG_LOG(
        "[DEBUG] loop iteration: lookahead token_type = %d, expect_operand "
        "= %d, BRACKET_depth = %d, stack_top = %d\n",
        lookahead.token_type, expect_operand, BRACKET_depth, parser->stack.top);
    if (lookahead.token_type == T_EOL && BRACKET_depth > 0) {
      CONSUME_TOKEN_GUARD(parser);
      lookahead = look_next_eol(parser);
      if (lookahead.token_type != T_EOL && lookahead.token_type != T_EOF &&
          parser->line_indent <= start_indent)
        return SYN_ANAL_ERR;
      continue;
    }
    bool is_end_token = false;
    switch (lookahead.token_type) {
    case T_EOL:
    case T_EOF:
    case T_COMMA:
    case T_CLOSE_BRACKET:
      if (BRACKET_depth == 0)
        is_end_token = true;
      break;
    default:
      break;
    }

    if (is_end_token) {
      DEBUG_LOG("[DEBUG] reached end token at depth 0, expect_operand = %d\n",
                expect_operand);
      if (expect_operand)
        return SYN_ANAL_ERR;
      break;
    }

    if (is_operand_token(lookahead.token_type) && expect_operand) {
      DEBUG_LOG("[DEBUG] branch: OPERAND token %d\n", lookahead.token_type);
      ASTNode *leaf = NULL;
      ExprType t = expr_type_from_token(lookahead.token_type);
      bool is_call = false;
      if (lookahead.token_type == T_IDENTIFICATOR) {
        symtable_entry *sym = NULL;
        is_call = peek_token(parser).token_type == T_OPEN_BRACKET ||
                  (smt_search(&parser->smt,
                              (char *)lookahead.token_lexeme.lexeme, &sym) &&
                   sym->function);
      }

      if (is_call) {
        Error err = parse_call(parser, &t, &leaf);
        if (err != SUCCESS)
          return err;
      } else if (lookahead.token_type == T_IDENTIFICATOR) {
        Error err = check_identifier_in_expr(parser, &lookahead, &t);
        if (err != SUCCESS)
          return err;

        leaf = ast_create_node(AST_IDENTIFIER);
        leaf->data.string_val = strdup((char *)lookahead.token_lexeme.lexeme);
      } else if (lookahead.token_type == T_INT_LITERAL) {
        char *c = strip_us(lookahead.token_lexeme.lexeme);
        leaf = ast_make_int(c ? atoi(c) : 0);
        free(c);
      } else if (lookahead.token_type == T_DOUBLE_LITERAL) {
        char *c = strip_us(lookahead.token_lexeme.lexeme);
        leaf = ast_make_double(c ? strtod(c, NULL) : 0.0);
        free(c);
      } else if (lookahead.token_type == T_STRING_LITERAL) {
        leaf = ast_make_string(strdup((char *)lookahead.token_lexeme.lexeme));
      } else {
        leaf = ast_create_node(AST_LITERAL_NULL);
      }
      if (leaf)
        leaf->expr_type = t; // ja
      if (!tstack_push(&tstack, t) || !ast_stack_push(&ast_stack, leaf)) {
        return COMPILER_ERR;
      }
      if (!is_call) {
        CONSUME_TOKEN_GUARD(parser);
      }
      expect_operand = false;
      Error merr = apply_member(parser, &tstack, &ast_stack);
      if (merr != SUCCESS)
        return merr;
      Error uerr = reduce_unary(&parser->stack, &tstack, &ast_stack);
      if (uerr != SUCCESS)
        return uerr;

    } else if (is_open_BRACKET(lookahead.token_type) && expect_operand) {
      DEBUG_LOG("[DEBUG] branch: OPEN BRACKET (\n");
      stack_push(&parser->stack, lookahead.token_type);
      CONSUME_TOKEN_GUARD(parser);
      BRACKET_depth++;

    } else if (lookahead.token_type == T_MINUS_SIGN && expect_operand) {
      DEBUG_LOG("[DEBUG] branch: UNARY MINUS\n");
      stack_push(&parser->stack, T_UNARY_MINUS_MARK);
      CONSUME_TOKEN_GUARD(parser);

    } else if (is_close_BRACKET(lookahead.token_type)) {
      DEBUG_LOG("[DEBUG] branch: CLOSE BRACKET ), expect_operand = %d\n",
                expect_operand);
      if (expect_operand) {
        if (parser->stack.top >= 0 &&
            is_open_BRACKET(parser->stack.items[parser->stack.top])) {
          DEBUG_LOG("[DEBUG] found empty brackets ()\n");
          stack_pop(&parser->stack);
          BRACKET_depth--;

          ExprType t = EXPR_TYPE_UNIT;
          ASTNode *unit_node = ast_create_node(AST_LITERAL_UNIT);

          unit_node->expr_type = EXPR_TYPE_UNIT; // ja

          if (!tstack_push(&tstack, t) ||
              !ast_stack_push(&ast_stack, unit_node)) {
            return COMPILER_ERR;
          }
          CONSUME_TOKEN_GUARD(parser);
          expect_operand = false;
          Error merr = apply_member(parser, &tstack, &ast_stack);
          if (merr != SUCCESS)
            return merr;
          Error uerr = reduce_unary(&parser->stack, &tstack, &ast_stack);
          if (uerr != SUCCESS)
            return uerr;
        } else {
          DEBUG_LOG("[DEBUG] empty bracket check failed, stack top "
                    "mismatch (SYN_ANAL_ERR)\n");
          return SYN_ANAL_ERR;
        }
      } else {
        bool found = false;
        while (parser->stack.top >= 0) {
          TokenType top = parser->stack.items[parser->stack.top];
          DEBUG_LOG(
              "[DEBUG] closing bracket cleanup loop: inspecting stack top "
              "token = %d\n",
              top);
          if (is_open_BRACKET(top)) {
            found = true;
            stack_pop(&parser->stack);
            BRACKET_depth--;
            break;
          } else if (is_operator_token(top)) {
            stack_pop(&parser->stack);
            Error err = apply_operator(top, &tstack, &ast_stack);
            if (err != SUCCESS)
              return err;
          } else {
            DEBUG_LOG(
                "[DEBUG] unexpected token in bracket cleanup (SYN_ANAL_ERR)\n");
            return SYN_ANAL_ERR;
          }
        }
        if (!found) {
          DEBUG_LOG("[DEBUG] open bracket not found for closing bracket "
                    "(SYN_ANAL_ERR)\n");
          return SYN_ANAL_ERR;
        }
        CONSUME_TOKEN_GUARD(parser);
        expect_operand = false;
        Error merr = apply_member(parser, &tstack, &ast_stack);
        if (merr != SUCCESS)
          return merr;
        Error uerr = reduce_unary(&parser->stack, &tstack, &ast_stack);
        if (uerr != SUCCESS)
          return uerr;
      }
    } else if (is_operator_token(lookahead.token_type) && !expect_operand) {
      TokenType next = lookahead.token_type;
      DEBUG_LOG("[DEBUG] branch: OPERATOR token %d\n", next);
      bool pushed = false;
      while (!pushed) {
        TokenType top = top_terminal(&parser->stack);
        DEBUG_LOG(
            "[DEBUG] precedence loop: top terminal = %d, next operator = %d\n",
            top, next);
        if (top == T_EOF) {
          DEBUG_LOG("[DEBUG] top terminal is T_EOF, shifting operator\n");
          stack_push(&parser->stack, next);
          CONSUME_TOKEN_GUARD(parser);
          expect_operand = true;
          break;
        }

        ExprColumn left = token_to_column(top);
        ExprColumn right = token_to_column(next);

        if (left == EX_COUNT || right == EX_COUNT)
          return SYN_ANAL_ERR;

        PrecedenceAction action = precedence_table[left][right];
        switch (action) {
        case PREC_SHIFT:
          stack_push(&parser->stack, next);
          CONSUME_TOKEN_GUARD(parser);
          expect_operand = true;
          pushed = true;
          break;
        case PREC_REDUCE: {
          stack_pop(&parser->stack);
          if (!is_operator_token(top))
            return SYN_ANAL_ERR;

          Error err = apply_operator(top, &tstack, &ast_stack);
          if (err != SUCCESS)
            return err;
          break;
        }
        case PREC_EQUAL:
          return SYN_ANAL_ERR;
        case PREC_ERROR:
        default:
          return SYN_ANAL_ERR;
        }
      }
    } else {
      if (expect_operand)
        return SYN_ANAL_ERR;
      break;
    }
    lookahead = look_next_eol(parser);
  }

  while (parser->stack.top >= 0) {
    TokenType top = parser->stack.items[parser->stack.top];
    if (is_open_BRACKET(top) || is_close_BRACKET(top))
      return SYN_ANAL_ERR;
    if (!is_operator_token(top))
      return SYN_ANAL_ERR;

    stack_pop(&parser->stack);
    Error err = apply_operator(top, &tstack, &ast_stack);
    if (err != SUCCESS)
      return err;
  }

  ExprType final_type = EXPR_TYPE_UNKNOWN;
  tstack_peek(&tstack, &final_type);
  if (out_type)
    *out_type = final_type;

  ASTNode *final_ast = NULL;
  ast_stack_pop(&ast_stack, &final_ast);
  if (out_ast)
    *out_ast = final_ast;

  return SUCCESS;
}
