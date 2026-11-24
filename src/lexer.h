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

#include "dyn_buff.h"
#include <stdbool.h>
#include <stdio.h>

/**
 * @brief Maximální hloubka vnoření interpolovaných řetězců.
 *
 * Určuje velikost zásobníku LexerCtx::interp. Při překročení vrací lexer
 * T_ERROR.
 */
#define INTERP_MAX_DEPTH 32

/**
 * @brief Typ tokenu.
 *
 * Komentář u hodnoty říká, jakému vstupu token odpovídá.
 */
typedef enum TokenType {
  T_INT_LITERAL,    ///< celočíselný literál
  T_DOUBLE_LITERAL, ///< desetinný literál
  T_STRING_LITERAL, ///< řetězcový literál (lexém bez uvozovek)
  T_IDENTIFICATOR,  ///< identifikátor
  T_KW_DEF,         ///< `def`
  T_KW_DO,          ///< `do`
  T_KW_DOUBLE,      ///< `Double`
  T_KW_ELSE,        ///< `else`
  T_KW_IF,          ///< `if`
  T_KW_IMPORT,      ///< `import`
  T_KW_INT,         ///< `Int`
  T_KW_RETURN,      ///< `return`
  T_KW_STRING,      ///< `String`
  T_KW_THEN,        ///< `then`
  T_KW_UNIT,        ///< `Unit`
  T_KW_VAL,         ///< `val`
  T_KW_VAR,         ///< `var`
  T_KW_WHILE,       ///< `while`
  T_NOT_EQUAL,      ///< `!=`
  T_PLUS_SIGN,      ///< `+`
  T_MINUS_SIGN,     ///< `-`
  T_MULT_SIGN,      ///< `*`
  T_OPEN_BRACKET,   ///< `(`
  T_CLOSE_BRACKET,  ///< `)`
  T_COMMA,          ///< `,`
  T_EQUAL,          ///< `==`
  T_ASSIGN,         ///< `=`
  T_DIVIDE,         ///< `/`
  T_LT,             ///< `<`
  T_LTE,            ///< `<=`
  T_GT,             ///< `>`
  T_GTE,            ///< `>=`
  T_INDENTS,        ///< odsazení mezerami (počet v `count`)
  T_ERROR,          ///< lexikální chyba
  T_EOF,            ///< konec vstupu
  T_EOL,            ///< konec řádku (i po komentáři)
  T_DOUBLE_DOT,     ///< dvojtečka `:`
  T_WHITESPACE_SEP, ///< jednořádkový blokový komentář
  T_FUNC_MAIN,      ///< \@main
  T_DOT,            ///< `.`
  T_SEMICOLON,      ///< `;`
  T_KW_IFJ26,       ///< `ifj26`
  T_TAB,            ///< jeden či více tabulátorů
  T_EXTENSION,      ///< `extension`
  T_KW_BOOLEAN,     ///< `Boolean`
  T_KW_TRUE,        ///< `true`
  T_KW_FALSE,       ///< `false`
  T_AND,            ///< `&&`
  T_OR,             ///< `||`
  T_NOT,            ///< `!`
  T_KW_FOR,         ///< `for`
} TokenType;

/**
 * @brief Token nesoucí lexém (literál nebo identifikátor).
 */
typedef struct {
  TokenType type;     ///< Typ tokenu.
  const char *lexeme; ///< Lexém z haldy (malloc); uvolňuje volající.
} TokenLexeme;

/**
 * @brief Token odsazení (T_INDENTS).
 */
typedef struct {
  TokenType type; ///< Typ tokenu (T_INDENTS).
  int count;      ///< Počet mezer na začátku řádku.
} TokenIndents;

/**
 * @brief Token vrácený lexerem.
 *
 * Všechny členy začínají polem TokenType, takže typ tokenu vždy zjistíme přes
 * `token_type`. Ostatní členy jsou použitelné jen pro příslušné typy tokenů.
 */
typedef union {
  TokenType token_type;       ///< Typ tokenu (platí vždy).
  TokenLexeme token_lexeme;   ///< Literály a identifikátory.
  TokenIndents token_indents; ///< Odsazení (T_INDENTS).
} Token;

/**
 * @brief Fáze zpracování jedné úrovně interpolovaného řetězce.
 */
