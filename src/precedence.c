#include "precedence.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define TYPE_STACK_MAX 256

typedef struct {
  ExprType items[TYPE_STACK_MAX];
  int top;
} TypeStack;

static void tstack_init(TypeStack *s) { s->top = -1; }
static bool tstack_push(TypeStack *s, ExprType t) {
  if (s->top >= TYPE_STACK_MAX - 1)
    return false;
  s->items[++s->top] = t;
  return true;
}
static bool tstack_pop(TypeStack *s, ExprType *out) {
  if (s->top < 0)
    return false;
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

static void stack_clear(PrecedenceStack *s) { s->top = -1; }

static bool stack_push(PrecedenceStack *s, TokenType t) {
  if (s->top >= 255)
    return false;
  s->items[++s->top] = t;
  return true;
}

static bool stack_pop(PrecedenceStack *s) {
  if (s->top < 0)
    return false;
  s->top--;
  return true;
}

static ExprType expr_type_from_token(TokenType t) {
  switch (t) {
  case T_INT_LITERAL:
  case T_DOUBLE_LITERAL:
    return EXPR_TYPE_NUM;
  case T_STRING_LITERAL:
    return EXPR_TYPE_STRING;
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

static Error check_identifier_in_expr(ParserCtx *parser, const Token *token) {
  symtable_entry *entry = NULL;

  if (smt_search(&parser->smt, (char *)token->token_lexeme.lexeme, &entry)) {
    if (!entry->function) {
      return SUCCESS;
    }
    return STATIC_SEM_TYPE_ERR;
  }

  symtable_entry *head = NULL;
  if (smt_search_global(&parser->smt, (char *)token->token_lexeme.lexeme,
                        &head)) {
    return STATIC_SEM_TYPE_ERR;
  }

  return UNDEF_FN_ERR;
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
    return true;
  default:
    return false;
  }
}

Error apply_operator(TokenType op, TypeStack *tstack, ASTStack *ast_stack) {
  ExprType right_type, left_type;
  ASTNode *right_node, *left_node;

  if (!tstack_pop(tstack, &right_type) || !tstack_pop(tstack, &left_type)) {
    return SYN_ANAL_ERR;
  }
  if (!ast_stack_pop(ast_stack, &right_node) ||
      !ast_stack_pop(ast_stack, &left_node)) {
    return SYN_ANAL_ERR;
  }

  ExprType result = EXPR_TYPE_UNKNOWN;

  switch (op) {
  case T_PLUS_SIGN:
    if (left_type == EXPR_TYPE_NUM && right_type == EXPR_TYPE_NUM)
      result = EXPR_TYPE_NUM;
    else if (left_type == EXPR_TYPE_STRING && right_type == EXPR_TYPE_STRING)
      result = EXPR_TYPE_STRING;
    else if (left_type != EXPR_TYPE_UNKNOWN && right_type != EXPR_TYPE_UNKNOWN)
      return STATIC_SEM_TYPE_ERR;
    break;
  case T_MINUS_SIGN:
  case T_MULT_SIGN:
  case T_DIVIDE:
    if (op == T_MULT_SIGN && left_type == EXPR_TYPE_STRING &&
        right_type == EXPR_TYPE_NUM)
      result = EXPR_TYPE_STRING;
    else if (left_type == EXPR_TYPE_NUM && right_type == EXPR_TYPE_NUM)
      result = EXPR_TYPE_NUM;
    else if (left_type != EXPR_TYPE_UNKNOWN && right_type != EXPR_TYPE_UNKNOWN)
      return STATIC_SEM_TYPE_ERR;
    break;
  case T_LT:
  case T_LTE:
  case T_GT:
  case T_GTE:
    if (left_type != EXPR_TYPE_UNKNOWN && right_type != EXPR_TYPE_UNKNOWN) {
      if (left_type != EXPR_TYPE_NUM || right_type != EXPR_TYPE_NUM)
        return STATIC_SEM_TYPE_ERR;
    }
    result = EXPR_TYPE_NUM;
    break;
  case T_EQUAL:
  case T_NOT_EQUAL:
    result = EXPR_TYPE_NUM;
    break;
  default:
    result = EXPR_TYPE_UNKNOWN;
    break;
  }

  if (!tstack_push(tstack, result)) {
    return COMPILER_ERR;
  }

  ASTNode *binop_node = ast_make_binop(op, left_node, right_node);
  if (!ast_stack_push(ast_stack, binop_node)) {
    return COMPILER_ERR;
  }

  return SUCCESS;
}

Error parse_expression(ParserCtx *parser, ASTNode **out_ast,
                       ExprType *out_type) {
  stack_clear(&parser->stack);

  TypeStack tstack;
  tstack_init(&tstack);

  ASTStack ast_stack;
  ast_stack_init(&ast_stack);

  bool expect_operand = true;
  int BRACKET_depth = 0;

  Token lookahead = look_next_eol(parser);
  if (lookahead.token_type == T_ERROR)
    return LEX_ANAL_ERR;

  while (true) {
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
      if (expect_operand)
        return SYN_ANAL_ERR;
      break;
    }

    if (is_operand_token(lookahead.token_type) && expect_operand) {
      ASTNode *leaf = NULL;
      if (lookahead.token_type == T_IDENTIFICATOR) {
        Error err = check_identifier_in_expr(parser, &lookahead);
        if (err != SUCCESS)
          return err;

        leaf = ast_create_node(AST_IDENTIFIER);
        leaf->data.string_val = strdup((char *)lookahead.token_lexeme.lexeme);
      } else if (lookahead.token_type == T_INT_LITERAL) {
        leaf = ast_make_int(atoi((char *)lookahead.token_lexeme.lexeme));
      } else if (lookahead.token_type == T_STRING_LITERAL) {
        leaf = ast_make_string(strdup((char *)lookahead.token_lexeme.lexeme));
      } else {
        leaf = ast_create_node(AST_LITERAL_NULL);
      }

      ExprType t = expr_type_from_token(lookahead.token_type);
      if (!tstack_push(&tstack, t) || !ast_stack_push(&ast_stack, leaf)) {
        return COMPILER_ERR;
      }
      consume_token(parser);
      expect_operand = false;
    } else if (is_open_BRACKET(lookahead.token_type) && expect_operand) {
      stack_push(&parser->stack, lookahead.token_type);
      consume_token(parser);
      BRACKET_depth++;
    } else if (is_close_BRACKET(lookahead.token_type) && !expect_operand) {
      bool found = false;
      while (parser->stack.top >= 0) {
        TokenType top = parser->stack.items[parser->stack.top];
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
          return SYN_ANAL_ERR;
        }
      }
      if (!found)
        return SYN_ANAL_ERR;
      consume_token(parser);
      expect_operand = false;
    } else if (is_operator_token(lookahead.token_type) && !expect_operand) {
      TokenType next = lookahead.token_type;
      while (true) {
        TokenType top = top_terminal(&parser->stack);
        if (top == T_EOF) {
          stack_push(&parser->stack, next);
          consume_token(parser);
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
          consume_token(parser);
          expect_operand = true;
          return SUCCESS;
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
      return SYN_ANAL_ERR;
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
