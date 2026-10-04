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
 * @brief Makro na vytvoření dorbnějších tokenů (T_EQUAL, T_LTE, atd.)
 *
 * @param TYPE: typ tokenu
 */
#define MAKE_TOKEN(TYPE)                                                       \
  (Token) { .token_type = (TYPE) }

/**
 * @brief Makro na vytvoření tokenu odsadzení
 *
 * @param TYPE: typ tokenu
 * @param COUNT: počet odsadzení
 *
 */
#define MAKE_TOKEN_INDENTS(COUNT)                                              \
  (Token) {                                                                    \
    .token_indents = {.type = T_INDENTS, .count = (COUNT) }                    \
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

#ifdef NDEBUG
#define DEBUG_LOG(fmt, ...) ((void)0)
#else
#define DEBUG_LOG(fmt, ...)                                                    \
  fprintf(stderr, "[%s:%d](%s)", __FILE__, __LINE__, __func__);                \
  fprintf(stderr, fmt, ##__VA_ARGS__);
#endif

void lexer_init(LexerCtx *ctx, FILE *input) {
  ctx->input = input;
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

Token lex_white_space(LexerCtx *ctx) {
  int num_indents = 0;
  State current_state = S_INDENT_START;
  while (1) {
    int c = fgetc(ctx->input);
    DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
    switch (current_state) {
    case S_INDENT_START:
      if (c == '\t') {
        current_state = S_INDENT_HAS_TAB;
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
    case S_INDENT_HAS_TAB:
      if (c == '\n') {
        return MAKE_TOKEN(T_EOL);
      } else if (c == ' ' || c == '\t' || c == '\r') {
        continue; // stay in this state
      } else if (c == EOF) {
        return MAKE_TOKEN(T_EOF);
      } else {
        return MAKE_TOKEN(T_WHITESPACE_SEP);
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

Token lex_identificator(LexerCtx *ctx) {
  State current_state = S_START;
  while (1) {
    int c = fgetc(ctx->input);
    DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);

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

Token lex_numbers(LexerCtx *ctx) {
  State current_state = S_START;

  while (1) {
    int c = fgetc(ctx->input);
    DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
    char input_char = (char)c;
    switch (current_state) {
    case S_START:
      if (c == '0') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_INT_ZERO;
      } else if (c >= '1' && c <= '9') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_INT;
      } else if (c == '.') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_DOT_DOUBLE;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case S_INT_ZERO:
      if (isdigit(c)) {
        return MAKE_TOKEN_ERROR();
      } else if (c == '.') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_DOT_DOUBLE;
      } else if (c == 'e' || c == 'E') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_DOUBLE_E;
      } else {
        ungetc(c, ctx->input);
        DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
        return MAKE_TOKEN_LEXEME(T_INT_LITERAL, &ctx->Buffer);
      }
      break;

    case S_INT:
      if (c == '_') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_INT_UNDERSCORE;
      } else if (isdigit(c)) {
        sb_append_char(&ctx->Buffer, input_char);
        // loop, kdy dostává čísla
      } else if (c == '.') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_DOT_DOUBLE;
      } else if (c == 'e' || c == 'E') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_DOUBLE_E;
      } else {
        ungetc(c, ctx->input);
        DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
        return MAKE_TOKEN_LEXEME(T_INT_LITERAL, &ctx->Buffer);
      }
      break;

    case S_INT_UNDERSCORE:
      if (isdigit(c)) {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_INT;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case S_DOUBLE_E:
      if (isdigit(c)) {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_DOUBLE_AFTER_E;
      } else if (c == '-' || c == '+') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_DOUBLE_UNDERSCORE_AFTER_E;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case S_DOUBLE_E_SIGN:
      if (isdigit(c)) {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_DOUBLE_AFTER_E;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case S_DOUBLE_AFTER_E:
      if (isdigit(c)) {
        sb_append_char(&ctx->Buffer, input_char);
        // loop čísel
      } else if (c == '_') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_DOUBLE_UNDERSCORE_AFTER_E;
      } else if (c == 'd' || c == 'D') {
        sb_append_char(&ctx->Buffer, input_char);
        return MAKE_TOKEN_LEXEME(
            T_DOUBLE_LITERAL,
            &ctx->Buffer); // Tady nemusím dávat stav, kdy to jde do
                           // S_DOUBLE_END_D, jelikož to rovnou udělá int/double
      } else {
        ungetc(c, ctx->input);
        DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
        return MAKE_TOKEN_LEXEME(T_DOUBLE_LITERAL, &ctx->Buffer);
      }
      break;

    case S_DOUBLE_UNDERSCORE_AFTER_E:
      if (isdigit(c)) {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_DOUBLE_AFTER_E;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case S_DOT_DOUBLE:
      if (isdigit(c)) {
        sb_append_char(&ctx->Buffer, input_char);
      } else if (c == 'e' || c == 'E') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_DOUBLE_E;
      } else if (c == 'd' || c == 'D') {
        sb_append_char(&ctx->Buffer, input_char);
        return MAKE_TOKEN_LEXEME(T_DOUBLE_LITERAL, &ctx->Buffer);
      } else {
        ungetc(c, ctx->input);
        DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
        return MAKE_TOKEN_LEXEME(T_DOUBLE_LITERAL, &ctx->Buffer);
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

// nové FSM na stringy
Token lex_string(LexerCtx *ctx) {
  State current_state = S_START;
  while (1) {
    int c = fgetc(ctx->input);
    DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
    char input_char = (char)c;
    switch (current_state) {
    case S_START:
      if (c == '"') {
        current_state = S_STR_START;
      } else {
        // nemělo by nastat, jelikož vždy to bude " jinak bych tady nebyl
        return MAKE_TOKEN_ERROR();
      }
      break;

    case S_STR_START:
      if (c == '\\') {
        current_state = S_STR_BACKSLASH;
      } else if (c == '"') {
        current_state = S_STR_END;
      } else if (c >= ' ') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_STR_IN_STR;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case S_STR_IN_STR:
      if (c == '\\') {
        current_state = S_STR_BACKSLASH;
      } else if (c == '"') {
        current_state = S_STR_END;
      } else if (c >= ' ' && c < 127) {
        sb_append_char(&ctx->Buffer, input_char);
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case S_STR_BACKSLASH:
      if (c == 'n') {
        sb_append_char(&ctx->Buffer, '\n');
        current_state = S_STR_IN_STR;
      } else if (c == 't') {
        sb_append_char(&ctx->Buffer, '\t');
        current_state = S_STR_IN_STR;
      } else if (c == 'r') {
        sb_append_char(&ctx->Buffer, '\r');
        current_state = S_STR_IN_STR;
      } else if (c == '\\') {
        sb_append_char(&ctx->Buffer, '\\');
        current_state = S_STR_IN_STR;
      } else if (c == '"') {
        sb_append_char(&ctx->Buffer, '\"');
        current_state = S_STR_IN_STR;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case S_STR_END:
      if (c == '"') {
        current_state = S_STR_MULTILINE_START;
      } else {
        ungetc(c, ctx->input);
        DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
        return MAKE_TOKEN_LEXEME(T_STRING_LITERAL, &ctx->Buffer);
      }
      break;

    case S_STR_MULTILINE_START:
      if (c == '"') {
        current_state = S_MSTR_END_1;
      } else if (c == '\n' || c == '\r' || c == '\t' ||
                 (c >= ' ' && c != 127)) {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_MSTR_IN_STR;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case S_MSTR_IN_STR:
      if (c == '"') {
        current_state = S_MSTR_END_1;
      } else if (c == '\n' || c == '\r' || c == '\t' ||
                 (c >= ' ' && c != 127)) {
        sb_append_char(&ctx->Buffer, input_char);
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case S_MSTR_END_1:
      if (c == '"') {
        current_state = S_MSTR_END_2;
      } else if (c == '\n' || c == '\r' || c == '\t' ||
                 (c >= ' ' && c != 127)) {
        sb_append_char(&ctx->Buffer,
                       '"'); // to not loose the " from the previous state
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_MSTR_IN_STR;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case S_MSTR_END_2:
      if (c == '"') {
        return MAKE_TOKEN_LEXEME(T_STRING_LITERAL, &ctx->Buffer);
      } else if (c == '\n' || c == '\r' || c == '\t' ||
                 (c >= ' ' && c != 127)) {
        sb_append_char(&ctx->Buffer,
                       '"'); // to not loose the " from the previous state
        sb_append_char(&ctx->Buffer, '"');
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_MSTR_IN_STR;
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

Token lex_div_com(LexerCtx *ctx) {
  State current_state = S_BACKSLASH_COMM;
  int comment_nesting = 0;

  while (1) {
    int c = fgetc(ctx->input);
    DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
    switch (current_state) {
    case S_BACKSLASH_COMM:
      if (c == '/') {
        current_state = S_IN_COM;
      } else if (c == '*') {
        current_state = S_MCOM_SINGLE;
      } else {
        return MAKE_TOKEN(T_DIVIDE);
      }
      break;

    case S_IN_COM:
      if (c == '\n') {
        return MAKE_TOKEN(T_EOL);
      } else if (c == EOF) {
        return MAKE_TOKEN(T_EOF);
      }
      break;

    case S_MCOM_SINGLE:
      if (c == '\n') {
        current_state = S_MCOM_MULTILINE;
      } else if (c == '*') {
        current_state = S_MCOM_SINGLE_END_STAR;
      } else if (c == '/') {
        current_state = S_MCOM_SINGLE_NEST_BACKSLASH;
      } else if (c == EOF) {
        return MAKE_TOKEN_ERROR();
      } else {
        current_state = S_MCOM_SINGLE;
      }
      break;

    case S_MCOM_SINGLE_END_STAR:
      if (c == '/') {
        return MAKE_TOKEN(T_WHITESPACE_SEP); // S_MSTR_SINGLE_END mělo by stačit
      } else if (c == EOF) {
        ungetc(c, ctx->input);
        DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
        return MAKE_TOKEN_ERROR();
      } else if (c == '\n') {
        current_state = S_MCOM_MULTILINE;
      } else {
        current_state = S_MCOM_SINGLE;
      }
      break;

    case S_MCOM_SINGLE_NEST_BACKSLASH:
      if (c == '*') {
        comment_nesting++;
        current_state = S_MCOM_SINGLE_NEST_STAR_START;
      } else if (c == '\n') {
        current_state = S_MCOM_MULTILINE;
      } else {
        current_state = S_MCOM_SINGLE;
      }
      break;

    case S_MCOM_SINGLE_NEST_STAR_START:
      if (c == '*') {
        current_state = S_MCOM_SINGLE_NEST_STAR_STAR;
      } else if (c == '\n') {
        current_state = S_MCOM_MULTILINE;
      } else {
        current_state = S_MCOM_SINGLE_NEST_STAR_START; // continue teoreticky
      }
      break;

    case S_MCOM_SINGLE_NEST_STAR_STAR:
      if (c == '/') {
        comment_nesting--;
        if (comment_nesting == 0) {
          current_state = S_MCOM_SINGLE;
        } else {
          current_state = S_MCOM_SINGLE_NEST_BACKSLASH;
        }
      } else if (c == '\n') {
        current_state = S_MCOM_MULTILINE;
      } else {
        current_state = S_MCOM_SINGLE_NEST_STAR_STAR;
      }
      break;

    case S_MCOM_MULTILINE:
      if (c == '*') {
        current_state = S_MCOM_MULT_STAR;
      } else if (c == '/') {
        current_state = S_MCOM_MULT_NEST_BACKSLASH;
      } else if (c == EOF) {
        return MAKE_TOKEN_ERROR();
      } else {
        current_state = S_MCOM_MULTILINE;
      }
      break;

    case S_MCOM_MULT_STAR:
      if (c == '/') {
        // nepotřebné ? current_state = S_MCOM_MULT_BACKSLASH;
        if (comment_nesting == 0) {
          current_state = S_MCOM_MULT_END;
        } else {
          comment_nesting--;
          current_state = S_MCOM_MULTILINE;
        }
      } else {
        current_state = S_MCOM_MULTILINE;
      }
      break;

    case S_MCOM_MULT_END:
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

    case S_MCOM_MULT_NEST_BACKSLASH:
      if (c == '*') {
        comment_nesting++;
      }
      current_state = S_MCOM_MULTILINE;
      break;

    default:
      assert(false && "Lex comments state machine fell through!");
      return MAKE_TOKEN_ERROR();
      break;
    }
  }
}

int skip_white_space(LexerCtx *ctx) {
  while (1) {
    int c = fgetc(ctx->input);
    DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
    if (c == ' ' || c == '\r' || c == '\t') {
      continue;
    }
    return c;
  }
}

Token lex_equal(LexerCtx *ctx) {
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

Token lex_compare_l(LexerCtx *ctx) {
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

Token lex_compare_g(LexerCtx *ctx) {
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

Token lex_not_equal(LexerCtx *ctx) {
  int c = fgetc(ctx->input);
  DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
  if (c == '=') {
    return MAKE_TOKEN(T_NOT_EQUAL);
  } else {
    return MAKE_TOKEN_ERROR();
  }
}

Token lex_main_func(LexerCtx *ctx) {
  State current_state = S_MAIN_AT;
  while (1) {
    int c = fgetc(ctx->input);
    DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
    switch (current_state) {
    case S_MAIN_AT:
      if (c == 'm') {
        current_state = S_MAIN_M;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case S_MAIN_M:
      if (c == 'a') {
        current_state = S_MAIN_A;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case S_MAIN_A:
      if (c == 'i') {
        current_state = S_MAIN_I;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case S_MAIN_I:
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

Token lex_unit(LexerCtx *ctx) {
  int c = fgetc(ctx->input);
  DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
  if (c == ')') {
    return MAKE_TOKEN(T_UNIT_LITERAL);
  } else {
    DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
    ungetc(c, ctx->input);
    return MAKE_TOKEN(T_OPEN_BRACKET);
  }
}

Token get_next_token(LexerCtx *ctx) {
  int c;
  sb_reset(&ctx->Buffer);
  c = fgetc(ctx->input);
  DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
  if (c == EOF) {
    return MAKE_TOKEN(T_EOF);
  }

  if (c == '\r') { // ignore this character
    c = fgetc(ctx->input);
  }

  /* dopsat stavy z jednotlivých FSM */
  if (c == ' ' || c == '\t') {
    DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
    ungetc(c, ctx->input);
    return lex_white_space(ctx);
  }

  if (c == '_' || isalpha(c)) {
    DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
    ungetc(c, ctx->input);
    return lex_identificator(ctx);
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
  // stringy
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
    return lex_compare_g(ctx);
  }
  if (c == '<') {
    return lex_compare_l(ctx);
  }

  if (c == '!') {
    return lex_not_equal(ctx);
  }
  if (c == '.') {
    return MAKE_TOKEN(T_DOT);
  }
  if (c == '(') {
    return MAKE_TOKEN(T_OPEN_BRACKET);
  }
  if (c == ';') {
    return MAKE_TOKEN(T_SEMICOLON);
  }

  if (c == '@') {
    return lex_main_func(ctx);
  }

  return MAKE_TOKEN_ERROR();
}

// Testing purposes
static const char *token_to_string_table[] = {
    [T_INT_LITERAL] = "T_INT_LITERAL",
    [T_DOUBLE_LITERAL] = "T_DOUBLE_LITERAL",
    [T_STRING_LITERAL] = "T_STRING_LITERAL",
    [T_IDENTIFICATOR] = "T_IDENTIFICATOR",
    [T_KW_DEF] = "T_KW_DEF",
    [T_KW_DO] = "T_KW_DO",
    [T_KW_DOUBLE] = "T_KW_DOUBLE",
    [T_KW_ELSE] = "T_KW_ELSE",
    [T_KW_IF] = "T_KW_IF",
    [T_KW_IMPORT] = "T_KW_IMPORT",
    [T_KW_INT] = "T_KW_INT",
    [T_KW_RETURN] = "T_KW_RETURN",
    [T_KW_STRING] = "T_KW_STRING",
    [T_KW_THEN] = "T_KW_THEN",
    [T_KW_UNIT] = "T_KW_UNIT",
    [T_KW_VAL] = "T_KW_VAL",
    [T_KW_VAR] = "T_KW_VAR",
    [T_KW_WHILE] = "T_KW_WHILE",
    [T_NOT_EQUAL] = "T_NOT_EQUAL",
    [T_PLUS_SIGN] = "T_PLUS_SIGN",
    [T_MINUS_SIGN] = "T_MINUS_SIGN",
    [T_MULT_SIGN] = "T_MULT_SIGN",
    [T_OPEN_BRACKET] = "T_OPEN_BRACKET",
    [T_CLOSE_BRACKET] = "T_CLOSE_BRACKET",
    [T_COMMA] = "T_COMMA",
    [T_EQUAL] = "T_EQUAL",
    [T_ASSIGN] = "T_ASSIGN",
    [T_DIVIDE] = "T_DIVIDE",
    [T_LT] = "T_LT",
    [T_LTE] = "T_LTE",
    [T_GT] = "T_GT",
    [T_GTE] = "T_GTE",
    [T_INDENTS] = "T_INDENTS",
    [T_ERROR] = "T_ERROR",
    [T_EOF] = "T_EOF",
    [T_EOL] = "T_EOL",
    [T_DOUBLE_DOT] = "T_DOUBLE_DOT",
    [T_WHITESPACE_SEP] = "T_WHITESPACE_SEP",
    [T_FUNC_MAIN] = "T_FUNC_MAIN",
    [T_DOT] = "T_DOT",
    [T_UNIT_LITERAL] = "T_UNIT_LITERAL",

};

const char *token_type_to_string(TokenType token) {
  return token_to_string_table[token];
}

void print_token(const Token *token) {
  const char *token_type_str = token_type_to_string(token->token_type);
  switch (token->token_type) {
  case T_INT_LITERAL:
  case T_DOUBLE_LITERAL:
  case T_STRING_LITERAL:
  case T_IDENTIFICATOR:
    printf("Token {type = %s, data = \"%s\"}", token_type_str,
           token->token_lexeme.lexeme);
    break;
  case T_KW_DEF:
  case T_KW_DO:
  case T_KW_DOUBLE:
  case T_KW_ELSE:
  case T_KW_IF:
  case T_KW_IMPORT:
  case T_KW_INT:
  case T_KW_RETURN:
  case T_KW_STRING:
  case T_KW_THEN:
  case T_KW_UNIT:
  case T_KW_VAL:
  case T_KW_VAR:
  case T_KW_WHILE:
  case T_NOT_EQUAL:
  case T_PLUS_SIGN:
  case T_MINUS_SIGN:
  case T_MULT_SIGN:
  case T_OPEN_BRACKET:
  case T_CLOSE_BRACKET:
  case T_COMMA:
  case T_EQUAL:
  case T_ASSIGN:
  case T_DIVIDE:
  case T_LT:
  case T_LTE:
  case T_GT:
  case T_GTE:
    printf("Token {type = %s}", token_type_str);
    break;
  case T_INDENTS:
    printf("Token {type = %s, indents = %d}", token_type_str,
           token->token_indents.count);
    break;
  case T_ERROR:
  case T_EOF:
  case T_EOL:
  case T_DOUBLE_DOT:
  case T_WHITESPACE_SEP:
  case T_FUNC_MAIN:
  case T_DOT:
  case T_UNIT_LITERAL:
    printf("Token {type = %s}", token_type_str);
    break;
  default:
    // should never happen!!
    assert(false && "print_token fell thourgh!!");
    break;
  }
}
