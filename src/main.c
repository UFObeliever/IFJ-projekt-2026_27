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

  int result = parse_program(&parser);
  if (result == 0) {
    ASTNode *root = ast_create_node(AST_PROGRAM);
    root->data.block.statements = parser.funcs;
    result = generate_program(root, stdout);
  }

  smt_dispose(&parser.smt);

  lexer_free(&lexer_ctx);
  return result;
}
