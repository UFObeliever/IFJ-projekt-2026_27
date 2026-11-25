#include "ast.h"
#include "generator.h"
#include "lexer.h"
#include "parser.h"
#include <stdio.h>
int main(void) {
  LexerCtx lexer_ctx;
  lexer_init(&lexer_ctx, stdin);

  ParserCtx parser;
  parser.lexer = &lexer_ctx;
  smt_init(&parser.smt);
  parser.stack.top = -1;
  parser.ret_type = T_ERROR;
  parser.main_count = 0;
  parser.funcs = NULL;
  parser.funcs_tail = NULL;
  parser.for_count = 0;

  int result = parse_program(&parser);
  ASTNode *root = ast_create_node(AST_PROGRAM);
  root->data.block.statements = parser.funcs;
  if (result == 0)
    result = generate_program(root, stdout);
  ast_free(root);

  smt_dispose(&parser.smt);

  lexer_free(&lexer_ctx);
  parser_free(&parser);
  return result;
}
