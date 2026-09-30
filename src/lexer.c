#include "lexer.h"
#include <assert.h>

/**
 * @brief Makro na vytvoření tokenu lexémy
 *
 * @param TYPE: typ tokenu
 * @param LEXEME: lexem na uložení
 */
#define MAKE_TOKEN_LEXEME(TYPE, LEXEME)                                        \
  (Token) {                                                                    \
    .token_lexeme = {.type = (TYPE), .lexeme = sb_copy(LEXEME) }               \
  }

/**
 * @brief Makro na vytvoření tokenu odsadzení
 *
 * @param TYPE: typ tokenu
 * @param COUNT: počet odsadzení
 *
 */
#define MAKE_TOKEN_INDENTS(TYPE, COUNT)                                        \
  (Token) {                                                                    \
    .Token_indents = {.type = (TYPE), .count = (COUNT) }                       \
  }

/**
 * @brief Makro na vytvoření tokenu chyby
 *
 * @param TYPE: typ tokenu
 * @param COUNT: počet odsadzení
 *
 */
#define MAKE_TOKEN_ERROR()                                                     \
  (Token) { .token_type = (T_ERROR) }

/**
 * @brief Makro na vytvoření tokenu bílých znakú
 *
 * @param TYPE: typ tokenu
 */
#define MAKE_TOKEN_WHITESPACE(TYPE)                                            \
  (Token) { .token_type = (TYPE) }

/**
 * @brief Makro na vytvoření tokenu keywordu
 *
 * @param TYPE: typ keywordu
 */
#define MAKE_TOKEN_KW(TYPE)                                                    \
  (Token) { .token_type = (TYPE) }

/**
 * @brief Makro na kontrolu zda token není keyword
 *
 * @param TYPE: typ tokenu
 * @param LEXEME: název tokenu na kontrolu
 */
#define KEYWORD_CHECK(TYPE, LEXEME)                                            \
  if (ctx->Buffer.length == strlen(LEXEME) &&                                  \
      strncmp(ctx->Buffer.data, LEXEME, ctx->Buffer.length) == 0) {            \
    return MAKE_TOKEN_KW(TYPE);                                                \
  }

void lexer_init(LexerCtx *ctx, FILE *input) {
  ctx->input = input;
  ctx->at_start_of_line = true;
  sb_init(&ctx->Buffer, 16);
}

void lexer_free(LexerCtx *ctx) {
  sb_free(&ctx->Buffer);
  ctx->input = NULL;
}

Token keyword_check(const LexerCtx *ctx) {
  /*
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
  */
  KEYWORD_CHECK(T_KW_DEF, "def");
  KEYWORD_CHECK(T_KW_DO, "do");
  KEYWORD_CHECK(T_KW_DOUBLE, "Double");
  KEYWORD_CHECK(T_KW_ELSE, "else");
  KEYWORD_CHECK(T_KW_IF, "if");
  KEYWORD_CHECK(T_KW_IMPORT, "import");
  KEYWORD_CHECK(T_KW_INT, "Int");
  KEYWORD_CHECK(T_KW_RETURN, "return");
  KEYWORD_CHECK(T_KW_STRING, "String");
  KEYWORD_CHECK(T_KW_THEN, "then");
  KEYWORD_CHECK(T_KW_UNIT, "Unit");
  KEYWORD_CHECK(T_KW_VAL, "val");
  KEYWORD_CHECK(T_KW_VAR, "var");
  KEYWORD_CHECK(T_KW_WHILE, "while");

  return MAKE_TOKEN_LEXEME(T_IDENTIFICATOR, &ctx->Buffer);
}

Token lex_indents(LexerCtx *ctx) {
  int num_indents = 0;
  bool has_tab = false;
  while (1) {
    int c = fgetc(ctx->input);
    fprintf(stderr, "[debug]: .... dealing with char: '%c' (%d)\n", (char)c, c);

    if (c == ' ') {
      num_indents++;
    } else if (c == '\t') {
      has_tab = true;
    } else if (c == '\r') {
      continue; // ignore this fucker
    } else if (c == '\n') {
      // reset .... previous line was just full of white space
      num_indents = 0;
      has_tab = false;
    } else {
      if (has_tab) {
        return MAKE_TOKEN_ERROR();
      }
      ungetc(c, ctx->input);
      break;
    }
  }
  return MAKE_TOKEN_INDENTS(T_INDENTS, num_indents);
}

Token lex_identificator(LexerCtx *ctx) {
  State current_state = S_START;
  while (1) {
    int c = fgetc(ctx->input);
    fprintf(stderr, "[debug]: .... dealing with char: '%c' (%d)\n", (char)c, c);

    char input_char = (char)c;
    switch (current_state) {
    case S_START:
      if (c == '_') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_UNDERSCORE;
      }
      if (isalpha(c) || isdigit(c)) {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_ID_1;
      }
      break;
    case S_UNDERSCORE:
      if (c == '_' || isalpha(c) || isdigit(c)) {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_ID_1;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;
    case S_ID_1:
      if (c == '_' || isalpha(c) || isdigit(c)) {
        sb_append_char(&ctx->Buffer, input_char);
      } else {
        fprintf(stderr, "[debug]: .... putting char back: '%c' (%d)\n", (char)c,
                c);
        ungetc(c, ctx->input);
        return keyword_check(ctx);
      }
      break;
    default:
      // this should never happen!!
      assert(false && "Lex Identificator state machine fell through!");
      return MAKE_TOKEN_ERROR();
      break;
    }
  }
}

int skip_white_space(LexerCtx *ctx) {
  while (1) {
    int c = fgetc(ctx->input);
    fprintf(stderr, "[debug]: .... dealing with char: '%c' (%d)\n", (char)c, c);
    if (c == ' ' || c == '\r' || c == '\t') {
      continue;
    }
    return c;
  }
}

Token get_next_token(LexerCtx *ctx) {
  int c;
  sb_reset(&ctx->Buffer);
  c = fgetc(ctx->input);
  fprintf(stderr, "[debug]: .... dealing with char: '%c' (%d)\n", (char)c, c);
  if (c == EOF) {
    return MAKE_TOKEN_WHITESPACE(T_EOF);
  }

  /* dopsat stavy z jednotlivých FSM */
  if (c == ' ') {
    if (ctx->at_start_of_line) {
      fprintf(stderr, "[debug]: .... putting char back: '%c' (%d)\n", (char)c,
              c);
      ungetc(c, ctx->input);
      return lex_indents(ctx);
    }
  }

  // skip all incomming white space
  ungetc(c, ctx->input);
  fprintf(stderr, "[debug]: .... putting char back: '%c' (%d)\n", (char)c, c);
  c = skip_white_space(ctx);
  ctx->at_start_of_line = false;

  if (c == '_' || isalpha(c)) {
    fprintf(stderr, "[debug]: .... putting char back: '%c' (%d)\n", (char)c, c);
    ungetc(c, ctx->input);
    return lex_identificator(ctx);
  }

  if (c == '\n') {
    ctx->at_start_of_line = true;
    return MAKE_TOKEN_WHITESPACE(T_EOL);
  }

  return MAKE_TOKEN_ERROR();
}
