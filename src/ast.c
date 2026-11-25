/**
 * @file ast.c
 *
 * Překladač jazyka IFJ26
 *
 * @brief Implementace alokace, konstruktorů a uvolňování uzlů AST.
 *
 * @author Alex Oľšavský (xolsava00)
 * @author Jaroslav Vančura (xvancuj00)
 */

#include "ast.h"
#include "error.h"
#include <stdio.h>
#include <stdlib.h>

ASTNode *ast_create_node(ASTNodeType type) {
  ASTNode *node = calloc(1, sizeof(ASTNode));
  if (node == NULL) {
    fprintf(stderr, "Chyba alokace pameti pro uzel AST\n");
    exit(COMPILER_ERR);
  }
  node->type = type;
  return node;
}

void ast_free(ASTNode *node) {
  if (node == NULL)
    return;

  ast_free(node->next);

  switch (node->type) {
  case AST_BIN_OP:
    ast_free(node->data.bin_op.left);
    ast_free(node->data.bin_op.right);
    break;

  case AST_UNARY_OP:
    ast_free(node->data.unary_op.operand);
    break;

  case AST_IF:
    ast_free(node->data.if_stmt.condition);
    ast_free(node->data.if_stmt.body);
    ast_free(node->data.if_stmt.else_body);
    break;

  case AST_WHILE:
    ast_free(node->data.while_loop.condition);
    ast_free(node->data.while_loop.body);
    break;

  case AST_PROGRAM:
  case AST_BLOCK:
    ast_free(node->data.block.statements);
    break;

  case AST_FUNC_DEF:
    free(node->data.func_def.name);
    ast_free(node->data.func_def.params);
    ast_free(node->data.func_def.body);
    break;

  case AST_VAR_DECL:
  case AST_ASSIGNMENT:
    free(node->data.var_assign.identifier);
    ast_free(node->data.var_assign.expression);
    break;

  case AST_RETURN:
    ast_free(node->data.return_stmt.expression);
    break;

  case AST_FUNC_CALL:
    free(node->data.func_call.name);
    ast_free(node->data.func_call.arguments);
    break;

  case AST_IDENTIFIER:
  case AST_LITERAL_STRING:
    free(node->data.string_val);
    break;

  default:
    break;
  }

  free(node);
}
ASTNode *ast_make_int(int value) {
  ASTNode *node = ast_create_node(AST_LITERAL_INT);
  if (node)
    node->data.int_val = value;
  return node;
}
ASTNode *ast_make_double(double value) {
  ASTNode *node = ast_create_node(AST_LITERAL_FLOAT);
  if (node)
    node->data.float_val = value;
  return node;
}

ASTNode *ast_make_bool(bool value) {
  ASTNode *node = ast_create_node(AST_LITERAL_BOOL);
  if (node)
    node->data.int_val = value ? 1 : 0;
  return node;
}

ASTNode *ast_make_string(char *value) {
  ASTNode *node = ast_create_node(AST_LITERAL_STRING);
  if (node)
    node->data.string_val = value;
  return node;
}

ASTNode *ast_make_binop(int operator_type, ASTNode *left, ASTNode *right) {
  ASTNode *node = ast_create_node(AST_BIN_OP);
  if (node) {
    node->data.bin_op.operator_type = operator_type;
    node->data.bin_op.left = left;
    node->data.bin_op.right = right;
  }
  return node;
}

ASTNode *ast_make_if(ASTNode *condition, ASTNode *body, ASTNode *else_body) {
  ASTNode *node = ast_create_node(AST_IF);
  if (node) {
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.body = body;
    node->data.if_stmt.else_body = else_body;
  }
  return node;
}

ASTNode *ast_make_while(ASTNode *condition, ASTNode *body) {
  ASTNode *node = ast_create_node(AST_WHILE);
  if (node) {
    node->data.while_loop.condition = condition;
    node->data.while_loop.body = body;
  }
  return node;
}

ASTNode *ast_make_unary(int operator_type, ASTNode *operand) {
  ASTNode *node = ast_create_node(AST_UNARY_OP);
  if (node) {
    node->data.unary_op.operator_type = operator_type;
    node->data.unary_op.operand = operand;
  }
  return node;
}
