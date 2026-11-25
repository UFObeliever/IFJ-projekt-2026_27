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

/**
 * @brief Stavy konečných automatů lexeru.
 *
 * Každý automat (identifikátory, čísla, řetězce, komentáře, ...) používá jen
 * své stavy. Společný je počáteční STATE_START automatů pro identifikátory,
 * čísla a řetězce.
 *
 * Název stavu říká, co už bylo přečteno; komentář u stavu uvádí příklad
 * takového vstupu.
 */
typedef enum State {
  STATE_START,

  STATE_INDENT,
  STATE_INDENT_TAB,

  STATE_ID_UNDERSCORE,
  STATE_ID,

  STATE_ZERO,
  STATE_INT,
  STATE_INT_UNDERSCORE,
  STATE_DOUBLE_DOT,
  STATE_DOUBLE_FRACTION,
  STATE_DOUBLE_FRACTION_UNDERSCORE,
  STATE_DOUBLE_EXP_CHAR,
  STATE_DOUBLE_EXP_SIGN_OR_UNDERSCORE,
  STATE_DOUBLE_EXPONENT,

  STATE_STRING,
  STATE_STRING_BODY,
  STATE_STRING_ESCAPE,
  STATE_STRING_CLOSED,
  STATE_MULTILINE_STRING,
  STATE_MULTILINE_STRING_BODY,
  STATE_MULTILINE_STRING_QUOTE_1,
  STATE_MULTILINE_STRING_QUOTE_2,

  STATE_INTERP_STRING,
  STATE_INTERP_STRING_QUOTE,
  STATE_INTERP_STRING_BODY,
  STATE_INTERP_STRING_ESCAPE,
  STATE_INTERP_STRING_DOLLAR,
  STATE_INTERP_STRING_ID,
  STATE_INTERP_STRING_QUOTES,

  STATE_SLASH,
  STATE_LINE_COMMENT,
  STATE_BLOCK_COMMENT,
  STATE_BLOCK_COMMENT_STAR,
  STATE_BLOCK_COMMENT_SLASH,
  STATE_BLOCK_COMMENT_NEWLINE,
  STATE_BLOCK_COMMENT_NEWLINE_STAR,
  STATE_BLOCK_COMMENT_NEWLINE_SLASH,
  STATE_BLOCK_COMMENT_NEWLINE_END,

  STATE_AT,
  STATE_AT_M,
  STATE_AT_MA,
  STATE_AT_MAI,
} State;

/**
 * @brief Úroveň interpolace na vrcholu zásobníku.
 *
 * Výsledek lze číst i přiřazovat. Smí se použít jen při `interp_depth > 0`.
 *
 * @param ctx Kontext lexeru.
 */
#define INTERP_TOP(ctx) ((ctx)->interp[(ctx)->interp_depth - 1])

/**
 * @brief Vytvoří token s lexémem (literál nebo identifikátor).
 *
 * Lexém je kopie obsahu vyrovnávací paměti na haldě (sb_copy). Při nedostatku
 * paměti program skončí s kódem 99.
 *
 * @param TYPE Typ tokenu.
 * @param LEXEME Ukazatel na StringBuff, jehož obsah se zkopíruje.
 */
#define MAKE_TOKEN_LEXEME(TYPE, LEXEME)                                        \
  (Token) {                                                                    \
    .token_lexeme = {.type = (TYPE), .lexeme = sb_copy(LEXEME) }               \
  }

/**
 * @brief Vytvoří token bez další hodnoty (operátor, oddělovač, T_EOL, ...).
 *
 * @param TYPE Typ tokenu.
 */
#define MAKE_TOKEN(TYPE)                                                       \
  (Token) { .token_type = (TYPE) }

/**
 * @brief Vytvoří token odsazení T_INDENTS.
 *
 * @param COUNT Počet mezer na začátku řádku.
 */
#define MAKE_TOKEN_INDENTS(COUNT)                                              \
  (Token) {                                                                    \
    .token_indents = {.type = T_INDENTS, .count = (COUNT) }                    \
  }

/**
 * @brief Vytvoří token T_ERROR (lexikální chyba).
 */
#define MAKE_TOKEN_ERROR()                                                     \
  (Token) { .token_type = (T_ERROR) }

