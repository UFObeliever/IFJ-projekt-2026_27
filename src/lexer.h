#ifndef LEXER_H
#define LEXER_H

#include "DynBuff.h"
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum TokenType {
  T_INT_LITERAL,
  T_DOUBLE_LITERAL,
  T_STRING_LITERAL,
  T_IDENTIFICATOR,
  T_KW_DEF,
  T_KW_DO,
  T_KW_DOUBLE,
  T_KW_ELSE,
  T_KW_IF,
  T_KW_IMPORT,
  T_KW_INT,
  T_KW_RETURN,
  T_KW_STRING,
  T_KW_THEN,
  T_KW_UNIT,
  T_KW_VAL,
  T_KW_VAR,
  T_KW_WHILE,
  T_NOT_EQUAL,
  T_PLUS_SIGN,
  T_MINUS_SIGN,
  T_MULT_SIGN,
  T_OPEN_BRACKET,  // (
  T_CLOSE_BRACKET, // )
  T_OPEN_PAREN,
  T_CLOSE_PAREN,
  T_OPEN_CURLY_BRACES,
  T_CLOSE_CURLY_BRACES,
  T_COMMA,
  T_EQUAL,  // token equal
  T_ASSIGN, // token ASSIGN (přiřazení)
  T_DIVIDE, // token dělení
  T_LT,
  T_LTE,
  T_GT,
  T_GTE,
  T_FUNCTION,
  T_INDENTS, // odsazení
  T_ERROR,
  T_EOF,
  T_EOL,
} TokenType;

typedef enum State {
  S_START,      // stav start
  S_ID_1,       // IDENTIFICATOR
  S_UNDERSCORE, // IDENTIFICATOR SOLO UNDERSCORE CHECK
  S_INT_ZERO,   // INTEGER ONLY SOLO ZERO CHECK
  S_INT,        // INTEGER
  S_INT_UNDERSCORE,
  S_MINUS,      // OPTIONAL NEGATIVE INT
  S_DOT_DOUBLE, // S_DOUBLE ORIGINALLY
  S_DOUBLE_E,
  S_DOUBLE_E_SIGN, // DOUBLE +/- OPTIONAL
  S_DOUBLE_AFTER_E,
  S_DOUBLE_UNDERSCORE_AFTER_E,   // Koncový stav floatu
  S_DOUBLE_UNDERSCORE_AFTER_DOT, //
  S_LTE,            // state less than or equal (pro porovnávací operátory)
  S_LT,             // state less than
  S_GTE,            // state greater than or equal to
  S_GT,             // state greater than
  S_NOT_EQUAL,      // State not equal
  S_ASSIGN,         // State assign
  S_IN_MULTILINE,   // State in multiline comment
  S_END_MULTILINE,  // End of multiline comment
  S_STR_START,      // Q1 STRING
  S_STR_END,        // Q2
  S_STR_IN_STR,     // Q3
  S_STR_BACKSLASH,  // Q4
  S_STR_ESC_SEQ,    // Q5 STRING END
  S_MSTR_START_1,   // Q1 MULT. LINE STRING
  S_MSTR_START_2,   // Q2
  S_MSTR_START_3,   // Q3
  S_MSTR_IN_STR,    // Q8
  S_MSTR_AFTER_NL,  // Q9
  S_MSTR_END_1,     // Q4
  S_MSTR_END_2,     // Q5
  S_MSTR_END_3,     // Q6 MULT LINE STRING END
  S_BACKSLASH_COMM, // Q1 COMMENTS
  S_SINGLE_COMM_START,  // Q2
  S_SINGLE_COMM_END,    // Q3 SINGLE LINE COMMENT
  S_START_LINE_COMMENT, // CHECK THIS IS IT IS FOR NESTED LINE COMMENTS
  S_IN_LINE_COMMENT,
  S_START_BLOCK_COMMENT_1,
  S_IN_BLOCK_COMMENT,
  S_BLOCK_COMMENT_END_1,
} State;

typedef struct {
  TokenType type;
  const char *lexeme;
} Token_lexeme;

typedef struct {
  TokenType type;
  int count;
} Token_indents;

typedef union {
  TokenType token_type;
  Token_lexeme token_lexeme;
  Token_indents Token_indents;
} Token;

typedef struct {
  StringBuff Buffer;
  FILE *input;
  bool at_start_of_line;
} LexerCtx;

// Uvolní paměť tokenu
void free_token(Token t);

// Funkce na počítání whitespace
Token lex_indents(LexerCtx *ctx);

/**
 * @brief Funkce tvořící tokeny (filtruje EOL tokeny)
 *
 * @param ctx: Kontext bufferu
 */
Token get_next_token(LexerCtx *ctx);

/**
 * @brief Funkce inicializující lexer
 *
 * @param ctx: pointer na konetext buffer
 * @param input: pointer na čtení z file
 */
void lexer_init(LexerCtx *ctx, FILE *input);

/**
 * @brief Funkce volající free na úklid
 *
 * @param ctx: pointer na kontext buffer
 */
void lexer_free(LexerCtx *ctx);

/**
 * @brief funkce která kontroluje zda identifikátor není keyword
 *
 * @param ctx: Kontext bufferu
 */
Token keyword_check(const LexerCtx *ctx);

#endif
