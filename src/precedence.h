/**
 * @file precedence.h
 *
 * Překladač jazyka IFJ26
 *
 * @brief Rozhraní precedenční analýzy výrazů.
 *
 * @author Alex Oľšavský (xolsava00)
 * @author Jaroslav Vančura (xvancuj00)
 */

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
  EXPR_TYPE_INT,
  EXPR_TYPE_DOUBLE,
  EXPR_TYPE_STRING,
  EXPR_TYPE_UNIT,
  EXPR_TYPE_NULL,
  EXPR_TYPE_BOOL
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
  EX_AND,
  EX_OR,
  EX_LPAREN,
  EX_RPAREN,
  EX_COMMA,
  EX_DOLLAR,
  EX_OPERAND,
  EX_COUNT
} ExprColumn;

/**
 * @brief Převede typ tokenu na sloupec precedenční tabulky
 *
 * @param t: Typ tokenu
 */
ExprColumn token_to_column(TokenType t);

/**
 * @brief Vrátí nejvyšší terminální symbol na precedenčním zásobníku
 *
 * @param stack: Precedenční zásobník
 */
TokenType top_terminal(PrecedenceStack *stack);

/**
 * @brief Zparsuje výraz a vrátí jeho AST a typ
 *
 * @param parser: Kontext parseru
 * @param out_ast: Výstupní uzel AST
 * @param out_type: Výstupní typ výrazu
 */
Error parse_expression(ParserCtx *parser, ASTNode **out_ast,
                       ExprType *out_type);

#endif
