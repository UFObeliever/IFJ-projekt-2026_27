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
#define MAKE_TOKEN_MISC(TYPE)                                                  \
  (Token) { .token_type = (TYPE) }

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

#ifdef NDEBUG
#define DEBUG_LOG(fmt, ...) ((void)0)
#else
#define DEBUG_LOG(fmt, ...)                                                    \
  fprintf(stderr, "[%s:%s:%d]", __FILE__, __func__, __LINE__);                 \
  fprintf(stderr, fmt, ##__VA_ARGS__);
#endif

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
    DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);

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
      DEBUG_LOG("putting back char: '%c' (%d)\n", (char)c, c);
      break;
    }
  }
  return MAKE_TOKEN_INDENTS(T_INDENTS, num_indents);
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
      } else if (c == '-') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_MINUS;
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

    case S_MINUS:
      if (c == '0') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_INT_ZERO;
      } else if (c >= '1' && c <= '9') {
        sb_append_char(&ctx->Buffer, input_char);
        current_state = S_INT;
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

// Token lex_div_com(LexerCtx *ctx) {
//   State current_state = S_BACKSLASH_COMM;
//   int comment_nesting = 0;
//   bool has_new_line = false;
//   while (1) {
//     int c = fgetc(ctx->input);
//     DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
//     switch (current_state) {
//     case S_BACKSLASH_COMM:
//       if (c == '/') {
//         current_state = S_SINGLE_COMM_START;
//       } else if (c == '*') {
//         comment_nesting++;
//         current_state = S_START_BLOCK_COMMENT_1;
//       } else {
//         return MAKE_TOKEN_MISC(T_DIVIDE);
//       }
//       break;

//     case S_SINGLE_COMM_START:
//       if (c == '\n' || c == EOF) {
//         return MAKE_TOKEN_MISC(T_EOL);
//       }
//       break;

//     case S_START_BLOCK_COMMENT_1:
//       DEBUG_LOG("S_START_BLOCK_COMMNET_1\n");
//       if (c == EOF) {
//         return MAKE_TOKEN_ERROR();
//       } else if (c == '/') {
//         current_state = S_IN_BLOCK_COMMENT;
//       } else if (c == '*') {
//         current_state = S_BLOCK_COMMENT_END_1;
//       } else if (c == '\n') {
//         has_new_line = true;
//       }
//       break;

//     case S_IN_BLOCK_COMMENT:
//       DEBUG_LOG("S_IN_BLOCK_COMMENT\n");
//       if (c == EOF) {
//         return MAKE_TOKEN_ERROR();
//       }
//       if (c == '*') {
//         comment_nesting++;
//       }
//       if (c == '\n') {
//         has_new_line = true;
//       }
//       current_state = S_START_BLOCK_COMMENT_1;
//       break;

//     case S_BLOCK_COMMENT_END_1:
//       DEBUG_LOG("S_BLOCK_COMMENT_END_1\n");
//       if (c == EOF) {
//         return MAKE_TOKEN_ERROR();
//       }
//       if (c == '\n') {
//         has_new_line = true;
//       }
//       if (c == '/') {
//         comment_nesting--;
//         if (comment_nesting == 0) {
//           if (has_new_line) {
//             return MAKE_TOKEN_MISC(T_WHITESPACE_SEP);
//           }
//           current_state = S_BLOCK_COMMENT_END_2;
//         }
//       } else {
//         current_state = S_START_BLOCK_COMMENT_1;
//       }
//       break;
//     case S_BLOCK_COMMENT_END_2:
//       DEBUG_LOG("S_BLOCK_COMMENT_END_2\n");
//       if (c == '\n' || c == EOF) {
//         return MAKE_TOKEN_WHITESPACE(T_EOL);
//       } else if (c == ' ' || c == '\t' || c == '\r') {
//         continue; // ignore this
//       } else {
//         return MAKE_TOKEN_ERROR();
//       }
//     default:
//       // should never happen!!!!
//       assert(false && "Lex comment state machine fell through!");
//       return MAKE_TOKEN_ERROR();
//       break;
//     }
//   }
// }

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
  if (c == '=') {
    return MAKE_TOKEN_MISC(T_EQUAL);
  } else {
    ungetc(c, ctx->input);
    DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
    return MAKE_TOKEN_MISC(T_ASSIGN);
  }
}

