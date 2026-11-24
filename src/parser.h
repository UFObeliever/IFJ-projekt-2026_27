/**
 * @file parser.h
 *
 * Implementace překladače imperativního jazyka IFJ26
 *
 * @brief Rozhraní syntaktického analyzátoru.
 *
 * Obsahuje kontext parseru a funkce, které používá hlavní program
 * a precedenční analýza výrazů.
 *
 * @author Alex Oľšavský (xolsava00)
 * @author Jaroslav Vančura (xvancuj00)
 */

#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "lexer.h"
#include "symtable.h"

typedef struct {
  TokenType items[1024];
  int top;
} PrecedenceStack;

typedef struct ParserCtx {
  Token current_token;
  LexerCtx *lexer;
  symtable smt;
  PrecedenceStack stack;
  TokenType ret_type;
  int main_count;
  int line_indent;
  bool at_bol;
  ASTNode *funcs;
  ASTNode *funcs_tail;
  Token *tokens;
  size_t tokens_len;
  size_t tokens_cap;
  size_t pos;
} ParserCtx;

/**
 * @brief Vrátí aktuální token bez posunu (včetně konce řádku)
 *
 * @param parser: kontext parseru
 */
Token look_next_eol(ParserCtx *parser);

/**
 * @brief Vrátí aktuální token a posune se na další
 *
 * @param parser: kontext parseru
 */
Token consume_token(ParserCtx *parser);

/**
 * @brief Vrátí token za aktuálním tokenem bez posunu čtení
 *
 * @param parser: kontext parseru
 */
Token peek_token(ParserCtx *parser);

/**
 * @brief Zpracuje celý program a vytvoří AST jeho funkcí
 *
 * @param ctx: inicializovaný kontext parseru
 */
int parse_program(ParserCtx *ctx);

/**
 * @brief Uvolní vyrovnávací paměť tokenů
 *
 * @param ctx: kontext parseru
 */
void parser_free(ParserCtx *ctx);

/**
 * @brief Analyzuje tělo funkce mimo pořadí, aby byl znám její návratový typ
 *
 * @param ctx: kontext parseru
 * @param name: jméno funkce
 * @param fentry: záznam funkce v tabulce symbolů
 */
int resolve_function(ParserCtx *ctx, const char *name, symtable_entry *fentry);

/**
 * @brief Zpracuje if jako výraz; volá ji precedenční analýza na místě operandu
 *
 * @param ctx: kontext parseru, aktuální token je if
 * @param out: kořen AST (uzel AST_IF s nastaveným expr_type)
 * @param out_type: typ výsledku (ExprType)
 */
int parse_if_expression(ParserCtx *ctx, ASTNode **out, int *out_type);

#endif
