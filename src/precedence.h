#ifndef PRECEDENCETABLE_H
#define PRECEDENCETABLE_H

#include "Error.h"
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "symtable.h"
#include <stdlib.h>

typedef enum ExprType {
  EXPR_TYPE_UNKNOWN,
  EXPR_TYPE_NUM,
  EXPR_TYPE_STRING,
  EXPR_TYPE_NULL
} ExprType;

typedef enum PrecedenceAction {
  PREC_SHIFT,
  PREC_REDUCE,
  PREC_EQUAL,
  PREC_ERROR
} PrecedenceAction;

typedef enum ExprColumn {
  EX_PLUS,
  EX_MINUS,
  EX_MUL,
  EX_DIV,
  EX_EQ,
  EX_NEQ,
  EX_LT,
  EX_LTE,
  EX_GT,
  EX_GTE,
  EX_IS,
  EX_LPAREN,
  EX_RPAREN,
  EX_COMMA,
  EX_DOLLAR,
  EX_OPERAND,
  EX_COUNT
} ExprColumn;

ExprColumn token_to_column(TokenType t);
TokenType top_terminal(PrecedenceStack *stack);

Error parse_expression(ParserCtx *parser, ASTNode **out_ast,
                       ExprType *out_type);

#endif
