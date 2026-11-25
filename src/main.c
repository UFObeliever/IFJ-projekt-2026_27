/**
 * @file main.c
 *
 * Implementace překladače imperativního jazyka IFJ26
 *
 * @brief Vstupní bod překladače
 *
 * Načte zdrojový kód ze standardního vstupu, provede lexikální, syntaktickou
 * a sémantickou analýzu a vygenerovaný kód IFJcode26 vypíše na standardní
 * výstup. Návratová hodnota programu je kód první nalezené chyby, při úspěchu
 * 0.
 *
 * @author Alex Oľšavský (xolsava00)
 * @author Jaroslav Vančura (xvancuj00)
 * @author Matěj Okruhlica (xokruhm00)
 * @author Klára Vantuchová (xvantuk00)
 */

#include "ast.h"
#include "generator.h"
#include "lexer.h"
#include "parser.h"
#include <stdio.h>

/**
 * @brief Spustí překlad programu ze standardního vstupu
 *
 * Inicializuje lexer, parser a tabulku symbolů, provede analýzu a při úspěchu
 * vygeneruje cílový kód. Na konci uvolní všechnu alokovanou paměť.
 *
 * @return 0 při úspěchu, jinak kód chyby (hodnota Error)
 */
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
