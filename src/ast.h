/**
 * @file ast.h
 *
 * Překladač jazyka IFJ26
 *
 * @brief Rozhraní abstraktního syntaktického stromu (AST).
 *
 * Definuje typy uzlů, strukturu uzlu AST a funkce pro jejich vytváření
 * a uvolňování.
 * @author Alex Oľšavský (xolsava00)
 * @author Jaroslav Vančura (xvancuj00)
 */

#ifndef AST_H
#define AST_H

#include <stdbool.h>

typedef enum {
  AST_PROGRAM,
  AST_FUNC_DEF,
  AST_BLOCK,
  AST_VAR_DECL,
  AST_ASSIGNMENT,
  AST_IF,
  AST_WHILE,
  AST_RETURN,
  AST_FUNC_CALL,
  AST_BIN_OP,
  AST_UNARY_OP,
  AST_IDENTIFIER,
  AST_LITERAL_INT,
  AST_LITERAL_FLOAT,
  AST_LITERAL_STRING,
  AST_LITERAL_NULL,
  AST_LITERAL_UNIT,
  AST_LITERAL_BOOL
} ASTNodeType;

typedef struct ASTNode {
  ASTNodeType type;
  int expr_type;
  bool is_main;
  struct ASTNode *next;

  union {
    struct {
      char *name;
      struct ASTNode *params;
      struct ASTNode *body;
    } func_def;

    struct {
      struct ASTNode *statements;
    } block;

    struct {
      char *identifier;
      struct ASTNode *expression;
    } var_assign;

    struct {
      struct ASTNode *condition;
      struct ASTNode *body;
      struct ASTNode *else_body;
    } if_stmt;

    struct {
      struct ASTNode *condition;
      struct ASTNode *body;
    } while_loop;

    struct {
      struct ASTNode *expression;
    } return_stmt;

    struct {
      char *name;
      struct ASTNode *arguments;
    } func_call;

    struct {
      int operator_type;
      struct ASTNode *left;
      struct ASTNode *right;
    } bin_op;

    struct {
      int operator_type;
      struct ASTNode *operand;
    } unary_op;

    char *string_val;

    int int_val;

    double float_val;

  } data;
} ASTNode;

/**
 * @brief Vytvoří nový uzel AST zadaného typu
 *
 * @param type: Typ uzlu AST
 */
ASTNode *ast_create_node(ASTNodeType type);

/**
 * @brief Rekurzivně uvolní uzel AST
 *
 * @param node: Uvolňovaný uzel
 */
void ast_free(ASTNode *node);

/**
 * @brief Vytvoří uzel celočíselného literálu
 *
 * @param value: Hodnota literálu
 */
ASTNode *ast_make_int(int value);

/**
 * @brief Vytvoří uzel desetinného literálu
 *
 * @param value: Hodnota literálu
 */
ASTNode *ast_make_double(double value);

/**
 * @brief Vytvoří uzel řetězcového literálu
 *
 * @param value: Řetězec literálu
 */
ASTNode *ast_make_string(char *value);

/**
 * @brief Vytvoří uzel booleovského literálu
 *
 * @param value: Pravdivostní hodnota
 */
ASTNode *ast_make_bool(bool value);

/**
 * @brief Vytvoří uzel binární operace
 *
 * @param operator_type: Typ operátoru
 * @param left: Levý operand
 * @param right: Pravý operand
 */
ASTNode *ast_make_binop(int operator_type, ASTNode *left, ASTNode *right);

/**
 * @brief Vytvoří uzel podmínky if
 *
 * @param condition: Podmínka
 * @param body: Větev then
 * @param else_body: Větev else
 */
ASTNode *ast_make_if(ASTNode *condition, ASTNode *body, ASTNode *else_body);

/**
 * @brief Vytvoří uzel cyklu while
 *
 * @param condition: Podmínka
 * @param body: Tělo cyklu
 */
ASTNode *ast_make_while(ASTNode *condition, ASTNode *body);

/**
 * @brief Vytvoří uzel unární operace
 *
 * @param operator_type: Typ operátoru
 * @param operand: Operand
 */
ASTNode *ast_make_unary(int operator_type, ASTNode *operand);

#endif
