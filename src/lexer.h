/**
 * @file lexer.h
 *
 * Překladač jazyka IFJ26
 *
 * @brief Rozhraní lexikálního analyzátoru.
 *
 * Obsahuje typy tokenů, kontext lexeru a funkce pro jeho inicializaci,
 * získávání dalších tokenů ze vstupu a uvolnění.
 *
 * @author Matěj Okruhlica (xokruhm00)
 */

#ifndef LEXER_H
#define LEXER_H

#include "DynBuff.h"
#include <stdbool.h>
#include <stdio.h>

#define INTERP_MAX_DEPTH 32

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
  T_COMMA,
  T_EQUAL,  // token equal
  T_ASSIGN, // token ASSIGN (přiřazení)
  T_DIVIDE, // token dělení
  T_LT,
  T_LTE,
  T_GT,
  T_GTE,
  T_INDENTS, // odsazení
  T_ERROR,
  T_EOF,
  T_EOL,
  T_DOUBLE_DOT,
  T_WHITESPACE_SEP,
  T_FUNC_MAIN,
  T_DOT,
  T_SEMICOLON,
  T_KW_IFJ26,
  T_TAB,
  T_EXTENSION,
  T_KW_BOOLEAN, // Boolean
  T_KW_TRUE,
  T_KW_FALSE,
  T_AND,    // &&
  T_OR,     // ||
  T_NOT,    // !
  T_KW_FOR, // for
} TokenType;

typedef struct {
  TokenType type;
  const char *lexeme;
} Token_lexeme;

typedef struct {
  TokenType type;
  int count;
} token_indents;

typedef union {
  TokenType token_type;
  Token_lexeme token_lexeme;
  token_indents token_indents;
} Token;

typedef enum InterpPhase {
  IP_START,
  IP_BODY,
  IP_EXPR,
} InterpPhase;

typedef struct {
  InterpPhase phase;
  bool multiline;
  bool has_part;
  int expr_tokens;
} InterpLevel;

typedef struct {
  StringBuff Buffer;
  FILE *input;
  Token *pending;
  size_t pending_len;
  size_t pending_pos;
  size_t pending_cap;
  int interp_depth;
  InterpLevel interp[INTERP_MAX_DEPTH];
} LexerCtx;

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
 * @brief Makes a string from token type enum
 */
const char *token_type_to_string(TokenType token);

void print_token(const Token *token);

#ifdef NDEBUG
#define DEBUG_LOG(fmt, ...) ((void)0)
#else
#define DEBUG_LOG(fmt, ...)                                                    \
  do {                                                                         \
    fprintf(stderr, "[%s:%d](%s)", __FILE__, __LINE__, __func__);              \
    fprintf(stderr, fmt, ##__VA_ARGS__);                                       \
  } while (0)
#endif
#endif