/**
 * @brief Vytvoří token klíčového slova.
 *
 * Rozvinutí je shodné s MAKE_TOKEN, makro jen zpřehledňuje kód.
 *
 * @param TYPE Typ tokenu klíčového slova.
 */
#define MAKE_TOKEN_KW(TYPE)                                                    \
  (Token) { .token_type = (TYPE) }

/**
 * @brief Porovná obsah vyrovnávací paměti s klíčovým slovem.
 *
 * Při shodě ukončí volající funkci (`return`) a vrátí token klíčového slova.
 * Makro používá proměnnou `ctx` z místa použití, smí se proto použít jen ve
 * funkci vracející Token, která ji má.
 *
 * Makro je zabalené do `do { } while (0)`, chová se tedy jako jeden příkaz;
 * středník píše až volající.
 *
 * @param TYPE Typ tokenu klíčového slova.
 * @param LEXEME Klíčové slovo jako řetězcový literál.
 */
#define KEYWORD_CHECK(TYPE, LEXEME)                                            \
  do {                                                                         \
    if (ctx->buffer.length == strlen(LEXEME) &&                                \
        strncmp(ctx->buffer.data, LEXEME, ctx->buffer.length) == 0) {          \
      return MAKE_TOKEN_KW(TYPE);                                              \
    }                                                                          \
  } while (0)

/**
 * @brief Vytvoří token T_TAB (jeden či více tabulátorů).
 */
#define MAKE_TOKEN_TAB()                                                       \
  (Token) { .token_type = (T_TAB) }

/**
 * @brief Uvolní lexém tokenu, pokud ho token nese.
 *
 * Lexém mají jen literály a identifikátory; u ostatních tokenů nedělá nic.
 *
 * @param t Token, jehož lexém se uvolní.
 */
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

/**
 * @brief Vyprázdní frontu tokenů.
 *
 * Uvolní lexémy dosud nevydaných tokenů. Pole `pending` zůstává alokované pro
 * další použití.
 *
 * @param ctx Kontext lexeru.
 */
static void queue_clear(LexerCtx *ctx) {
  for (size_t i = ctx->pending_pos; i < ctx->pending_len; i++) {
    discard_token(ctx->pending[i]);
  }
  ctx->pending_len = 0;
  ctx->pending_pos = 0;
}

/**
 * @brief Přidá token na konec fronty.
 *
 * Podle potřeby zvětší pole `pending` (začíná na 16 tokenech, pak kapacitu
 * zdvojnásobuje).
 *
 * @param ctx Kontext lexeru.
 * @param t Přidávaný token; fronta přebírá vlastnictví jeho lexému. Při selhání
 *   zůstává lexém volajícímu.
 * @return true při úspěchu, false při nedostatku paměti.
 */
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

/**
 * @brief Přidá do fronty token s lexémem zkopírovaným z řetězce.
 *
 * @param ctx Kontext lexeru.
 * @param type Typ vytvářeného tokenu.
 * @param text Řetězec ukončený nulovým znakem; zkopíruje se na haldu.
 * @return true při úspěchu, false při nedostatku paměti.
 */
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

/**
 * @brief Vyjme první token z fronty.
 *
 * Fronta nesmí být prázdná. Po vydání posledního tokenu se indexy fronty
 * vynulují.
 *
 * @param ctx Kontext lexeru.
 * @return Vyjmutý token; vlastnictví jeho lexému přechází na volajícího.
 */
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
  sb_init(&ctx->buffer, 16);

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
  sb_free(&ctx->buffer);
  ctx->input = NULL;
}

/**
 * @brief Rozhodne, zda načtený identifikátor je klíčové slovo.
 *
 * Porovná obsah vyrovnávací paměti se všemi klíčovými slovy jazyka (včetně
 * rozšíření).
 *
 * @param ctx Kontext lexeru; ve vyrovnávací paměti je identifikátor.
 * @return Token klíčového slova, jinak T_IDENTIFICATOR s lexémem.
 */
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

  return MAKE_TOKEN_LEXEME(T_IDENTIFICATOR, &ctx->buffer);
}

