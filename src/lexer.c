/**
 * @file lexer.c
 *
 * Překladač jazyka IFJ26
 *
 * @brief Implementace lexikálního analyzátoru.
 *
 * Lexer čte zdrojový kód ze vstupního souboru a zpracovává ho
 * konečnými automaty. Rozpoznává identifikátory, klíčová slova,
 * číselné a řetězcové literály (i víceřádkové a interpolované),
 * operátory, komentáře a odsazení. Interpolované řetězce převádí
 * na posloupnost tokenů spojených operátorem +.
 *
 * @author Matěj Okruhlica (xokruhm00)
 */

#include "lexer.h"
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

typedef enum State {
  STATE_START, // nic nepřečteno

  STATE_INDENT,     // "  "
  STATE_INDENT_TAB, // "  \t"

  STATE_ID_UNDERSCORE, // _
  STATE_ID,            // abc  _a

  STATE_ZERO,                          // 0
  STATE_INT,                           // 12
  STATE_INT_UNDERSCORE,                // 1_
  STATE_DOUBLE_DOT,                    // 1.  .5
  STATE_DOUBLE_FRACTION,               // 1.1
  STATE_DOUBLE_FRACTION_UNDERSCORE,    // 1.1_
  STATE_DOUBLE_EXP_CHAR,               // 1e  1.1e
  STATE_DOUBLE_EXP_SIGN_OR_UNDERSCORE, // 1e+  1e-  1e1_
  STATE_DOUBLE_EXPONENT,               // 1e1

  STATE_STRING,                   // "
  STATE_STRING_BODY,              // "a
  STATE_STRING_ESCAPE,            // "a\ (další symbol)
  STATE_STRING_CLOSED,            // ""  "a"  (+ " = """)
  STATE_MULTILINE_STRING,         // """
  STATE_MULTILINE_STRING_BODY,    // """a
  STATE_MULTILINE_STRING_QUOTE_1, // """a"
  STATE_MULTILINE_STRING_QUOTE_2, // """a""

  STATE_INTERP_STRING,        // s"
  STATE_INTERP_STRING_QUOTE,  // s""
  STATE_INTERP_STRING_BODY,   // s"a
  STATE_INTERP_STRING_ESCAPE, // s"a\ (další symbol)
  STATE_INTERP_STRING_DOLLAR, // s"a$
  STATE_INTERP_STRING_ID,     // s"$a
  STATE_INTERP_STRING_QUOTES, // s"""a"

  STATE_SLASH,                       // /
  STATE_LINE_COMMENT,                // //
  STATE_BLOCK_COMMENT,               // /* a
  STATE_BLOCK_COMMENT_STAR,          // /* a *
  STATE_BLOCK_COMMENT_SLASH,         // /* a /
  STATE_BLOCK_COMMENT_NEWLINE,       // /* a\n b
  STATE_BLOCK_COMMENT_NEWLINE_STAR,  // /* a\n b *
  STATE_BLOCK_COMMENT_NEWLINE_SLASH, // /* a\n b /
  STATE_BLOCK_COMMENT_NEWLINE_END,   // /* a\n b */

  STATE_AT,     // @
  STATE_AT_M,   // @m
  STATE_AT_MA,  // @ma
  STATE_AT_MAI, // @mai
} State;

#define INTERP_TOP(ctx) ((ctx)->interp[(ctx)->interp_depth - 1])

/**
 * @brief Makro na vytvoření tokenu lexémy
 *
 * @param TYPE: typ_tokenu
 * @param LEXEME: lexem_na_uložení
 */
#define MAKE_TOKEN_LEXEME(TYPE, LEXEME)                                        \
  (Token) {                                                                    \
    .token_lexeme = {.type = (TYPE), .lexeme = sb_copy(LEXEME) }               \
  }

/**
 * @brief Makro na vytvoření dorbnějších tokenů (T_EQUAL, T_LTE, atd.)
 *
 * @param TYPE: typ tokenu
 */
#define MAKE_TOKEN(TYPE)                                                       \
  (Token) { .token_type = (TYPE) }

/**
 * @brief Makro na vytvoření tokenu odsadzení
 *
 * @param COUNT: počet_odsadzení
 *
 */
#define MAKE_TOKEN_INDENTS(COUNT)                                              \
  (Token) {                                                                    \
    .token_indents = {.type = T_INDENTS, .count = (COUNT) }                    \
  }

/**
 * @brief Makro na vytvoření tokenu chyby
 */
#define MAKE_TOKEN_ERROR()                                                     \
  (Token) { .token_type = (T_ERROR) }

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