Token lex_compare_l(LexerCtx *ctx) {
  int c = fgetc(ctx->input);
  DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
  if (c == '=') {
    return MAKE_TOKEN_MISC(T_LTE);
  } else {
    ungetc(c, ctx->input);
    DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
    return MAKE_TOKEN_MISC(T_LT);
  }
}

Token lex_compare_g(LexerCtx *ctx) {
  int c = fgetc(ctx->input);
  DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
  if (c == '=') {
    return MAKE_TOKEN_MISC(T_GTE);
  } else {
    ungetc(c, ctx->input);
    DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
    return MAKE_TOKEN_MISC(T_GT);
  }
}

Token lex_not_equal(LexerCtx *ctx) {
  int c = fgetc(ctx->input);
  DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
  if (c == '=') {
    return MAKE_TOKEN_MISC(T_NOT_EQUAL);
  } else {
    return MAKE_TOKEN_ERROR();
  }
}

Token get_next_token(LexerCtx *ctx) {
  int c;
  sb_reset(&ctx->Buffer);
  c = fgetc(ctx->input);
  DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
  if (c == EOF) {
    return MAKE_TOKEN_WHITESPACE(T_EOF);
  }

  /* dopsat stavy z jednotlivých FSM */
  if (c == ' ') {
    if (ctx->at_start_of_line) {
      DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
      ungetc(c, ctx->input);
      return lex_indents(ctx);
    }
  }

  // skip all incomming white space
  ungetc(c, ctx->input);
  DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
  c = skip_white_space(ctx);
  ctx->at_start_of_line = false;

  if (c == '_' || isalpha(c)) {
    DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
    ungetc(c, ctx->input);
    return lex_identificator(ctx);
  }

  if (c == '\n') {
    ctx->at_start_of_line = true;
    return MAKE_TOKEN_WHITESPACE(T_EOL);
  }

  if (c == '0' || isdigit(c) || c == '-') {
    DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
    ungetc(c, ctx->input);
    return lex_numbers(ctx);
  }

  // stringy
  if (c == '"') {
    ungetc(c, ctx->input);
    DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
    return lex_string(ctx);
  }
  if (c == ',') {
    return MAKE_TOKEN_MISC(T_COMMA);
  }
  if (c == ':') {
    return MAKE_TOKEN_MISC(T_DOUBLE_DOT);
  }

  if (c == '*') {
    return MAKE_TOKEN_MISC(T_MULT_SIGN);
  }

  if (c == '+') {
    return MAKE_TOKEN_MISC(T_PLUS_SIGN);
  }

  if (c == '-') {
    return MAKE_TOKEN_MISC(T_MINUS_SIGN);
  }

  if (c == '/') {
    // return lex_div_com(ctx);
    // TO DO DO Dodělat ty opice
  }

  if (c == '=') {
    return lex_equal(ctx);
  }
  if (c == '>') {
    return lex_compare_l(ctx);
  }
  if (c == '<') {
    return lex_compare_g(ctx);
  }

  if (c == '!') {
    return lex_not_equal(ctx);
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
    [T_OPEN_PAREN] = "T_OPEN_PAREN",
    [T_CLOSE_PAREN] = "T_CLOSE_PAREN",
    [T_OPEN_CURLY_BRACES] = "T_OPEN_CURLY_BRACES",
    [T_CLOSE_CURLY_BRACES] = "T_CLOSE_CURLY_BRACES",
    [T_COMMA] = "T_COMMA",
    [T_EQUAL] = "T_EQUAL",
    [T_ASSIGN] = "T_ASSIGN",
    [T_DIVIDE] = "T_DIVIDE",
    [T_LT] = "T_LT",
    [T_LTE] = "T_LTE",
    [T_GT] = "T_GT",
    [T_GTE] = "T_GTE",
    [T_FUNCTION] = "T_FUNCTION",
    [T_INDENTS] = "T_INDENTS",
    [T_ERROR] = "T_ERROR",
    [T_EOF] = "T_EOF",
    [T_EOL] = "T_EOL",
};

const char *token_to_string(TokenType token) {
  return token_to_string_table[token];
}