/**
 * @brief Zpracuje bílé znaky na začátku řádku.
 *
 * Volá se s první mezerou vrácenou zpět do vstupu. Spočítá úvodní mezery a
 * vrátí T_INDENTS s jejich počtem; první jiný znak vrátí do vstupu. Znak CR
 * ignoruje.
 *
 * Objeví-li se mezi mezerami tabulátor, je platný jen řádek tvořený samými
 * bílými znaky: skončí-li koncem řádku, vrátí T_EOL, koncem vstupu T_EOF, jinak
 * T_ERROR.
 *
 * @param ctx Kontext lexeru.
 * @return T_INDENTS, T_EOL, T_EOF nebo T_ERROR.
 */
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
        continue; // CR (konce řádků z Windows) ignorujeme
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
        continue; // zůstáváme ve stejném stavu
      } else if (c == EOF) {
        return MAKE_TOKEN(T_EOF);
      } else {
        ungetc(c, ctx->input);
        DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
        return MAKE_TOKEN_ERROR();
      }
      break;

    default:
      // nikdy by nemělo nastat!!
      assert(false && "State machine for indents fell through!");
      return MAKE_TOKEN_ERROR();
      break;
    }
  }
}

/**
 * @brief Načte identifikátor nebo klíčové slovo.
 *
 * Volá se s prvním znakem vráceným zpět do vstupu. Identifikátor tvoří písmena,
 * číslice a podtržítka; samotné podtržítko je chyba. První znak, který k
 * identifikátoru nepatří, se vrací do vstupu. Načtený text se předá funkci
 * keyword_check().
 *
 * @param ctx Kontext lexeru.
 * @return Token klíčového slova, T_IDENTIFICATOR s lexémem, nebo T_ERROR.
 */
static Token lex_identificator(LexerCtx *ctx) {
  State current_state = STATE_START;
  while (1) {
    int c = fgetc(ctx->input);
    DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);

    char input_char = (char)c;
    switch (current_state) {
    case STATE_START:
      if (c == '_') {
        sb_append_char(&ctx->buffer, input_char);
        current_state = STATE_ID_UNDERSCORE;
      }
      if (isalpha(c) || isdigit(c)) {
        sb_append_char(&ctx->buffer, input_char);
        current_state = STATE_ID;
      }
      break;
    case STATE_ID_UNDERSCORE:
      if (c == '_' || isalpha(c) || isdigit(c)) {
        sb_append_char(&ctx->buffer, input_char);
        current_state = STATE_ID;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;
    case STATE_ID:
      if (c == '_' || isalpha(c) || isdigit(c)) {
        sb_append_char(&ctx->buffer, input_char);
      } else {
        DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
        ungetc(c, ctx->input);
        return keyword_check(ctx);
      }
      break;
    default:
      // nikdy by nemělo nastat!!
      assert(false && "Lex Identificator state machine fell through!");
      return MAKE_TOKEN_ERROR();
      break;
    }
  }
}

/**
 * @brief Načte celočíselný nebo desetinný literál.
 *
 * Volá se s prvním znakem vráceným zpět do vstupu. Rozpoznává celá čísla (`12`,
 * `1_000`) a desetinná čísla (`1.5`, `.5`, `1e3`, `1.5e-3`); desetinný literál
 * smí končit příponou `d` nebo `D`. Podtržítko smí oddělovat číslice. Nula na
 * začátku je povolena jen u samotné nuly nebo před desetinnou tečkou či
 * exponentem.
 *
 * Lexém obsahuje zápis literálu přesně tak, jak je ve zdroji.
 *
 * @param ctx Kontext lexeru.
 * @return T_INT_LITERAL nebo T_DOUBLE_LITERAL s lexémem, jinak T_ERROR.
 */
