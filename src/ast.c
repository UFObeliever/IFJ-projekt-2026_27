#include "ast.h"
#include <stdlib.h>

ASTNode *ast_create_node(ASTNodeType type) {
  ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
  if (node == NULL) {
    return NULL;
  }

  node->type = type;
  node->next = NULL;

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

  case AST_IDENTIFIER:
    free(node->data.string_val);
    break;

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