/**
 * @brief Makro na vytvoření tokenu tabulátoru
 */
#define MAKE_TOKEN_TAB()                                                       \
  (Token) { .token_type = (T_TAB) }

static void discard_token(Token t) {
  switch (t.token_type) {
  case T_INT_LITERAL:
  case T_DOUBLE_LITERAL:
  case T_STRING_LITERAL:
  case T_IDENTIFICATOR:
    free((void *)t.token_lexeme.lexeme);
    break;
  default:
    break;
  }
}

static void queue_clear(LexerCtx *ctx) {
  for (size_t i = ctx->pending_pos; i < ctx->pending_len; i++) {
    discard_token(ctx->pending[i]);
  }
  ctx->pending_len = 0;
  ctx->pending_pos = 0;
}

static bool queue_push(LexerCtx *ctx, Token t) {
  if (ctx->pending_len == ctx->pending_cap) {
    size_t cap = ctx->pending_cap ? ctx->pending_cap * 2 : 16;
    Token *tmp = realloc(ctx->pending, cap * sizeof(Token));
    if (tmp == NULL) {
      return false;
    }
    ctx->pending = tmp;
    ctx->pending_cap = cap;
  }
  ctx->pending[ctx->pending_len++] = t;
  return true;
}

static bool queue_push_lexeme(LexerCtx *ctx, TokenType type, const char *text) {
  size_t len = strlen(text);
  char *copy = malloc(len + 1);
  if (copy == NULL) {
    return false;
  }
  memcpy(copy, text, len + 1);
  Token t = {.token_lexeme = {.type = type, .lexeme = copy}};
  if (!queue_push(ctx, t)) {
    free(copy);
    return false;
  }
  return true;
}

static Token queue_pop(LexerCtx *ctx) {
  Token t = ctx->pending[ctx->pending_pos++];
  if (ctx->pending_pos == ctx->pending_len) {
    ctx->pending_pos = 0;
    ctx->pending_len = 0;
  }
  return t;
}

void lexer_init(LexerCtx *ctx, FILE *input) {
  ctx->input = input;
  sb_init(&ctx->Buffer, 16);

  ctx->pending = NULL;
  ctx->pending_len = 0;
  ctx->pending_pos = 0;
  ctx->pending_cap = 0;
  ctx->interp_depth = 0;
}

void lexer_free(LexerCtx *ctx) {
  queue_clear(ctx);
  free(ctx->pending);
  ctx->pending = NULL;
  ctx->pending_cap = 0;
  sb_free(&ctx->Buffer);
  ctx->input = NULL;
}

static Token keyword_check(const LexerCtx *ctx) {
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
  KEYWORD_CHECK(T_EXTENSION, "extension");
  KEYWORD_CHECK(T_KW_BOOLEAN, "Boolean");
  KEYWORD_CHECK(T_KW_TRUE, "true");
  KEYWORD_CHECK(T_KW_FALSE, "false");
  KEYWORD_CHECK(T_KW_FOR, "for");
  KEYWORD_CHECK(T_KW_IFJ26, "ifj26");

  return MAKE_TOKEN_LEXEME(T_IDENTIFICATOR, &ctx->Buffer);
}

static Token lex_white_space(LexerCtx *ctx) {
  int num_indents = 0;
  State current_state = STATE_INDENT;
  while (1) {
    int c = fgetc(ctx->input);
    DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
    switch (current_state) {
    case STATE_INDENT:
      if (c == '\t') {
        current_state = STATE_INDENT_TAB;
      } else if (c == ' ') {
        num_indents++;
      } else if (c == '\r') {
        continue; // ignore \r ....for windows compatibility
      } else {
        ungetc(c, ctx->input);
        DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
        return MAKE_TOKEN_INDENTS(num_indents);
      }
      break;
    case STATE_INDENT_TAB:
      if (c == '\n') {
        return MAKE_TOKEN(T_EOL);
      } else if (c == ' ' || c == '\t' || c == '\r') {
        continue; // stay in this state
      } else if (c == EOF) {
        return MAKE_TOKEN(T_EOF);
      } else {
        ungetc(c, ctx->input);
        DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
        return MAKE_TOKEN_ERROR();
      }
      break;

    default:
      // should never happen!!
      assert(false && "State machine for indents fell through!");
      return MAKE_TOKEN_ERROR();
      break;
    }
  }
}