typedef enum InterpPhase {
  IP_START, ///< Přečteno `s"`, z obsahu zatím nic.
  IP_BODY,  ///< Čtení textové části řetězce.
  IP_EXPR,  ///< Čtení výrazu uvnitř `${...}`.
} InterpPhase;

/**
 * @brief Stav jedné úrovně (vnoření) interpolovaného řetězce.
 *
 * Lexer interpolovaný řetězec rozbalí na posloupnost tokenů, kterou vydává
 * postupně.
 *
 * Příklad: `s"a${x}b"` se rozbalí na `("a" + toStr((x)) + "b")`.
 */
typedef struct {
  /** Aktuální fáze zpracování. */
  InterpPhase phase;
  /** Řetězec je víceřádkový (`s"""..."""`). */
  bool multiline;
  /**
   * Do výstupu už byla vložena aspoň jedna část (text či výraz); před další
   * částí se vloží `+`.
   */
  bool has_part;
  /** Počet tokenů dosud načtených v aktuálním výrazu `${...}`. */
  int expr_tokens;
} InterpLevel;

/**
 * @brief Kontext lexikálního analyzátoru.
 *
 * Inicializuje se funkcí lexer_init() a uvolňuje funkcí lexer_free().
 */
typedef struct {
  /** Vyrovnávací paměť pro lexém právě čtené jednotky. */
  StringBuff buffer;
  /** Vstupní soubor se zdrojovým kódem (lexer ho nevlastní). */
  FILE *input;
  /**
   * Fronta tokenů čekajících na vydání. Ukládá se do ní rozbalený interpolovaný
   * řetězec.
   */
  Token *pending;
  /** Počet tokenů ve frontě (včetně již vydaných). */
  size_t pending_len;
  /** Index prvního dosud nevydaného tokenu ve frontě. */
  size_t pending_pos;
  /** Kapacita pole `pending` v počtu tokenů. */
  size_t pending_cap;
  /** Aktuální hloubka vnoření interpolovaných řetězců. */
  int interp_depth;
  /** Zásobník úrovní interpolace; platných je `interp_depth` položek. */
  InterpLevel interp[INTERP_MAX_DEPTH];
} LexerCtx;

/**
 * @brief Načte ze vstupu další token.
 *
 * Nejdřív vydává tokeny čekající ve frontě (rozbalený interpolovaný řetězec),
 * jinak přečte další token ze vstupu. Tokeny nefiltruje: konce řádků vrací jako
 * T_EOL, mezery na začátku řádku jako T_INDENTS a tabulátory jako T_TAB.
 *
 * Komentáře vrací takto: řádkový komentář jako T_EOL (na konci vstupu T_EOF),
 * jednořádkový blokový jako T_WHITESPACE_SEP a víceřádkový blokový jako T_EOL.
 * Na konci vstupu vrací T_EOF, při lexikální chybě T_ERROR.
 *
 * Lexém literálů a identifikátorů je alokován přes malloc a uvolňuje ho
 * volající (free).
 *
 * @param ctx Kontext lexeru inicializovaný funkcí lexer_init().
 * @return Načtený token.
 */
Token get_next_token(LexerCtx *ctx);

/**
 * @brief Inicializuje lexer.
 *
 * Nastaví vstup, vytvoří vyrovnávací paměť a vyprázdní frontu tokenů. Soubor
 * lexer neotevírá ani nezavírá.
 *
 * @param ctx Kontext lexeru, který se inicializuje.
 * @param input Otevřený vstupní soubor se zdrojovým kódem.
 */
void lexer_init(LexerCtx *ctx, FILE *input);

/**
 * @brief Uvolní prostředky lexeru.
 *
 * Uvolní vyrovnávací paměť, frontu a lexémy dosud nevydaných tokenů. Vstupní
 * soubor nezavírá. Lexémy tokenů, které už lexer vydal, patří volajícímu.
 *
 * @param ctx Kontext lexeru inicializovaný funkcí lexer_init().
 */
void lexer_free(LexerCtx *ctx);

/**
 * @brief Vypíše ladicí zprávu na standardní chybový výstup.
 *
 * V ladicím překladu před zprávu vloží soubor, řádek a funkci, odkud se makro
 * volá. Při překladu s -DNDEBUG se rozvine na prázdný výraz a nic se nevypíše.
 *
 * @param fmt Formátovací řetězec jako u printf.
 * @param ... Argumenty formátovacího řetězce.
 */
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
