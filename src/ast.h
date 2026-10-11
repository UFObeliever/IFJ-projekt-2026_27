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
  AST_LITERAL_NULL
} ASTNodeType;

typedef struct ASTNode {
  ASTNodeType type;

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

ASTNode *ast_create_node(ASTNodeType type);
void ast_free(ASTNode *node);

ASTNode *ast_make_int(int value);
ASTNode *ast_make_string(char *value);
ASTNode *ast_make_binop(int operator_type, ASTNode *left, ASTNode *right);
ASTNode *ast_make_if(ASTNode *condition, ASTNode *body, ASTNode *else_body);
ASTNode *ast_make_while(ASTNode *condition, ASTNode *body);

#endif