static Token lex_identificator(LexerCtx *ctx) {
  State current_state = STATE_START;
  while (1) {
    int c = fgetc(ctx->input);
    DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);

    char input_char = (char)c;
    switch (current_state) {
    case STATE_START:
      if (c == '_') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_ID_UNDERSCORE;
      }
      if (isalpha(c) || isdigit(c)) {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_ID;
      }
      break;
    case STATE_ID_UNDERSCORE:
      if (c == '_' || isalpha(c) || isdigit(c)) {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_ID;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;
    case STATE_ID:
      if (c == '_' || isalpha(c) || isdigit(c)) {
        sb_append_char(&ctx->Buffer, input_char);
      } else {
        DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
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

static Token lex_numbers(LexerCtx *ctx) {
  State current_state = STATE_START;

  while (1) {
    int c = fgetc(ctx->input);
    DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
    char input_char = (char)c;
    switch (current_state) {
    case STATE_START:
      if (c == '0') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_ZERO;
      } else if (c >= '1' && c <= '9') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_INT;
      } else if (c == '.') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_DOUBLE_DOT;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_ZERO:
      if (isdigit(c)) {
        return MAKE_TOKEN_ERROR();
      } else if (c == '.') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_DOUBLE_DOT;
      } else if (c == 'e' || c == 'E') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_DOUBLE_EXP_CHAR;
      } else {
        ungetc(c, ctx->input);
        return MAKE_TOKEN_LEXEME(T_INT_LITERAL, &ctx->Buffer);
      }
      break;

    case STATE_INT:
      if (c == '_') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_INT_UNDERSCORE;
      } else if (isdigit(c)) {
        sb_append_char(&ctx->Buffer, input_char);
        // loop, kdy dostává čísla
      } else if (c == '.') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_DOUBLE_DOT;
      } else if (c == 'e' || c == 'E') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_DOUBLE_EXP_CHAR;
      } else {
        ungetc(c, ctx->input);
        return MAKE_TOKEN_LEXEME(T_INT_LITERAL, &ctx->Buffer);
      }
      break;

    case STATE_INT_UNDERSCORE:
      if (isdigit(c)) {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_INT;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_DOUBLE_EXP_CHAR:
      if (isdigit(c)) {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_DOUBLE_EXPONENT;
      } else if (c == '-' || c == '+') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_DOUBLE_EXP_SIGN_OR_UNDERSCORE;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_DOUBLE_EXPONENT:
      if (isdigit(c)) {
        sb_append_char(&ctx->Buffer, input_char);
        // loop čísel
      } else if (c == '_') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_DOUBLE_EXP_SIGN_OR_UNDERSCORE;
      } else if (c == 'd' || c == 'D') {
        sb_append_char(&ctx->Buffer, input_char);
        return MAKE_TOKEN_LEXEME(T_DOUBLE_LITERAL, &ctx->Buffer);
      } else {
        ungetc(c, ctx->input);
        DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
        return MAKE_TOKEN_LEXEME(T_DOUBLE_LITERAL, &ctx->Buffer);
      }
      break;

    case STATE_DOUBLE_EXP_SIGN_OR_UNDERSCORE:
      if (isdigit(c)) {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_DOUBLE_EXPONENT;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_DOUBLE_DOT:
      if (isdigit(c)) {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_DOUBLE_FRACTION;
      } else {
        ungetc(c, ctx->input);
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_DOUBLE_FRACTION:
      if (isdigit(c)) {
        sb_append_char(&ctx->Buffer, input_char);
      } else if (c == '_') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_DOUBLE_FRACTION_UNDERSCORE;
      } else if (c == 'e' || c == 'E') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_DOUBLE_EXP_CHAR;
      } else if (c == 'd' || c == 'D') {
        sb_append_char(&ctx->Buffer, input_char);
        return MAKE_TOKEN_LEXEME(T_DOUBLE_LITERAL, &ctx->Buffer);
      } else {
        ungetc(c, ctx->input);
        DEBUG_LOG("putting char back %c (%d)\n", (char)c, c);
        return MAKE_TOKEN_LEXEME(T_DOUBLE_LITERAL, &ctx->Buffer);
      }
      break;

    case STATE_DOUBLE_FRACTION_UNDERSCORE:
      if (isdigit(c)) {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_DOUBLE_FRACTION;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;
    default:
      // this should never happen!!
      assert(false && "Lex integer/double state machine fell through!");
      return MAKE_TOKEN_ERROR();
      break;
    }
  }
}

