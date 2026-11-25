/**
 * @file precedence.h
 *
 * Překladač jazyka IFJ26
 *
 * @brief Rozhraní precedenční analýzy výrazů.
 *
 * @author Alex Oľšavský (xolsava00)
 * @author Jaroslav Vančura (xvancuj00)
 */

#ifndef PRECEDENCE_H
#define PRECEDENCE_H

#include "ast.h"
#include "error.h"
#include "lexer.h"
#include "parser.h"
#include "symtable.h"
#include <stdlib.h>

/**
 * @brief Typ výrazu zjištěný při překladu.
 *
 * Hodnota EXPR_TYPE_UNKNOWN znamená, že typ zatím není znám; kontroly typů se
 * pak přeskakují a ověří se až později.
 */
typedef enum ExprType {
  EXPR_TYPE_UNKNOWN, ///< typ zatím není znám (kontrola typů se přeskočí)
  EXPR_TYPE_INT,     ///< `Int`
  EXPR_TYPE_DOUBLE,  ///< `Double`
  EXPR_TYPE_STRING,  ///< `String`
  EXPR_TYPE_UNIT,    ///< `Unit` (například hodnota `()`)
  EXPR_TYPE_NULL,    ///< `null` (zatím se nikde nepřiřazuje)
  EXPR_TYPE_BOOL     ///< `Boolean`
} ExprType;

/**
 * @brief Akce precedenční tabulky.
 *
 * Hodnotu vybírá dvojice symbolů: symbol na vrcholu zásobníku (řádek tabulky) a
 * vstupní symbol (sloupec tabulky).
 */
typedef enum PrecedenceAction {
  PREC_SHIFT,  ///< přesun vstupního symbolu na zásobník
  PREC_REDUCE, ///< redukce operátoru na vrcholu zásobníku
  PREC_EQUAL,  ///< symboly si odpovídají (dvojice závorek)
  PREC_ERROR   ///< kombinace symbolů není povolena (syntaktická chyba)
} PrecedenceAction;

/**
 * @brief Symbol precedenční tabulky.
 *
 * Slouží jako index řádku (symbol na vrcholu zásobníku) i sloupce (vstupní
 * symbol) tabulky. Tokeny na symboly převádí funkce token_to_column().
 */
typedef enum ExprColumn {
  EX_PLUS,    ///< `+`
  EX_MINUS,   ///< `-`
  EX_MUL,     ///< `*`
  EX_DIV,     ///< `/`
  EX_EQ,      ///< `==`
  EX_NEQ,     ///< `!=`
  EX_LT,      ///< `<`
  EX_LTE,     ///< `<=`
  EX_GT,      ///< `>`
  EX_GTE,     ///< `>=`
  EX_AND,     ///< `&&`
  EX_OR,      ///< `||`
  EX_LPAREN,  ///< `(`
  EX_RPAREN,  ///< `)`
  EX_OPERAND, ///< operand: identifikátor, literál, `true`, `false`
  EX_DOLLAR,  ///< konec výrazu (T_EOF)
  EX_COUNT    ///< počet symbolů; také hodnota pro token mimo tabulku
} ExprColumn;

/**
 * @brief Převede typ tokenu na sloupec precedenční tabulky.
 *
 * Operátory a závorky mají každý vlastní sloupec. Identifikátory, číselné a
 * řetězcové literály a `true` či `false` patří do sloupce EX_OPERAND, T_EOF do
 * sloupce EX_DOLLAR.
 *
 * @param t Typ tokenu.
 * @return Odpovídající sloupec; pro ostatní tokeny (i pro pomocné značky
 *   unárních operátorů) EX_COUNT.
 */
ExprColumn token_to_column(TokenType t);

/**
 * @brief Vrátí nejvyšší terminální symbol na precedenčním zásobníku.
 *
 * Prochází zásobník od vrcholu a přeskakuje symboly, které nemají sloupec
 * operátoru ani závorky (operandy a značky unárních operátorů).
 *
 * @param stack Precedenční zásobník.
 * @return Typ nalezeného tokenu; pokud na zásobníku žádný terminál není, vrací
 *   T_EOF (konec výrazu).
 */
TokenType top_terminal(PrecedenceStack *stack);

/**
 * @brief Zparsuje výraz a vrátí jeho AST a typ.
 *
 * Čte výraz od aktuálního tokenu po konec řádku, čárku nebo uzavírací závorku,
 * která nepatří k žádné závorce uvnitř výrazu. Podporuje literály,
 * identifikátory, volání funkcí, výrazy `if`, závorky, unární `-` a `!`,
 * binární operátory a volání metod přes tečku.
 *
 * Precedenční zásobník parseru se po skončení vrátí do původního stavu, takže
 * funkci lze volat rekurzivně (například pro argumenty volání).
 *
 * @param parser Kontext parseru; aktuální token je první token výrazu.
 * @param out_ast Výstup: kořen AST výrazu. Smí být NULL.
 * @param out_type Výstup: typ výrazu (EXPR_TYPE_UNKNOWN, pokud ho nelze určit
 *   při překladu). Smí být NULL.
 * @return SUCCESS, jinak chybový kód (například SYN_ANAL_ERR u chybné syntaxe
 *   nebo STATIC_SEM_TYPE_ERR u nesprávných typů operandů). Při chybě se
 *   výstupní parametry nenastavují.
 */
Error parse_expression(ParserCtx *parser, ASTNode **out_ast,
                       ExprType *out_type);

#endif