static Token lex_numbers(LexerCtx *ctx) {
  State current_state = STATE_START;

  while (1) {
    int c = fgetc(ctx->input);
    DEBUG_LOG("dealing with char: '%c' (%d)\n", (char)c, c);
    char input_char = (char)c;
    switch (current_state) {
    case STATE_START:
      if (c == '0') {
        sb_append_char(&ctx->buffer, input_char);
        current_state = STATE_ZERO;
      } else if (c >= '1' && c <= '9') {
        sb_append_char(&ctx->buffer, input_char);
        current_state = STATE_INT;
      } else if (c == '.') {
        sb_append_char(&ctx->buffer, input_char);
        current_state = STATE_DOUBLE_DOT;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_ZERO:
      if (isdigit(c)) {
        return MAKE_TOKEN_ERROR();
      } else if (c == '.') {
        sb_append_char(&ctx->buffer, input_char);
        current_state = STATE_DOUBLE_DOT;
      } else if (c == 'e' || c == 'E') {
        sb_append_char(&ctx->buffer, input_char);
        current_state = STATE_DOUBLE_EXP_CHAR;
      } else {
        ungetc(c, ctx->input);
        return MAKE_TOKEN_LEXEME(T_INT_LITERAL, &ctx->buffer);
      }
      break;

    case STATE_INT:
      if (c == '_') {
        sb_append_char(&ctx->buffer, input_char);
        current_state = STATE_INT_UNDERSCORE;
      } else if (isdigit(c)) {
        sb_append_char(&ctx->buffer, input_char);
        // zůstáváme ve stavu, čteme další číslice
      } else if (c == '.') {
        sb_append_char(&ctx->buffer, input_char);
        current_state = STATE_DOUBLE_DOT;
      } else if (c == 'e' || c == 'E') {
        sb_append_char(&ctx->buffer, input_char);
        current_state = STATE_DOUBLE_EXP_CHAR;
      } else {
        ungetc(c, ctx->input);
        return MAKE_TOKEN_LEXEME(T_INT_LITERAL, &ctx->buffer);
      }
      break;

    case STATE_INT_UNDERSCORE:
      if (isdigit(c)) {
        sb_append_char(&ctx->buffer, input_char);
        current_state = STATE_INT;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_DOUBLE_EXP_CHAR:
      if (isdigit(c)) {
        sb_append_char(&ctx->buffer, input_char);
        current_state = STATE_DOUBLE_EXPONENT;
      } else if (c == '-' || c == '+') {
        sb_append_char(&ctx->buffer, input_char);
        current_state = STATE_DOUBLE_EXP_SIGN_OR_UNDERSCORE;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_DOUBLE_EXPONENT:
      if (isdigit(c)) {
        sb_append_char(&ctx->buffer, input_char);
        // zůstáváme ve stavu, čteme další číslice
      } else if (c == '_') {
        sb_append_char(&ctx->buffer, input_char);
        current_state = STATE_DOUBLE_EXP_SIGN_OR_UNDERSCORE;
      } else if (c == 'd' || c == 'D') {
        sb_append_char(&ctx->buffer, input_char);
        return MAKE_TOKEN_LEXEME(T_DOUBLE_LITERAL, &ctx->buffer);
      } else {
        ungetc(c, ctx->input);
        DEBUG_LOG("putting char back: '%c' (%d)\n", (char)c, c);
        return MAKE_TOKEN_LEXEME(T_DOUBLE_LITERAL, &ctx->buffer);
      }
      break;

    case STATE_DOUBLE_EXP_SIGN_OR_UNDERSCORE:
      if (isdigit(c)) {
        sb_append_char(&ctx->buffer, input_char);
        current_state = STATE_DOUBLE_EXPONENT;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_DOUBLE_DOT:
      if (isdigit(c)) {
        sb_append_char(&ctx->buffer, input_char);
        current_state = STATE_DOUBLE_FRACTION;
      } else {
        ungetc(c, ctx->input);
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_DOUBLE_FRACTION:
      if (isdigit(c)) {
        sb_append_char(&ctx->buffer, input_char);
      } else if (c == '_') {
        sb_append_char(&ctx->buffer, input_char);
        current_state = STATE_DOUBLE_FRACTION_UNDERSCORE;
      } else if (c == 'e' || c == 'E') {
        sb_append_char(&ctx->buffer, input_char);
        current_state = STATE_DOUBLE_EXP_CHAR;
      } else if (c == 'd' || c == 'D') {
        sb_append_char(&ctx->buffer, input_char);
        return MAKE_TOKEN_LEXEME(T_DOUBLE_LITERAL, &ctx->buffer);
      } else {
        ungetc(c, ctx->input);
        DEBUG_LOG("putting char back %c (%d)\n", (char)c, c);
        return MAKE_TOKEN_LEXEME(T_DOUBLE_LITERAL, &ctx->buffer);
      }
      break;

    case STATE_DOUBLE_FRACTION_UNDERSCORE:
      if (isdigit(c)) {
        sb_append_char(&ctx->buffer, input_char);
        current_state = STATE_DOUBLE_FRACTION;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;
    default:
      // nikdy by nemělo nastat!!
      assert(false && "Lex integer/double state machine fell through!");
      return MAKE_TOKEN_ERROR();
      break;
    }
  }
}

/**
 * @brief Načte řetězcový literál.
 *
 * Volá se s úvodní uvozovkou vrácenou zpět do vstupu. Rozpoznává běžný
 * jednořádkový řetězec s escape sekvencemi (nový řádek, tabulátor, CR, zpětné
 * lomítko, uvozovka) a víceřádkový řetězec ohraničený třemi uvozovkami, v němž
 * se escape sekvence nevyhodnocují a smí obsahovat konce řádků.
 *
 * Lexém obsahuje obsah řetězce bez ohraničujících uvozovek a s převedenými
 * escape sekvencemi.
 *
 * @param ctx Kontext lexeru.
 * @return T_STRING_LITERAL s lexémem, jinak T_ERROR.
 */
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
        sb_append_char(&ctx->buffer, input_char);
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
        sb_append_char(&ctx->buffer, input_char);
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_STRING_ESCAPE:
      if (c == 'n') {
        sb_append_char(&ctx->buffer, '\n');
        current_state = STATE_STRING_BODY;
      } else if (c == 't') {
        sb_append_char(&ctx->buffer, '\t');
        current_state = STATE_STRING_BODY;
      } else if (c == 'r') {
        sb_append_char(&ctx->buffer, '\r');
        current_state = STATE_STRING_BODY;
      } else if (c == '\\') {
        sb_append_char(&ctx->buffer, '\\');
        current_state = STATE_STRING_BODY;
      } else if (c == '"') {
        sb_append_char(&ctx->buffer, '\"');
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
        return MAKE_TOKEN_LEXEME(T_STRING_LITERAL, &ctx->buffer);
      }
      break;

    case STATE_MULTILINE_STRING:
      if (c == '"') {
        current_state = STATE_MULTILINE_STRING_QUOTE_1;
      } else if (c == '\n' || c == '\r' || c == '\t' ||
                 (c >= ' ' && c != 127)) {
        sb_append_char(&ctx->buffer, input_char);
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
        sb_append_char(&ctx->buffer, input_char);
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_MULTILINE_STRING_QUOTE_1:
      if (c == '"') {
        current_state = STATE_MULTILINE_STRING_QUOTE_2;
      } else if (c == '\n' || c == '\r' || c == '\t' ||
                 (c >= ' ' && c != 127)) {
        sb_append_char(&ctx->buffer,
                       '"'); // nezahodit uvozovku z předchozího stavu
        sb_append_char(&ctx->buffer, input_char);
        current_state = STATE_MULTILINE_STRING_BODY;
      } else {
        return MAKE_TOKEN_ERROR();
      }
      break;

    case STATE_MULTILINE_STRING_QUOTE_2:
      if (c == '"') {
        return MAKE_TOKEN_LEXEME(T_STRING_LITERAL, &ctx->buffer);
      } else if (c == '\n' || c == '\r' || c == '\t' ||
                 (c >= ' ' && c != 127)) {
        sb_append_char(&ctx->buffer,
                       '"'); // nezahodit uvozovku z předchozího stavu
        sb_append_char(&ctx->buffer, '"');
        sb_append_char(&ctx->buffer, input_char);
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

/**
 * @brief Zpracuje lomítko: dělení nebo komentář.
 *
 * Volá se po přečtení znaku `/`. Podle dalšího znaku vrátí:
 *
 * - T_DIVIDE, pokud nejde o komentář,
 * - T_EOL (na konci vstupu T_EOF) po řádkovém komentáři,
 * - T_WHITESPACE_SEP po jednořádkovém blokovém komentáři,
 * - T_EOL po víceřádkovém blokovém komentáři; za jeho koncem smějí do konce
 *   řádku následovat jen bílé znaky, jinak je to T_ERROR.
 *
 * Blokové komentáře lze vnořovat. Neukončený komentář je T_ERROR.
 *
 * @param ctx Kontext lexeru.
 * @return Jeden z výše uvedených tokenů.
 */
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

/**
 * @brief Zpracuje znak `=` (přiřazení nebo porovnání).
 *
 * Volá se po přečtení znaku `=`.
 *
 * @param ctx Kontext lexeru.
 * @return T_EQUAL pro `==`, jinak T_ASSIGN (další znak se vrátí do vstupu).
 */
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

/**
 * @brief Zpracuje znak `<` (menší nebo menší rovno).
 *
 * Volá se po přečtení znaku `<`.
 *
 * @param ctx Kontext lexeru.
 * @return T_LTE pro `<=`, jinak T_LT (další znak se vrátí do vstupu).
 */
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

/**
 * @brief Zpracuje znak `>` (větší nebo větší rovno).
 *
 * Volá se po přečtení znaku `>`.
 *
 * @param ctx Kontext lexeru.
 * @return T_GTE pro `>=`, jinak T_GT (další znak se vrátí do vstupu).
 */
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

/**
 * @brief Zpracuje znak `!` (negace nebo nerovnost).
 *
 * Volá se po přečtení znaku `!`.
 *
 * @param ctx Kontext lexeru.
 * @return T_NOT_EQUAL pro `!=`, jinak T_NOT (další znak se vrátí do vstupu).
 */
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

/**
 * @brief Zpracuje znak `&` (logické a).
 *
 * Volá se po přečtení znaku `&`. Samotné `&` jazyk nezná.
 *
 * @param ctx Kontext lexeru.
 * @return T_AND pro `&&`, jinak T_ERROR (další znak se vrátí do vstupu).
 */
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

/**
 * @brief Zpracuje znak `|` (logické nebo).
 *
 * Volá se po přečtení znaku `|`. Samotné `|` jazyk nezná.
 *
 * @param ctx Kontext lexeru.
 * @return T_OR pro `||`, jinak T_ERROR (další znak se vrátí do vstupu).
 */
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

/**
 * @brief Zpracuje symbol \@main.
 *
 * Volá se po přečtení zavináče. Očekává písmena `main`; jakýkoli jiný znak je
 * chyba.
 *
 * @param ctx Kontext lexeru.
 * @return T_FUNC_MAIN, jinak T_ERROR.
 */
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

/**
 * @brief Ukončí zpracování interpolovaného řetězce chybou.
 *
 * Zruší všechny rozpracované úrovně interpolace, vyprázdní frontu tokenů i
 * vyrovnávací paměť.
 *
 * @param ctx Kontext lexeru.
 * @return T_ERROR.
 */
static Token interp_error(LexerCtx *ctx) {
  ctx->interp_depth = 0;
  queue_clear(ctx);
  sb_reset(&ctx->buffer);
  return MAKE_TOKEN_ERROR();
}

/**
 * @brief Zahájí další část rozbaleného interpolovaného řetězce.
 *
 * Pokud aktuální úroveň už nějakou část obsahuje, vloží do fronty operátor `+`,
 * který části spojuje. Úroveň pak označí jako neprázdnou.
 *
 * @param ctx Kontext lexeru.
 * @return true při úspěchu, false při nedostatku paměti.
 */
static bool interp_begin_part(LexerCtx *ctx) {
  InterpLevel *lv = &INTERP_TOP(ctx);
  if (lv->has_part && !queue_push(ctx, MAKE_TOKEN(T_PLUS_SIGN))) {
    return false;
  }
  lv->has_part = true;
  return true;
}

/**
 * @brief Vloží do fronty text nasbíraný ve vyrovnávací paměti.
 *
 * Neprázdný obsah vyrovnávací paměti se stane tokenem T_STRING_LITERAL (podle
 * potřeby s předchozím `+`) a paměť se vyprázdní. Při prázdném obsahu nedělá
 * nic.
 *
 * @param ctx Kontext lexeru.
 * @return true při úspěchu, false při nedostatku paměti.
 */
static bool interp_flush_literal(LexerCtx *ctx) {
  if (ctx->buffer.length == 0) {
    return true;
  }
  if (!interp_begin_part(ctx)) {
    return false;
  }
  Token t = MAKE_TOKEN_LEXEME(T_STRING_LITERAL, &ctx->buffer);
  sb_reset(&ctx->buffer);
  if (!queue_push(ctx, t)) {
    discard_token(t);
    return false;
  }
  return true;
}

/**
 * @brief Vloží do fronty začátek volání toStr.
 *
 * Vloží tokeny `toStr ( (`. Hodnota výrazu se tak předá vestavěné funkci toStr,
 * která ji převede na řetězec.
 *
 * @param ctx Kontext lexeru.
 * @return true při úspěchu, false při nedostatku paměti.
 */
static bool interp_push_tostr_open(LexerCtx *ctx) {
  return queue_push_lexeme(ctx, T_IDENTIFICATOR, "toStr") &&
         queue_push(ctx, MAKE_TOKEN(T_OPEN_BRACKET)) &&
         queue_push(ctx, MAKE_TOKEN(T_OPEN_BRACKET));
}

/**
 * @brief Vloží do fronty konec volání toStr.
 *
 * Vloží dvě uzavírací závorky, které ukončují zápis z funkce
 * interp_push_tostr_open().
 *
 * @param ctx Kontext lexeru.
 * @return true při úspěchu, false při nedostatku paměti.
 */
static bool interp_push_tostr_close(LexerCtx *ctx) {
  return queue_push(ctx, MAKE_TOKEN(T_CLOSE_BRACKET)) &&
         queue_push(ctx, MAKE_TOKEN(T_CLOSE_BRACKET));
}

/**
 * @brief Dokončí interpolovaný řetězec po uzavírací uvozovce.
 *
 * Vloží zbývající text (u zcela prázdného řetězce prázdný řetězcový literál) a
 * uzavírací závorku, sníží hloubku vnoření a vyprázdní vyrovnávací paměť.
 *
 * @param ctx Kontext lexeru.
 * @return První token z fronty, při nedostatku paměti T_ERROR.
 */
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
  sb_reset(&ctx->buffer);
  return queue_pop(ctx);
}

/**
 * @brief Zahájí výraz `${...}` uvnitř interpolovaného řetězce.
 *
 * Vloží do fronty dosavadní text a začátek volání toStr, přepne úroveň do fáze
 * IP_EXPR a další tokeny výrazu čte lexer běžným způsobem.
 *
 * @param ctx Kontext lexeru.
 * @return První token z fronty, při nedostatku paměti T_ERROR.
 */
static Token interp_begin_expr(LexerCtx *ctx) {
  if (!interp_flush_literal(ctx) || !interp_begin_part(ctx) ||
      !interp_push_tostr_open(ctx)) {
    return interp_error(ctx);
  }
  INTERP_TOP(ctx).phase = IP_EXPR;
  INTERP_TOP(ctx).expr_tokens = 0;
  sb_reset(&ctx->buffer);
  return queue_pop(ctx);
}

/**
 * @brief Vloží do fronty zkrácený zápis `$identifikátor`.
 *
 * Identifikátor z vyrovnávací paměti obalí voláním toStr. Samotné podtržítko
 * jako identifikátor je chyba.
 *
 * @param ctx Kontext lexeru.
 * @return První token z fronty, jinak T_ERROR.
 */
static Token interp_emit_ident(LexerCtx *ctx) {
  if (ctx->buffer.length == 1 && ctx->buffer.data[0] == '_') {
    return interp_error(ctx);
  }
  if (!interp_begin_part(ctx) || !interp_push_tostr_open(ctx)) {
    return interp_error(ctx);
  }
  Token id = MAKE_TOKEN_LEXEME(T_IDENTIFICATOR, &ctx->buffer);
  sb_reset(&ctx->buffer);
  if (!queue_push(ctx, id)) {
    discard_token(id);
    return interp_error(ctx);
  }
  if (!interp_push_tostr_close(ctx)) {
    return interp_error(ctx);
  }
  return queue_pop(ctx);
}

/**
 * @brief Zahájí interpolovaný řetězec po přečtení `s"`.
 *
 * Založí novou úroveň na zásobníku a vloží do fronty otevírací závorku, kterou
 * rozbalený řetězec začíná. Vnořený řetězec uvnitř výrazu `${...}` se počítá
 * jako jeden token tohoto výrazu. Při překročení INTERP_MAX_DEPTH vrací chybu.
 *
 * @param ctx Kontext lexeru.
 * @return Otevírací závorka, jinak T_ERROR.
 */
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

/**
 * @brief Ukončí výraz `${...}` po načtení znaku `}`.
 *
 * Prázdný výraz je chyba. Jinak vloží uzavírací závorky volání toStr a vrátí
 * úroveň do fáze IP_BODY.
 *
 * @param ctx Kontext lexeru.
 * @return První token z fronty, jinak T_ERROR.
 */
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

/**
 * @brief Načte textovou část interpolovaného řetězce.
 *
 * Čte znaky úrovně na vrcholu zásobníku (fáze IP_START nebo IP_BODY).
 * Rozpoznává prázdný řetězec `s""`, víceřádkový tvar `s"""..."""`, escape
 * sekvence (jen v jednořádkovém tvaru), `$$` jako znak `$`, začátek výrazu `${`
 * a zkrácený zápis `$identifikátor`. Uzavírací uvozovka řetězec dokončí.
 *
 * Výsledkem je vždy první token rozbalené posloupnosti, další tokeny čekají ve
 * frontě.
 *
 * @param ctx Kontext lexeru.
 * @return První token z fronty, jinak T_ERROR.
 */
static Token lex_interp_literal(LexerCtx *ctx) {
  InterpLevel *lv = &INTERP_TOP(ctx);
  State current_state =
      (lv->phase == IP_START) ? STATE_INTERP_STRING : STATE_INTERP_STRING_BODY;
  int run = 0;
  sb_reset(&ctx->buffer);
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
        sb_append_char(&ctx->buffer, (char)c);
      } else {
        return interp_error(ctx);
      }
      break;

    case STATE_INTERP_STRING_ESCAPE:
      if (c == 'n') {
        sb_append_char(&ctx->buffer, '\n');
      } else if (c == 't') {
        sb_append_char(&ctx->buffer, '\t');
      } else if (c == 'r') {
        sb_append_char(&ctx->buffer, '\r');
      } else if (c == '\\') {
        sb_append_char(&ctx->buffer, '\\');
      } else if (c == '"') {
        sb_append_char(&ctx->buffer, '"');
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
            sb_append_char(&ctx->buffer, '"');
          }
          return interp_finish(ctx);
        }
        for (int i = 0; i < run; i++) {
          sb_append_char(&ctx->buffer, '"');
        }
        current_state = STATE_INTERP_STRING_BODY;
      }
      break;

    case STATE_INTERP_STRING_DOLLAR:
      if (c == '$') {
        sb_append_char(&ctx->buffer, '$');
        current_state = STATE_INTERP_STRING_BODY;
      } else if (c == '{') {
        return interp_begin_expr(ctx);
      } else if (c == '_' || isalpha(c)) {
        if (!interp_flush_literal(ctx)) {
          return interp_error(ctx);
        }
        sb_reset(&ctx->buffer);
        sb_append_char(&ctx->buffer, (char)c);
        current_state = STATE_INTERP_STRING_ID;
      } else {
        return interp_error(ctx);
      }
      break;

    case STATE_INTERP_STRING_ID:
      if (c == '_' || isalnum(c)) {
        sb_append_char(&ctx->buffer, (char)c);
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

/**
 * @brief Načte jeden token ze vstupu.
 *
 * Podle prvního znaku vybere příslušný automat. Znak CR před dalším znakem
 * přeskočí. Identifikátor `s` těsně následovaný uvozovkou zahajuje
 * interpolovaný řetězec. Znak `}` uzavírá výraz interpolace jen uvnitř
 * `${...}`, jinak je chybou. Neznámý znak vrací T_ERROR.
 *
 * Filtrování tokenů uvnitř výrazu interpolace řeší až get_next_token().
 *
 * @param ctx Kontext lexeru.
 * @return Načtený token.
 */
static Token lex_token(LexerCtx *ctx) {
  int c;
  sb_reset(&ctx->buffer);
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