static Token lex_string(LexerCtx *ctx) {
  State current_state = STATE_START;
  while (1) {
    int c = fgetc(ctx->input);
    DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
    char input_char = (char)c;
    switch (current_state) {
    case STATE_START:
      if (c == '"') {
        current_state = STATE_STRING;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_STRING:
      if (c == '\\') {
        current_state = STATE_STRING_ESCAPE;
      } else if (c == '"') {
        current_state = STATE_STRING_CLOSED;
      } else if (c >= ' ' && c < 127) {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_STRING_BODY;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_STRING_BODY:
      if (c == '\\') {
        current_state = STATE_STRING_ESCAPE;
      } else if (c == '"') {
        current_state = STATE_STRING_CLOSED;
      } else if (c >= ' ' && c < 127) {
        sb_append_char(&ctx->Buffer, input_char);
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_STRING_ESCAPE:
      if (c == 'n') {
        sb_append_char(&ctx->Buffer, '\n');
        current_state = STATE_STRING_BODY;
      } else if (c == 't') {
        sb_append_char(&ctx->Buffer, '\t');
        current_state = STATE_STRING_BODY;
      } else if (c == 'r') {
        sb_append_char(&ctx->Buffer, '\r');
        current_state = STATE_STRING_BODY;
      } else if (c == '\\') {
        sb_append_char(&ctx->Buffer, '\\');
        current_state = STATE_STRING_BODY;
      } else if (c == '"') {
        sb_append_char(&ctx->Buffer, '\"');
        current_state = STATE_STRING_BODY;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_STRING_CLOSED:
      if (c == '"') {
        current_state = STATE_MULTILINE_STRING;
      } else {
        ungetc(c, ctx->input);
        DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
        return MAKE_TOKEN_LEXEME(T_STRING_LITERAL, &ctx->Buffer);
      }
      break;

    case STATE_MULTILINE_STRING:
      if (c == '"') {
        current_state = STATE_MULTILINE_STRING_QUOTE_1;
      } else if (c == '\n' || c == '\r' || c == '\t' ||
                 (c >= ' ' && c != 127)) {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_MULTILINE_STRING_BODY;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_MULTILINE_STRING_BODY:
      if (c == '"') {
        current_state = STATE_MULTILINE_STRING_QUOTE_1;
      } else if (c == '\n' || c == '\r' || c == '\t' ||
                 (c >= ' ' && c != 127)) {
        sb_append_char(&ctx->Buffer, input_char);
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_MULTILINE_STRING_QUOTE_1:
      if (c == '"') {
        current_state = STATE_MULTILINE_STRING_QUOTE_2;
      } else if (c == '\n' || c == '\r' || c == '\t' ||
                 (c >= ' ' && c != 127)) {
        sb_append_char(&ctx->Buffer,
                       '"'); // to not loose the " from the previous state
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_MULTILINE_STRING_BODY;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_MULTILINE_STRING_QUOTE_2:
      if (c == '"') {
        return MAKE_TOKEN_LEXEME(T_STRING_LITERAL, &ctx->Buffer);
      } else if (c == '\n' || c == '\r' || c == '\t' ||
                 (c >= ' ' && c != 127)) {
        sb_append_char(&ctx->Buffer,
                       '"'); // to not loose the " from the previous state
        sb_append_char(&ctx->Buffer, '"');
        sb_append_char(&ctx->Buffer, input_char);
        current_state = STATE_MULTILINE_STRING_BODY;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    default:
      assert(false && "Lex string state machine fell through!");
      return MAKE_TOKEN_ERROR();
      break;
    }
  }
}

static Token lex_div_com(LexerCtx *ctx) {
  State current_state = STATE_SLASH;
  int comment_nesting = 0;
  while (1) {
    int c = fgetc(ctx->input);
    DEBUG_LOG("current state: %d\n", current_state);
    DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
    switch (current_state) {
    case STATE_SLASH:
      if (c == '/') {
        current_state = STATE_LINE_COMMENT;
      } else if (c == '*') {
        current_state = STATE_BLOCK_COMMENT;
      } else {
        ungetc(c, ctx->input);
        return MAKE_TOKEN(T_DIVIDE);
      }
      break;

    case STATE_LINE_COMMENT:
      if (c == '\n') {
        return MAKE_TOKEN(T_EOL);
      } else if (c == EOF) {
        return MAKE_TOKEN(T_EOF);
      }
      break;

    case STATE_BLOCK_COMMENT:
      if (c == '\n') {
        current_state = STATE_BLOCK_COMMENT_NEWLINE;
      } else if (c == '*') {
        current_state = STATE_BLOCK_COMMENT_STAR;
      } else if (c == '/') {
        current_state = STATE_BLOCK_COMMENT_SLASH;
      } else if (c == EOF) {
        return MAKE_TOKEN_ERROR();
      } else {
        current_state = STATE_BLOCK_COMMENT;
      }
      break;

    case STATE_BLOCK_COMMENT_STAR:
      if (c == '/') {
        if (comment_nesting == 0) {
          return MAKE_TOKEN(T_WHITESPACE_SEP);
        }
        comment_nesting--;
        current_state = STATE_BLOCK_COMMENT;
      } else if (c == '*') {
        continue;
      } else if (c == EOF) {
        ungetc(c, ctx->input);
        DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
        return MAKE_TOKEN_ERROR();
      } else if (c == '\n') {
        current_state = STATE_BLOCK_COMMENT_NEWLINE;
      } else {
        current_state = STATE_BLOCK_COMMENT;
      }
      break;

    case STATE_BLOCK_COMMENT_SLASH:
      if (c == '*') {
        comment_nesting++;
        DEBUG_LOG("Number of iterations %d", comment_nesting);
        current_state = STATE_BLOCK_COMMENT;
      } else if (c == '/') {
        continue;
      } else if (c == '\n') {
        current_state = STATE_BLOCK_COMMENT_NEWLINE;
      } else if (c == EOF) {
        return MAKE_TOKEN_ERROR();
      } else {
        current_state = STATE_BLOCK_COMMENT;
      }
      break;

    case STATE_BLOCK_COMMENT_NEWLINE:
      if (c == '*') {
        current_state = STATE_BLOCK_COMMENT_NEWLINE_STAR;
      } else if (c == '/') {
        current_state = STATE_BLOCK_COMMENT_NEWLINE_SLASH;
      } else if (c == EOF) {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_BLOCK_COMMENT_NEWLINE_STAR:
      if (c == '/') {
        if (comment_nesting == 0) {
          current_state = STATE_BLOCK_COMMENT_NEWLINE_END;
        } else {
          comment_nesting--;
          current_state = STATE_BLOCK_COMMENT_NEWLINE;
        }
      } else if (c == '*') {
        continue;
      } else if (c == EOF) {
        return MAKE_TOKEN_ERROR();
      } else {
        current_state = STATE_BLOCK_COMMENT_NEWLINE;
      }
      break;

    case STATE_BLOCK_COMMENT_NEWLINE_END:
      if (c == '\n' || c == EOF) {
        if (c == EOF) {
          ungetc(c, ctx->input);
          DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
        }
        return MAKE_TOKEN(T_EOL);
      } else if (c == ' ' || c == '\t' || c == '\r') {
        continue;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_BLOCK_COMMENT_NEWLINE_SLASH:
      if (c == '*') {
        comment_nesting++;
        current_state = STATE_BLOCK_COMMENT_NEWLINE;
      } else if (c == '/') {
        continue;
      } else if (c == EOF) {
        return MAKE_TOKEN_ERROR();
      } else {
        current_state = STATE_BLOCK_COMMENT_NEWLINE;
      }
      break;

    default:
      assert(false && "Lex comments state machine fell through!");
      return MAKE_TOKEN_ERROR();
      break;
    }
  }
}

static Token lex_equal(LexerCtx *ctx) {
  int c = fgetc(ctx->input);
  DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
  if (c == '=') {
    return MAKE_TOKEN(T_EQUAL);
  } else {
    ungetc(c, ctx->input);
    DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
    return MAKE_TOKEN(T_ASSIGN);
  }
}

static Token lex_compare_lesser(LexerCtx *ctx) {
  int c = fgetc(ctx->input);
  DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
  if (c == '=') {
    return MAKE_TOKEN(T_LTE);
  } else {
    ungetc(c, ctx->input);
    DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
    return MAKE_TOKEN(T_LT);
  }
}

static Token lex_compare_greater(LexerCtx *ctx) {
  int c = fgetc(ctx->input);
  DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
  if (c == '=') {
    return MAKE_TOKEN(T_GTE);
  } else {
    ungetc(c, ctx->input);
    DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
    return MAKE_TOKEN(T_GT);
  }
}

static Token lex_not_equal(LexerCtx *ctx) {
  int c = fgetc(ctx->input);
  DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
  if (c == '=') {
    return MAKE_TOKEN(T_NOT_EQUAL);
  } else {
    ungetc(c, ctx->input);
    DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
    return MAKE_TOKEN(T_NOT);
  }
}

static Token lex_and(LexerCtx *ctx) {
  int c = fgetc(ctx->input);
  DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
  if (c == '&') {
    return MAKE_TOKEN(T_AND);
  }
  ungetc(c, ctx->input);
  DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
  return MAKE_TOKEN_ERROR();
}

static Token lex_or(LexerCtx *ctx) {
  int c = fgetc(ctx->input);
  DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
  if (c == '|') {
    return MAKE_TOKEN(T_OR);
  }
  ungetc(c, ctx->input);
  DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
  return MAKE_TOKEN_ERROR();
}

static Token lex_main_func(LexerCtx *ctx) {
  State current_state = STATE_AT;
  while (1) {
    int c = fgetc(ctx->input);
    DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
    switch (current_state) {
    case STATE_AT:
      if (c == 'm') {
        current_state = STATE_AT_M;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_AT_M:
      if (c == 'a') {
        current_state = STATE_AT_MA;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_AT_MA:
      if (c == 'i') {
        current_state = STATE_AT_MAI;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_AT_MAI:
      if (c == 'n') {
        return MAKE_TOKEN(T_FUNC_MAIN);
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    default:
      assert(false && "Lex '@main' state machine fell through!");
      return MAKE_TOKEN_ERROR();
      break;
    }
  }
}

static Token interp_error(LexerCtx *ctx) {
  ctx->interp_depth = 0;
  queue_clear(ctx);
  sb_reset(&ctx->Buffer);
  return MAKE_TOKEN_ERROR();
}

static bool interp_begin_part(LexerCtx *ctx) {
  InterpLevel *lv = &INTERP_TOP(ctx);
  if (lv->has_part && !queue_push(ctx, MAKE_TOKEN(T_PLUS_SIGN))) {
    return false;
  }
  lv->has_part = true;
  return true;
}

static bool interp_flush_literal(LexerCtx *ctx) {
  if (ctx->Buffer.length == 0) {
    return true;
  }
  if (!interp_begin_part(ctx)) {
    return false;
  }
  Token t = MAKE_TOKEN_LEXEME(T_STRING_LITERAL, &ctx->Buffer);
  sb_reset(&ctx->Buffer);
  if (!queue_push(ctx, t)) {
    discard_token(t);
    return false;
  }
  return true;
}

static bool interp_push_tostr_open(LexerCtx *ctx) {
  return queue_push_lexeme(ctx, T_IDENTIFICATOR, "toStr") &&
         queue_push(ctx, MAKE_TOKEN(T_OPEN_BRACKET)) &&
         queue_push(ctx, MAKE_TOKEN(T_OPEN_BRACKET));
}

static bool interp_push_tostr_close(LexerCtx *ctx) {
  return queue_push(ctx, MAKE_TOKEN(T_CLOSE_BRACKET)) &&
         queue_push(ctx, MAKE_TOKEN(T_CLOSE_BRACKET));
}

static Token interp_finish(LexerCtx *ctx) {
  bool ok = interp_flush_literal(ctx);
  if (ok && !INTERP_TOP(ctx).has_part) {
    ok = queue_push_lexeme(ctx, T_STRING_LITERAL, "");
  }
  if (ok) {
    ok = queue_push(ctx, MAKE_TOKEN(T_CLOSE_BRACKET));
  }
  if (!ok) {
    return interp_error(ctx);
  }
  ctx->interp_depth--;
  sb_reset(&ctx->Buffer);
  return queue_pop(ctx);
}

static Token interp_begin_expr(LexerCtx *ctx) {
  if (!interp_flush_literal(ctx) || !interp_begin_part(ctx) ||
      !interp_push_tostr_open(ctx)) {
    return interp_error(ctx);
  }
  INTERP_TOP(ctx).phase = IP_EXPR;
  INTERP_TOP(ctx).expr_tokens = 0;
  sb_reset(&ctx->Buffer);
  return queue_pop(ctx);
}

static Token interp_emit_ident(LexerCtx *ctx) {
  if (ctx->Buffer.length == 1 && ctx->Buffer.data[0] == '_') {
    return interp_error(ctx);
  }
  if (!interp_begin_part(ctx) || !interp_push_tostr_open(ctx)) {
    return interp_error(ctx);
  }
  Token id = MAKE_TOKEN_LEXEME(T_IDENTIFICATOR, &ctx->Buffer);
  sb_reset(&ctx->Buffer);
  if (!queue_push(ctx, id)) {
    discard_token(id);
    return interp_error(ctx);
  }
  if (!interp_push_tostr_close(ctx)) {
    return interp_error(ctx);
  }
  return queue_pop(ctx);
}

static Token interp_open(LexerCtx *ctx) {
  if (ctx->interp_depth >= INTERP_MAX_DEPTH) {
    return interp_error(ctx);
  }
  if (ctx->interp_depth > 0) {
    INTERP_TOP(ctx).expr_tokens++;
  }
  ctx->interp_depth++;
  INTERP_TOP(ctx) = (InterpLevel){.phase = IP_START,
                                  .multiline = false,
                                  .has_part = false,
                                  .expr_tokens = 0};
  if (!queue_push(ctx, MAKE_TOKEN(T_OPEN_BRACKET))) {
    return interp_error(ctx);
  }
  return queue_pop(ctx);
}

static Token interp_close_expr(LexerCtx *ctx) {
  if (INTERP_TOP(ctx).expr_tokens == 0) {
    return interp_error(ctx);
  }
  if (!interp_push_tostr_close(ctx)) {
    return interp_error(ctx);
  }
  INTERP_TOP(ctx).phase = IP_BODY;
  return queue_pop(ctx);
}

static Token lex_interp_literal(LexerCtx *ctx) {
  InterpLevel *lv = &INTERP_TOP(ctx);
  State current_state =
      (lv->phase == IP_START) ? STATE_INTERP_STRING : STATE_INTERP_STRING_BODY;
  int run = 0;
  sb_reset(&ctx->Buffer);
  while (1) {
    int c = fgetc(ctx->input);
    switch (current_state) {
    case STATE_INTERP_STRING:
      if (c == '"') {
        current_state = STATE_INTERP_STRING_QUOTE;
      } else {
        ungetc(c, ctx->input);
        lv->phase = IP_BODY;
        current_state = STATE_INTERP_STRING_BODY;
      }
      break;

    case STATE_INTERP_STRING_QUOTE:
      if (c == '"') {
        lv->multiline = true;
        lv->phase = IP_BODY;
        current_state = STATE_INTERP_STRING_BODY;
      } else {
        ungetc(c, ctx->input);
        return interp_finish(ctx);
      }
      break;

    case STATE_INTERP_STRING_BODY:
      if (c == '"') {
        if (lv->multiline) {
          run = 1;
          current_state = STATE_INTERP_STRING_QUOTES;
        } else {
          return interp_finish(ctx);
        }
      } else if (c == '$') {
        current_state = STATE_INTERP_STRING_DOLLAR;
      } else if (c == '\\' && !lv->multiline) {
        current_state = STATE_INTERP_STRING_ESCAPE;
      } else if (lv->multiline ? (c == '\n' || c == '\r' || c == '\t' ||
                                  (c >= ' ' && c != 127))
                               : (c >= ' ' && c < 127)) {
        sb_append_char(&ctx->Buffer, (char)c);
      } else {
        return interp_error(ctx);
      }
      break;

    case STATE_INTERP_STRING_ESCAPE:
      if (c == 'n') {
        sb_append_char(&ctx->Buffer, '\n');
      } else if (c == 't') {
        sb_append_char(&ctx->Buffer, '\t');
      } else if (c == 'r') {
        sb_append_char(&ctx->Buffer, '\r');
      } else if (c == '\\') {
        sb_append_char(&ctx->Buffer, '\\');
      } else if (c == '"') {
        sb_append_char(&ctx->Buffer, '"');
      } else {
        return interp_error(ctx);
      }
      current_state = STATE_INTERP_STRING_BODY;
      break;

    case STATE_INTERP_STRING_QUOTES:
      if (c == '"') {
        run++;
      } else {
        ungetc(c, ctx->input);
        if (run >= 3) {
          for (int i = 0; i < run - 3; i++) {
            sb_append_char(&ctx->Buffer, '"');
          }
          return interp_finish(ctx);
        }
        for (int i = 0; i < run; i++) {
          sb_append_char(&ctx->Buffer, '"');
        }
        current_state = STATE_INTERP_STRING_BODY;
      }
      break;

    case STATE_INTERP_STRING_DOLLAR:
      if (c == '$') {
        sb_append_char(&ctx->Buffer, '$');
        current_state = STATE_INTERP_STRING_BODY;
      } else if (c == '{') {
        return interp_begin_expr(ctx);
      } else if (c == '_' || isalpha(c)) {
        if (!interp_flush_literal(ctx)) {
          return interp_error(ctx);
        }
        sb_reset(&ctx->Buffer);
        sb_append_char(&ctx->Buffer, (char)c);
        current_state = STATE_INTERP_STRING_ID;
      } else {
        return interp_error(ctx);
      }
      break;

    case STATE_INTERP_STRING_ID:
      if (c == '_' || isalnum(c)) {
        sb_append_char(&ctx->Buffer, (char)c);
      } else {
        ungetc(c, ctx->input);
        return interp_emit_ident(ctx);
      }
      break;

    default:
      assert(false && "Lex interpolated string state machine fell through!");
      return interp_error(ctx);
    }
  }
}

static Token lex_token(LexerCtx *ctx) {
  int c;
  sb_reset(&ctx->Buffer);
  c = fgetc(ctx->input);
  DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
  if (c == EOF) {
    return MAKE_TOKEN(T_EOF);
  }
  if (c == '\r') {
    c = fgetc(ctx->input);
  }
  if (c == ' ') {
    DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
    ungetc(c, ctx->input);
    return lex_white_space(ctx);
  }
  if (c == '\t') {
    while (c == '\t') {
      c = fgetc(ctx->input);
    }
    ungetc(c, ctx->input);
    return MAKE_TOKEN_TAB();
  }
  if (c == '_' || isalpha(c)) {
    DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
    ungetc(c, ctx->input);
    Token id = lex_identificator(ctx);
    if (id.token_type == T_IDENTIFICATOR &&
        strcmp(id.token_lexeme.lexeme, "s") == 0) {
      int quote = fgetc(ctx->input);
      if (quote == '"') {
        discard_token(id);
        return interp_open(ctx);
      }
      ungetc(quote, ctx->input);
    }
    return id;
  }
  if (c == '\n') {
    return MAKE_TOKEN(T_EOL);
  }
  if (c == '0' || isdigit(c)) {
    DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
    ungetc(c, ctx->input);
    return lex_numbers(ctx);
  }
  if (c == '+') {
    return MAKE_TOKEN(T_PLUS_SIGN);
  }
  if (c == '-') {
    return MAKE_TOKEN(T_MINUS_SIGN);
  }
  if (c == '"') {
    ungetc(c, ctx->input);
    DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
    return lex_string(ctx);
  }
  if (c == ',') {
    return MAKE_TOKEN(T_COMMA);
  }
  if (c == ':') {
    return MAKE_TOKEN(T_DOUBLE_DOT);
  }
  if (c == '*') {
    return MAKE_TOKEN(T_MULT_SIGN);
  }
  if (c == '/') {
    return lex_div_com(ctx);
  }
  if (c == '=') {
    return lex_equal(ctx);
  }
  if (c == '>') {
    return lex_compare_greater(ctx);
  }
  if (c == '<') {
    return lex_compare_lesser(ctx);
  }
  if (c == '!') {
    return lex_not_equal(ctx);
  }
  if (c == '&') {
    return lex_and(ctx);
  }
  if (c == '|') {
    return lex_or(ctx);
  }
  if (c == '.') {
    int c2 = fgetc(ctx->input);
    ungetc(c2, ctx->input);
    if (isdigit(c2)) {
      ungetc(c, ctx->input);
      return lex_numbers(ctx);
    }
    return MAKE_TOKEN(T_DOT);
  }
  if (c == '(') {
    return MAKE_TOKEN(T_OPEN_BRACKET);
  }
  if (c == ')') {
    return MAKE_TOKEN(T_CLOSE_BRACKET);
  }
  if (c == ';') {
    return MAKE_TOKEN(T_SEMICOLON);
  }
  if (c == '@') {
    return lex_main_func(ctx);
  }
  if (c == '}' && ctx->interp_depth > 0 && INTERP_TOP(ctx).phase == IP_EXPR) {
    return interp_close_expr(ctx);
  }

  return MAKE_TOKEN_ERROR();
}

Token get_next_token(LexerCtx *ctx) {
  while (1) {
    if (ctx->pending_pos < ctx->pending_len) {
      return queue_pop(ctx);
    }
    if (ctx->interp_depth > 0 && INTERP_TOP(ctx).phase != IP_EXPR) {
      return lex_interp_literal(ctx);
    }

    int depth_before = ctx->interp_depth;
    Token t = lex_token(ctx);
    if (depth_before == 0) {
      return t;
    }
    if (t.token_type == T_ERROR) {
      return interp_error(ctx);
    }
    if (ctx->interp_depth != depth_before || INTERP_TOP(ctx).phase != IP_EXPR) {
      return t;
    }
    switch (t.token_type) {
    case T_INDENTS:
    case T_TAB:
    case T_WHITESPACE_SEP:
      continue;
    case T_EOL:
      if (INTERP_TOP(ctx).multiline) {
        continue;
      }
      return interp_error(ctx);
    case T_EOF:
      return interp_error(ctx);
    default:
      INTERP_TOP(ctx).expr_tokens++;
      return t;
    }
  }
}
