/**
 * @file precedence.c
 *
 * Překladač jazyka IFJ26
 *
 * @brief Implementace precedenční analýzy výrazů, typových kontrol a volání.
 *
 * Zpracovává binární a unární operátory, závorky, volání funkcí a tečkové
 * (vestavěné i rozšiřující) metody se skládáním uzlů AST.
 *
 * @author Alex Oľšavský (xolsava00)
 * @author Jaroslav Vančura (xvancuj00)
 * @author Matěj Okruhlica (xokruhm00)
 * @author Klára Vantuchová (xvantuk00)
 */

#include "precedence.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Kapacita zásobníku typů (TypeStack) i zásobníku uzlů AST (ASTStack).
 */
#define TYPE_STACK_MAX 256

/**
 * @brief Kapacita precedenčního zásobníku (PrecedenceStack).
 *
 * Počítá se z velikosti pole `items`, takže sleduje případnou změnu typu
 * PrecedenceStack.
 */
#define PREC_STACK_MAX                                                         \
  ((int)(sizeof(((PrecedenceStack *)0)->items) / sizeof(TokenType)))

/**
 * @brief Značka unárního minus na precedenčním zásobníku.
 *
 * Hodnota leží mimo rozsah TokenType, takže se nezamění s žádným skutečným
 * tokenem.
 */
#define T_UNARY_MINUS_MARK ((TokenType)1000)

/**
 * @brief Značka unární negace `!` na precedenčním zásobníku.
 *
 * Hodnota leží mimo rozsah TokenType, takže se nezamění s žádným skutečným
 * tokenem.
 */
#define T_UNARY_NOT_MARK ((TokenType)1001)

/**
 * @brief Posune se na další token a ošetří lexikální chybu.
 *
 * Pokud lexer vrátí T_ERROR, ukončí volající funkci s kódem LEX_ANAL_ERR. Makro
 * obsahuje `return`, smí se proto použít jen ve funkci vracející Error.
 * Středník píše až volající.
 *
 * @param parser Kontext parseru.
 */
#define CONSUME_TOKEN_GUARD(parser)                                            \
  do {                                                                         \
    Token tok = consume_token((parser));                                       \
    if (tok.token_type == T_ERROR) {                                           \
      return LEX_ANAL_ERR;                                                     \
    }                                                                          \
  } while (0)

/**
 * @brief Zásobník typů operandů a mezivýsledků.
 *
 * Běží souběžně se zásobníkem uzlů AST (ASTStack): na stejné pozici leží typ a
 * uzel téhož podvýrazu.
 */
typedef struct {
  /** Pole uložených typů. */
  ExprType items[TYPE_STACK_MAX];
  /** Index vrcholu; -1 značí prázdný zásobník. */
  int top;
} TypeStack;

/**
 * @brief Inicializuje prázdný zásobník typů.
 *
 * @param stack Zásobník typů.
 */
static void tstack_init(TypeStack *stack) { stack->top = -1; }

/**
 * @brief Vloží typ na vrchol zásobníku typů.
 *
 * @param stack Zásobník typů.
 * @param expr_type Vkládaný typ.
 * @return true při úspěchu, false při přeplnění zásobníku.
 */
static bool tstack_push(TypeStack *stack, ExprType expr_type) {
  if (stack->top >= TYPE_STACK_MAX - 1) {
    DEBUG_LOG("[DEBUG] tstack_push overflow!\n");
    return false;
  }
  DEBUG_LOG("[DEBUG] tstack_push: pushed type %d, new top = %d\n", expr_type,
            stack->top);
  stack->items[++stack->top] = expr_type;
  return true;
}

/**
 * @brief Odebere typ z vrcholu zásobníku typů.
 *
 * @param stack Zásobník typů.
 * @param out Výstup: odebraný typ.
 * @return true při úspěchu, false na prázdném zásobníku.
 */
static bool tstack_pop(TypeStack *stack, ExprType *out) {
  if (stack->top < 0) {
    DEBUG_LOG("[DEBUG] tstack_pop underflow!\n");
    return false;
  }
  DEBUG_LOG("[DEBUG] tstack_pop: popped type %d, new top = %d\n", *out,
            stack->top);
  *out = stack->items[stack->top--];
  return true;
}

/**
 * @brief Přečte typ z vrcholu zásobníku typů bez odebrání.
 *
 * @param stack Zásobník typů.
 * @param out Výstup: typ na vrcholu.
 * @return true při úspěchu, false na prázdném zásobníku.
 */
static bool tstack_peek(TypeStack *stack, ExprType *out) {
  if (stack->top < 0)
    return false;
  *out = stack->items[stack->top];
  return true;
}

/**
 * @brief Zásobník uzlů AST právě sestavovaného výrazu.
 *
 * Zásobník uzly nevlastní a neuvolňuje je; odebraný uzel přebírá volající.
 */
typedef struct {
  /** Pole ukazatelů na uzly. */
  ASTNode *items[TYPE_STACK_MAX];
  /** Index vrcholu; -1 značí prázdný zásobník. */
  int top;
} ASTStack;

/**
 * @brief Inicializuje prázdný zásobník uzlů AST.
 *
 * @param stack Zásobník uzlů.
 */
static void ast_stack_init(ASTStack *stack) { stack->top = -1; }

/**
 * @brief Vloží uzel AST na vrchol zásobníku.
 *
 * @param stack Zásobník uzlů.
 * @param node Vkládaný uzel.
 * @return true při úspěchu, false při přeplnění zásobníku.
 */
static bool ast_stack_push(ASTStack *stack, ASTNode *node) {
  if (stack->top >= TYPE_STACK_MAX - 1)
    return false;
  stack->items[++stack->top] = node;
  return true;
}

/**
 * @brief Odebere uzel AST z vrcholu zásobníku.
 *
 * @param stack Zásobník uzlů.
 * @param out Výstup: odebraný uzel.
 * @return true při úspěchu, false na prázdném zásobníku.
 */
static bool ast_stack_pop(ASTStack *stack, ASTNode **out) {
  if (stack->top < 0)
    return false;
  *out = stack->items[stack->top--];
  return true;
}

/**
 * @brief Vyprázdní precedenční zásobník.
 *
 * @param stack Precedenční zásobník.
 */
static void stack_clear(PrecedenceStack *stack) {
  stack->top = -1;
  DEBUG_LOG("[DEBUG] precedence stack cleared\n");
}

/**
 * @brief Vloží token na vrchol precedenčního zásobníku.
 *
 * @param stack Precedenční zásobník.
 * @param tok_type Vkládaný token.
 * @return true při úspěchu, false při přeplnění zásobníku.
 */
static bool stack_push(PrecedenceStack *stack, TokenType tok_type) {
  if (stack->top >= PREC_STACK_MAX - 1) {
    DEBUG_LOG("[DEBUG] precedence stack overflow!\n");
    return false;
  }
  DEBUG_LOG("[DEBUG] stack_push: token %d pushed, new stack top = %d\n",
            tok_type, stack->top);
  stack->items[++stack->top] = tok_type;
  return true;
}

/**
 * @brief Odebere token z vrcholu precedenčního zásobníku.
 *
 * @param stack Precedenční zásobník.
 * @return true při úspěchu, false na prázdném zásobníku.
 */
static bool stack_pop(PrecedenceStack *stack) {
  if (stack->top < 0) {
    DEBUG_LOG("[DEBUG] precedence stack underflow on pop!\n");
    return false;
  }
  DEBUG_LOG("[DEBUG] stack_pop: removing token %d from top = %d\n",
            stack->items[stack->top], stack->top);
  stack->top--;
  return true;
}

/**
 * @brief Určí typ výrazu podle tokenu literálu.
 *
 * Rozpoznává celočíselný, desetinný a řetězcový literál a `true` či `false`.
 *
 * @param tok_type Typ tokenu.
 * @return Typ literálu, pro ostatní tokeny EXPR_TYPE_UNKNOWN.
 */
static ExprType expr_type_from_token(TokenType tok_type) {
  switch (tok_type) {
  case T_INT_LITERAL:
    return EXPR_TYPE_INT;
  case T_DOUBLE_LITERAL:
    return EXPR_TYPE_DOUBLE;
  case T_STRING_LITERAL:
    return EXPR_TYPE_STRING;
  case T_KW_TRUE:
  case T_KW_FALSE:
    return EXPR_TYPE_BOOL;
  default:
    return EXPR_TYPE_UNKNOWN;
  }
}

/**
 * @brief Určí typ výrazu podle klíčového slova typu.
 *
 * Rozpoznává `Int`, `Double`, `String`, `Unit` a `Boolean`.
 *
 * @param tok_type Typ tokenu (klíčové slovo typu).
 * @return Odpovídající typ, pro ostatní tokeny EXPR_TYPE_UNKNOWN.
 */
static ExprType expr_type_from_kw(TokenType tok_type) {
  if (tok_type == T_TYPE_BOOLEAN)
    return EXPR_TYPE_BOOL;
  switch (tok_type) {
  case T_KW_INT:
    return EXPR_TYPE_INT;
  case T_KW_DOUBLE:
    return EXPR_TYPE_DOUBLE;
  case T_KW_STRING:
    return EXPR_TYPE_STRING;
  case T_KW_UNIT:
    return EXPR_TYPE_UNIT;
  default:
    return EXPR_TYPE_UNKNOWN;
  }
}

/** @brief Zkratka pro PREC_SHIFT při zápisu tabulky. */
#define S PREC_SHIFT
/** @brief Zkratka pro PREC_REDUCE při zápisu tabulky. */
#define R PREC_REDUCE
/** @brief Zkratka pro PREC_EQUAL při zápisu tabulky. */
#define Q PREC_EQUAL
/** @brief Zkratka pro PREC_ERROR při zápisu tabulky. */
#define X PREC_ERROR

// clang-format off
static const PrecedenceAction precedence_table[EX_COUNT][EX_COUNT] = {
    /*          +  -  *  /  == != <  <= >  >= && || (  )  i  $ */
    /* +  */  {R, R, S, S, R, R, R, R, R, R, R, R, S, R, S, R},
    /* -  */  {R, R, S, S, R, R, R, R, R, R, R, R, S, R, S, R},
    /* *  */  {R, R, R, R, R, R, R, R, R, R, R, R, S, R, S, R},
    /* /  */  {R, R, R, R, R, R, R, R, R, R, R, R, S, R, S, R},
    /* == */  {S, S, S, S, X, X, S, S, S, S, R, R, S, R, S, R},
    /* != */  {S, S, S, S, X, X, S, S, S, S, R, R, S, R, S, R},
    /* <  */  {S, S, S, S, R, R, X, X, X, X, R, R, S, R, S, R},
    /* <= */  {S, S, S, S, R, R, X, X, X, X, R, R, S, R, S, R},
    /* >  */  {S, S, S, S, R, R, X, X, X, X, R, R, S, R, S, R},
    /* >= */  {S, S, S, S, R, R, X, X, X, X, R, R, S, R, S, R},
    /* && */  {S, S, S, S, S, S, S, S, S, S, R, R, S, R, S, R},
    /* || */  {S, S, S, S, S, S, S, S, S, S, S, R, S, R, S, R},
    /* (  */  {S, S, S, S, S, S, S, S, S, S, S, S, S, Q, S, X},
    /* )  */  {R, R, R, R, R, R, R, R, R, R, R, R, X, R, X, R},
    /* i  */  {R, R, R, R, R, R, R, R, R, R, R, R, X, R, X, R},
    /* $  */  {S, S, S, S, S, S, S, S, S, S, S, S, S, X, S, X},
};
// clang-format on

#undef S
#undef R
#undef Q
#undef X

/**
 * @brief Ověří identifikátor použitý ve výrazu a zjistí jeho typ.
 *
 * Identifikátor se hledá v tabulce symbolů. Jméno funkce nelze použít jako
 * hodnotu.
 *
 * @param parser Kontext parseru (s tabulkou symbolů).
 * @param token Token s identifikátorem.
 * @param type Výstup: typ proměnné.
 * @return SUCCESS; UNDEF_FN_ERR, pokud identifikátor není definován;
 *   STATIC_SEM_TYPE_ERR, pokud jde o funkci.
 */
static Error check_identifier_in_expr(ParserCtx *parser, const Token *token,
                                      ExprType *type) {
  SymtableEntry *entry = NULL;

  if (!smt_search(&parser->smt, (char *)token->token_lexeme.lexeme, &entry)) {
    DEBUG_LOG("[DEBUG] check_identifier_in_expr1: identifier '%s' not found "
              "(UNDEF_FN_ERR)\n",
              (char *)token->token_lexeme.lexeme);
    return UNDEF_FN_ERR;
  }
  if (entry->function) {
    DEBUG_LOG("[DEBUG] check_identifier_in_expr2: identifier '%s' not found "
              "(UNDEF_FN_ERR)\n",
              (char *)token->token_lexeme.lexeme);
    return STATIC_SEM_TYPE_ERR;
  }

  *type = expr_type_from_kw(entry->type);
  DEBUG_LOG("[DEBUG] check_identifier_in_expr3: identifier '%s' not found "
            "(UNDEF_FN_ERR)\n",
            (char *)token->token_lexeme.lexeme);
  return SUCCESS;
}

ExprColumn token_to_column(TokenType tok_type) {
  switch (tok_type) {
  case T_PLUS_SIGN:
    return EX_PLUS;
  case T_MINUS_SIGN:
    return EX_MINUS;
  case T_MULT_SIGN:
    return EX_MUL;
  case T_DIVIDE:
    return EX_DIV;
  case T_EQUAL:
    return EX_EQ;
  case T_NOT_EQUAL:
    return EX_NEQ;
  case T_LT:
    return EX_LT;
  case T_LTE:
    return EX_LTE;
  case T_GT:
    return EX_GT;
  case T_GTE:
    return EX_GTE;
  case T_AND:
    return EX_AND;
  case T_OR:
    return EX_OR;
  case T_OPEN_BRACKET:
    return EX_LPAREN;
  case T_CLOSE_BRACKET:
    return EX_RPAREN;
  case T_IDENTIFICATOR:
  case T_INT_LITERAL:
  case T_DOUBLE_LITERAL:
  case T_STRING_LITERAL:
  case T_KW_TRUE:
  case T_KW_FALSE:
    return EX_OPERAND;
  case T_EOF:
    return EX_DOLLAR;
  default:
    return EX_COUNT;
  }
}

TokenType top_terminal(PrecedenceStack *stack) {
  for (int i = stack->top; i >= 0; i--) {
    TokenType tok_type = stack->items[i];
    ExprColumn col = token_to_column(tok_type);
    if (col != EX_OPERAND && col != EX_COUNT) {
      return tok_type;
    }
  }
  return T_EOF;
}

/**
 * @brief Zjistí, zda je token operand.
 *
 * Operandem je identifikátor, číselný či řetězcový literál, `true` nebo
 * `false`.
 *
 * @param tok_type Typ tokenu.
 * @return true, pokud jde o operand.
 */
static bool is_operand_token(TokenType tok_type) {
  switch (tok_type) {
  case T_IDENTIFICATOR:
  case T_INT_LITERAL:
  case T_DOUBLE_LITERAL:
  case T_STRING_LITERAL:
  case T_KW_TRUE:
  case T_KW_FALSE:
    return true;
  default:
    return false;
  }
}

/**
 * @brief Zjistí, zda je token otevírací závorka.
 *
 * @param tok_type Typ tokenu.
 * @return true pro `(`.
 */
static bool is_open_bracket(TokenType tok_type) {
  return tok_type == T_OPEN_BRACKET;
}

/**
 * @brief Zjistí, zda je token uzavírací závorka.
 *
 * @param tok_type Typ tokenu.
 * @return true pro `)`.
 */
static bool is_close_bracket(TokenType tok_type) {
  return tok_type == T_CLOSE_BRACKET;
}

/**
 * @brief Zjistí, zda je token binární operátor.
 *
 * Operátory jsou `+`, `-`, `*`, `/`, `==`, `!=`, `<`, `<=`, `>`, `>=`, `&&` a
 * `||`.
 *
 * @param tok_type Typ tokenu.
 * @return true, pokud jde o binární operátor.
 */
static bool is_operator_token(TokenType tok_type) {
  switch (tok_type) {
  case T_PLUS_SIGN:
  case T_MINUS_SIGN:
  case T_MULT_SIGN:
  case T_DIVIDE:
  case T_EQUAL:
  case T_NOT_EQUAL:
  case T_LT:
  case T_LTE:
  case T_GT:
  case T_GTE:
  case T_AND:
  case T_OR:
    return true;
  default:
    return false;
  }
}

/**
 * @brief Zjistí, zda je typ číselný.
 *
 * @param expr_type Typ výrazu.
 * @return true pro Int a Double.
 */
static bool is_numeric(ExprType expr_type) {
  return expr_type == EXPR_TYPE_INT || expr_type == EXPR_TYPE_DOUBLE;
}

static ExprType int_or_double(ExprType l, ExprType r);

/**
 * @brief Zredukuje logickou negaci `!` na vrcholu precedenčního zásobníku.
 *
 * Odebere značku negace, odebere typ a uzel operandu a vloží unární uzel s
 * typem Boolean. Operand musí být Boolean nebo mít dosud neznámý typ.
 *
 * @param st Precedenční zásobník.
 * @param ts Zásobník typů.
 * @param as Zásobník uzlů AST.
 * @return SUCCESS; SYN_ANAL_ERR, chybí-li operand; STATIC_SEM_TYPE_ERR při
 *   nesprávném typu; COMPILER_ERR při selhání vložení.
 */
static Error reduce_not(PrecedenceStack *st, TypeStack *ts, ASTStack *as) {
  ExprType expr_type = EXPR_TYPE_UNKNOWN;
  ASTNode *node = NULL;
  st->top--;
  if (!tstack_pop(ts, &expr_type) || !ast_stack_pop(as, &node))
    return SYN_ANAL_ERR;
  if (expr_type != EXPR_TYPE_UNKNOWN && expr_type != EXPR_TYPE_BOOL)
    return STATIC_SEM_TYPE_ERR;
  ASTNode *unary = ast_make_unary(T_NOT, node);
  if (unary)
    unary->expr_type = EXPR_TYPE_BOOL;
  if (!tstack_push(ts, EXPR_TYPE_BOOL) || !ast_stack_push(as, unary))
    return COMPILER_ERR;
  return SUCCESS;
}

/**
 * @brief Zredukuje všechny unární operátory na vrcholu precedenčního zásobníku.
 *
 * Volá se po dokončení operandu. Pro každou značku unárního operátoru odebere
 * typ a uzel operandu a vloží unární uzel. Negaci `!` řeší reduce_not(); unární
 * minus vyžaduje číselný operand (nebo dosud neznámý typ) a výsledek má typ
 * operandu.
 *
 * @param st Precedenční zásobník.
 * @param ts Zásobník typů.
 * @param as Zásobník uzlů AST.
 * @return SUCCESS; SYN_ANAL_ERR, chybí-li operand; STATIC_SEM_TYPE_ERR při
 *   nesprávném typu; COMPILER_ERR při selhání vložení.
 */
static Error reduce_unary(PrecedenceStack *st, TypeStack *ts, ASTStack *as) {
  while (st->top >= 0 && (st->items[st->top] == T_UNARY_MINUS_MARK ||
                          st->items[st->top] == T_UNARY_NOT_MARK)) {
    if (st->items[st->top] == T_UNARY_NOT_MARK) {
      Error err = reduce_not(st, ts, as);
      if (err != SUCCESS)
        return err;
      continue;
    }
    ExprType expr_type = EXPR_TYPE_UNKNOWN;
    ASTNode *node = NULL;
    st->top--;
    if (!tstack_pop(ts, &expr_type) || !ast_stack_pop(as, &node))
      return SYN_ANAL_ERR;
    if (expr_type != EXPR_TYPE_UNKNOWN && !is_numeric(expr_type))
      return STATIC_SEM_TYPE_ERR;
    ASTNode *unary = ast_make_unary(T_MINUS_SIGN, node);
    if (unary)
      unary->expr_type = expr_type;
    if (!tstack_push(ts, expr_type) || !ast_stack_push(as, unary))
      return COMPILER_ERR;
  }
  return SUCCESS;
}

/**
 * @brief Určí výsledný typ aritmetické operace nad čísly.
 *
 * Je-li aspoň jeden operand Double, výsledek je Double, jinak Int.
 *
 * @param left Typ levého operandu.
 * @param right Typ pravého operandu.
 * @return EXPR_TYPE_DOUBLE nebo EXPR_TYPE_INT.
 */
static ExprType int_or_double(ExprType left, ExprType right) {
  if (left == EXPR_TYPE_DOUBLE) {
    return EXPR_TYPE_DOUBLE;
  }
  if (right == EXPR_TYPE_DOUBLE) {
    return EXPR_TYPE_DOUBLE;
  }
  return EXPR_TYPE_INT;
}

/**
 * @brief Zredukuje binární operátor: ověří typy a sestaví uzel AST.
 *
 * Odebere ze zásobníků typy a uzly obou operandů a vloží výsledný typ a uzel
 * operace. Pravidla pro typy operandů:
 *
 * - `+` vyžaduje dvě čísla (výsledek Int, nebo Double, je-li některý operand
 *   Double) nebo dva řetězce (výsledek String),
 * - `-`, `*`, `/` vyžadují dvě čísla,
 * - `<`, `<=`, `>`, `>=` vyžadují dvě čísla nebo dva řetězce (výsledek
 *   Boolean),
 * - `==`, `!=` vyžadují dvě čísla nebo dva stejné typy (výsledek Boolean),
 * - `&&`, `||` vyžadují Boolean (výsledek Boolean).
 *
 * Je-li typ některého operandu dosud neznámý, kontrola typů se přeskočí a
 * výsledek aritmetické operace zůstane neznámý.
 *
 * @param op Typ tokenu operátoru.
 * @param tstack Zásobník typů.
 * @param ast_stack Zásobník uzlů AST.
 * @return SUCCESS; SYN_ANAL_ERR, chybí-li operand; STATIC_SEM_TYPE_ERR při
 *   nesprávných typech; COMPILER_ERR při selhání vložení.
 */
static Error apply_operator(TokenType op, TypeStack *tstack,
                            ASTStack *ast_stack) {
  ExprType right_type = EXPR_TYPE_UNKNOWN, left_type = EXPR_TYPE_UNKNOWN;
  ASTNode *right_node, *left_node;
  DEBUG_LOG("[DEBUG] apply_operator called for operator token %d\n", op);

  if (!tstack_pop(tstack, &right_type) || !tstack_pop(tstack, &left_type)) {
    DEBUG_LOG("[DEBUG] apply_operator: tstack_pop failed (SYN_ANAL_ERR)\n");
    return SYN_ANAL_ERR;
  }
  if (!ast_stack_pop(ast_stack, &right_node) ||
      !ast_stack_pop(ast_stack, &left_node)) {
    DEBUG_LOG("[DEBUG] apply_operator: ast_stack_pop failed (SYN_ANAL_ERR)\n");
    return SYN_ANAL_ERR;
  }

  ExprType result = EXPR_TYPE_UNKNOWN;

  bool known =
      left_type != EXPR_TYPE_UNKNOWN && right_type != EXPR_TYPE_UNKNOWN;

  switch (op) {
  case T_PLUS_SIGN:
    if (is_numeric(left_type) && is_numeric(right_type))
      result = int_or_double(left_type, right_type);
    else if (left_type == EXPR_TYPE_STRING && right_type == EXPR_TYPE_STRING)
      result = EXPR_TYPE_STRING;
    else if (known) {
      DEBUG_LOG(
          "[DEBUG] apply_operator (+) type mismatch (STATIC_SEM_TYPE_ERR)\n");
      return STATIC_SEM_TYPE_ERR;
    }
    break;
  case T_MINUS_SIGN:
  case T_MULT_SIGN:
  case T_DIVIDE:
    if (is_numeric(left_type) && is_numeric(right_type))
      result = int_or_double(left_type, right_type);
    else if (known) {
      DEBUG_LOG("[DEBUG] apply_operator (arithmetic) type mismatch "
                "(STATIC_SEM_TYPE_ERR)\n");
      return STATIC_SEM_TYPE_ERR;
    }
    break;
  case T_LT:
  case T_LTE:
  case T_GT:
  case T_GTE:
    if (known) {
      bool both_num = is_numeric(left_type) && is_numeric(right_type);
      bool both_str =
          left_type == EXPR_TYPE_STRING && right_type == EXPR_TYPE_STRING;
      if (!both_num && !both_str) {
        DEBUG_LOG("[DEBUG] apply_operator (relational) type mismatch "
                  "(STATIC_SEM_TYPE_ERR)\n");
        return STATIC_SEM_TYPE_ERR;
      }
    }
    result = EXPR_TYPE_BOOL;
    break;
  case T_EQUAL:
  case T_NOT_EQUAL:
    if (known) {
      bool both_num = is_numeric(left_type) && is_numeric(right_type);
      if (!both_num && left_type != right_type) {
        DEBUG_LOG("[DEBUG] apply_operator (equality) type mismatch "
                  "(STATIC_SEM_TYPE_ERR)\n");
        return STATIC_SEM_TYPE_ERR;
      }
    }
    result = EXPR_TYPE_BOOL;
    break;
  case T_AND:
  case T_OR:
    if (left_type != EXPR_TYPE_UNKNOWN && left_type != EXPR_TYPE_BOOL)
      return STATIC_SEM_TYPE_ERR;
    if (right_type != EXPR_TYPE_UNKNOWN && right_type != EXPR_TYPE_BOOL)
      return STATIC_SEM_TYPE_ERR;
    result = EXPR_TYPE_BOOL;
    break;
  default:
    result = EXPR_TYPE_UNKNOWN;
    break;
  }

  DEBUG_LOG("[DEBUG] apply_operator result type = %d\n", result);

  if (!tstack_push(tstack, result)) {
    return COMPILER_ERR;
  }

  ASTNode *binop_node = ast_make_binop(op, left_node, right_node);
  if (!ast_stack_push(ast_stack, binop_node)) {
    return COMPILER_ERR;
  }
  binop_node->expr_type = result;

  return SUCCESS;
}

/**
 * @brief Zkontroluje, zda typ argumentu odpovídá typu parametru funkce.
 *
 * Neznámý typ argumentu vyhovuje vždy. Parametr typu Double přijímá i argument
 * typu Int, ostatní typy se musí shodovat.
 *
 * @param param Typ parametru (klíčové slovo typu).
 * @param actual Typ předaného argumentu.
 * @return true, pokud je argument přípustný.
 */
static bool arg_type_ok(TokenType param, ExprType actual) {
  if (actual == EXPR_TYPE_UNKNOWN)
    return true;
  if (param == T_TYPE_BOOLEAN)
    return actual == EXPR_TYPE_BOOL;
  switch (param) {
  case T_KW_INT:
    return actual == EXPR_TYPE_INT;
  case T_KW_DOUBLE:
    return actual == EXPR_TYPE_INT || actual == EXPR_TYPE_DOUBLE;
  case T_KW_STRING:
    return actual == EXPR_TYPE_STRING;
  case T_KW_UNIT:
    return actual == EXPR_TYPE_UNIT;
  default:
    return true;
  }
}

/**
 * @brief Přeskočí konce řádků na aktuální pozici.
 *
 * @param parser Kontext parseru.
 */
static void skip_eols(ParserCtx *parser) {
  while (parser->current_token.token_type == T_EOL)
    (void)consume_token(parser);
}

/**
 * @brief Zjistí počet parametrů v zadaném seznamu parametrů funkce.
 *
 * Funkce může mít víc seznamů parametrů za sebou (rozšíření paramlists).
 * Nemá-li uložené velikosti jednotlivých seznamů, má jediný seznam se všemi
 * parametry.
 *
 * @param fn Záznam funkce v tabulce symbolů.
 * @param i Pořadí seznamu parametrů od nuly.
 * @return Počet parametrů, nebo -1, pokud takový seznam funkce nemá.
 */
static int list_size(const SymtableEntry *fn, int i) {
  if (i >= fn->f_list_count)
    return -1;
  if (fn->f_list_sizes == NULL)
    return fn->f_param_count;
  return fn->f_list_sizes[i];
}

static Error finish_call(ParserCtx *parser, const char *name, SymtableEntry *fn,
                         ASTNode *recv, ExprType *out_type, ASTNode **out_node);

/**
 * @brief Zparsuje volání funkce, jejíž jméno je aktuální token.
 *
 * Funkci vyhledá v tabulce symbolů, přeskočí její jméno a argumenty zpracuje
 * funkcí finish_call().
 *
 * @param parser Kontext parseru.
 * @param out_type Výstup: návratový typ funkce.
 * @param out_node Výstup: uzel volání v AST.
 * @return SUCCESS; UNDEF_FN_ERR, pokud funkce není definována; jinak chybový
 *   kód z funkce finish_call().
 */
static Error parse_call(ParserCtx *parser, ExprType *out_type,
                        ASTNode **out_node) {
  const char *name = parser->current_token.token_lexeme.lexeme;
  SymtableEntry *fn = NULL;
  if (!smt_search(&parser->smt, (char *)name, &fn) || !fn->function) {
    DEBUG_LOG("DEBUG: lexeme not found!: %s\n", name);
    return UNDEF_FN_ERR;
  }
  CONSUME_TOKEN_GUARD(parser);
  return finish_call(parser, name, fn, NULL, out_type, out_node);
}

// pro přepis z a.toString() -> toString(a)

/**
 * @brief Zparsuje argumenty volání a sestaví uzel volání.
 *
 * Čte seznamy argumentů v závorkách a každý argument zpracuje funkcí
 * parse_expression(). Za poslední čárkou smí být konec řádku. Špatný počet nebo
 * typ argumentů se jen zapamatuje a ohlásí se až po přečtení celého volání, aby
 * přednost měla syntaktická chyba.
 *
 * Pokud návratový typ funkce zatím není znám, nechá se tělo funkce analyzovat
 * funkcí resolve_function(). Argumenty se spojí do seznamu uzlů pod uzlem
 * AST_FUNC_CALL.
 *
 * Při volání metody přes tečku (`a.f(b)`) se přijímač `a` vloží jako první
 * argument, tedy `f(a, b)`.
 *
 * @param parser Kontext parseru; aktuální token je `(` nebo další token za
 *   voláním.
 * @param name Jméno volané funkce.
 * @param fn Záznam funkce v tabulce symbolů.
 * @param recv Přijímač volání metody, jinak NULL.
 * @param out_type Výstup: návratový typ funkce.
 * @param out_node Výstup: uzel volání v AST.
 * @return SUCCESS; STATIC_SEM_ERR při špatném počtu nebo typu argumentů;
 *   SYN_ANAL_ERR nebo LEX_ANAL_ERR při chybné syntaxi; jinak chybový kód z
 *   vnořených funkcí.
 */
static Error finish_call(ParserCtx *parser, const char *name, SymtableEntry *fn,
                         ASTNode *recv, ExprType *out_type,
                         ASTNode **out_node) {
  Error deferred = SUCCESS;
  ASTNode *head = recv, *tail = recv;
  int lists = 0;
  int offset = recv ? 1 : 0;

  while (parser->current_token.token_type == T_OPEN_BRACKET) {
    const int expected = list_size(fn, lists);
    int argc = 0;
    CONSUME_TOKEN_GUARD(parser);
    skip_eols(parser);

    if (parser->current_token.token_type == T_ERROR)
      return LEX_ANAL_ERR;
    if (parser->current_token.token_type == T_CLOSE_BRACKET) {
      CONSUME_TOKEN_GUARD(parser);
    } else {
      while (true) {
        ExprType at = EXPR_TYPE_UNKNOWN;
        ASTNode *arg = NULL;
        Error err = parse_expression(parser, &arg, &at);
        if (err != SUCCESS)
          return err;
        if (tail)
          tail->next = arg;
        else
          head = arg;
        tail = arg;
        if (argc >= expected ||
            !arg_type_ok(fn->f_params[offset + argc].type, at))
          deferred = STATIC_SEM_ERR;
        argc++;

        skip_eols(parser);

        TokenType tok_type = parser->current_token.token_type;
        if (tok_type == T_ERROR)
          return LEX_ANAL_ERR;
        if (tok_type == T_COMMA) {
          CONSUME_TOKEN_GUARD(parser);
          tok_type = parser->current_token.token_type;
          if (tok_type == T_ERROR)
            return LEX_ANAL_ERR;
          if (tok_type == T_CLOSE_BRACKET)
            return SYN_ANAL_ERR;
          skip_eols(parser);
          if (parser->current_token.token_type == T_ERROR)
            return LEX_ANAL_ERR;
          if (parser->current_token.token_type == T_CLOSE_BRACKET) {
            CONSUME_TOKEN_GUARD(parser);
            break;
          }
          continue;
        }
        if (tok_type == T_CLOSE_BRACKET) {
          CONSUME_TOKEN_GUARD(parser);
          break;
        }
        return SYN_ANAL_ERR;
      }
    }

    if (argc != expected)
      deferred = STATIC_SEM_ERR;
    if (expected > 0)
      offset += expected;
    lists++;
  }

  if (lists != fn->f_list_count)
    deferred = STATIC_SEM_ERR;
  if (deferred != SUCCESS)
    return deferred;
  if (fn->type == T_ERROR && fn->body_state == BODY_PENDING) {
    int status = resolve_function(parser, name, fn);
    if (status != SUCCESS)
      return (Error)status;
  }
  if (fn->type == T_ERROR)
    return STATIC_SEM_TYPE_ERR;
  *out_type = expr_type_from_kw(fn->type);
  ASTNode *node = ast_create_node(AST_FUNC_CALL);
  if (node != NULL) {
    node->data.func_call.name = strdup(name);
    node->data.func_call.arguments = head; // bylo NULL
    node->expr_type = *out_type;
  }
  *out_node = node;
  return SUCCESS;
}

/**
 * @brief Vytvoří kopii řetězce bez podtržítek.
 *
 * Používá se pro číselné literály, v nichž podtržítko odděluje číslice
 * (`1_000`).
 *
 * @param str Vstupní řetězec ukončený nulovým znakem.
 * @return Nový řetězec z haldy, který uvolňuje volající; NULL při nedostatku
 *   paměti.
 */
static char *strip_us(const char *str) {
  char *result = malloc(strlen(str) + 1), *out = result;
  if (!result)
    return NULL;
  for (; *str; str++)
    if (*str != '_')
      *out++ = *str;
  *out = 0;
  return result;
}

/**
 * @brief Zjistí, zda je jméno vestavěnou metodou pro daný typ.
 *
 * Metoda `toString` platí pro všechny typy, `toInt` a `floor` pro Double a
 * `length` pro String.
 *
 * @param member Jméno metody.
 * @param expr_type Typ přijímače.
 * @return true, pokud jde o vestavěnou metodu.
 */
static bool is_builtin_member(const char *member, ExprType expr_type) {
  if (!strcmp(member, "toString"))
    return true;
  if (!strcmp(member, "toInt") || !strcmp(member, "floor"))
    return expr_type == EXPR_TYPE_DOUBLE;
  if (!strcmp(member, "length"))
    return expr_type == EXPR_TYPE_STRING;
  return false;
}

// klíč rozšiřující metody v tabulce symbolů jako je"Typ$metoda"

/**
 * @brief Sestaví klíč rozšiřující metody v tabulce symbolů.
 *
 * Klíč má tvar `Typ$metoda`, například `Int$double`.
 *
 * @param expr_type Typ přijímače.
 * @param member Jméno metody.
 * @return Nový řetězec z haldy, který uvolňuje volající; NULL pro neznámý typ
 *   nebo při nedostatku paměti.
 */
static char *ext_method_key(ExprType expr_type, const char *member) {
  const char *tn;
  switch (expr_type) {
  case EXPR_TYPE_INT:
    tn = "Int";
    break;
  case EXPR_TYPE_DOUBLE:
    tn = "Double";
    break;
  case EXPR_TYPE_STRING:
    tn = "String";
    break;
  case EXPR_TYPE_UNIT:
    tn = "Unit";
    break;
  case EXPR_TYPE_BOOL:
    tn = "Boolean";
    break;
  default:
    return NULL;
  }
  size_t len = strlen(tn) + 1 + strlen(member) + 1;
  char *key = malloc(len);
  if (key != NULL)
    snprintf(key, len, "%s$%s", tn, member);
  return key;
}

/**
 * @brief Zpracuje řetězec volání metod přes tečku za operandem.
 *
 * Pro každé `.jméno` odebere ze zásobníků přijímač a vloží výsledek volání.
 * Nejdřív se hledá rozšiřující metoda (funkce `Typ$metoda`), u vestavěných
 * metod `toString`, `toInt`, `floor` a `length` se sestaví volání funkcí toStr,
 * toInt, $floor a strLength. Známý typ přijímače se kontroluje.
 *
 * @param parser Kontext parseru.
 * @param ts Zásobník typů.
 * @param as Zásobník uzlů AST.
 * @return SUCCESS; UNDEF_FN_ERR pro neznámou metodu; STATIC_SEM_TYPE_ERR pro
 *   nevhodný typ přijímače; jinak chybový kód z vnořených funkcí.
 */
static Error apply_member(ParserCtx *parser, TypeStack *ts, ASTStack *as) {
  while (parser->current_token.token_type == T_DOT) {
    CONSUME_TOKEN_GUARD(parser);
    if (parser->current_token.token_type == T_ERROR)
      return LEX_ANAL_ERR;
    if (parser->current_token.token_type != T_IDENTIFICATOR)
      return SYN_ANAL_ERR;

    const char *member = parser->current_token.token_lexeme.lexeme;
    ExprType expr_type = EXPR_TYPE_UNKNOWN;
    ASTNode *recv = NULL;
    if (!tstack_pop(ts, &expr_type) || !ast_stack_pop(as, &recv))
      return SYN_ANAL_ERR;

    if (!is_builtin_member(member, expr_type)) {
      char *key = ext_method_key(expr_type, member);
      SymtableEntry *ext = NULL;
      if (key != NULL && smt_search_global(&parser->smt, key, &ext) &&
          ext->function) {
        CONSUME_TOKEN_GUARD(parser);
        ExprType rt = EXPR_TYPE_UNKNOWN;
        ASTNode *call = NULL;
        Error err = finish_call(parser, key, ext, recv, &rt, &call);
        free(key);
        if (err != SUCCESS)
          return err;
        if (!tstack_push(ts, rt) || !ast_stack_push(as, call))
          return COMPILER_ERR;
        continue;
      }
      free(key);
    }

    const char *fn;
    ExprType res;
    bool known = (expr_type != EXPR_TYPE_UNKNOWN);
    if (!strcmp(member, "toString")) {
      fn = "toStr";
      res = EXPR_TYPE_STRING;
    } else if (!strcmp(member, "toInt")) {
      fn = "toInt";
      res = EXPR_TYPE_INT;
      if (known && expr_type != EXPR_TYPE_DOUBLE)
        return STATIC_SEM_TYPE_ERR;
    } else if (!strcmp(member, "floor")) {
      fn = "$floor";
      res = EXPR_TYPE_DOUBLE;
      if (known && expr_type != EXPR_TYPE_DOUBLE)
        return STATIC_SEM_TYPE_ERR;
    } else if (!strcmp(member, "length")) {
      fn = "strLength";
      res = EXPR_TYPE_INT;
      if (known && expr_type != EXPR_TYPE_STRING)
        return STATIC_SEM_TYPE_ERR;
    } else {
      return UNDEF_FN_ERR;
    }

    ASTNode *call = ast_create_node(AST_FUNC_CALL);
    if (!call)
      return COMPILER_ERR;
    call->data.func_call.name = strdup(fn);
    call->data.func_call.arguments = recv;
    call->expr_type = res;
    if (!tstack_push(ts, res) || !ast_stack_push(as, call))
      return COMPILER_ERR;

    CONSUME_TOKEN_GUARD(parser);
  }
  return SUCCESS;
}

static Error parse_expression_core(ParserCtx *parser, ASTNode **out_ast,
                                   ExprType *out_type);

Error parse_expression(ParserCtx *parser, ASTNode **out_ast,
                       ExprType *out_type) {
  PrecedenceStack saved = parser->stack;
  Error err = parse_expression_core(parser, out_ast, out_type);
  parser->stack = saved;
  return err;
}

/**
 * @brief Zparsuje výraz precedenční analýzou (jádro funkce parse_expression()).
 *
 * Tokeny čte po jednom a rozhoduje podle toho, zda se očekává operand. Operandy
 * (literály, identifikátory, volání funkcí, výrazy `if`) ukládá na zásobník
 * typů a zásobník uzlů, operátory a závorky na precedenční zásobník; o přesunu
 * a redukci rozhoduje precedenční tabulka. Prázdné závorky `()` jsou hodnota
 * typu Unit. Unární `-` a `!` se ukládají jako značky a redukují se po
 * dokončení operandu.
 *
 * Výraz končí na konci řádku, na konci vstupu, u čárky nebo u uzavírací závorky
 * mimo závorky výrazu. Uvnitř závorek smí výraz pokračovat na dalším řádku,
 * který musí být odsazen víc než řádek, kde výraz začal. Na konci zredukuje
 * zbylé operátory.
 *
 * @param parser Kontext parseru.
 * @param out_ast Výstup: kořen AST výrazu. Smí být NULL.
 * @param out_type Výstup: typ výrazu. Smí být NULL.
 * @return SUCCESS, jinak chybový kód.
 */
Error parse_expression_core(ParserCtx *parser, ASTNode **out_ast,
                            ExprType *out_type) {
  DEBUG_LOG("[DEBUG] === parse_expression START ===\n");
  stack_clear(&parser->stack);

  TypeStack tstack;
  tstack_init(&tstack);

  ASTStack ast_stack;
  ast_stack_init(&ast_stack);

  bool expect_operand = true;
  int bracket_depth = 0;
  const int start_indent = parser->line_indent;

  Token lookahead = look_next_eol(parser);

  DEBUG_LOG("[DEBUG] initial lookahead token_type = %d\n",
            lookahead.token_type);

  while (true) {
    if (lookahead.token_type == T_ERROR)
      return LEX_ANAL_ERR;
    DEBUG_LOG(
        "[DEBUG] loop iteration: lookahead token_type = %d, expect_operand "
        "= %d, bracket_depth = %d, stack_top = %d\n",
        lookahead.token_type, expect_operand, bracket_depth, parser->stack.top);
    if (lookahead.token_type == T_EOL && bracket_depth > 0) {
      CONSUME_TOKEN_GUARD(parser);
      lookahead = look_next_eol(parser);
      if (lookahead.token_type != T_EOL && lookahead.token_type != T_EOF &&
          parser->line_indent <= start_indent)
        return SYN_ANAL_ERR;
      continue;
    }
    bool is_end_token = false;
    switch (lookahead.token_type) {
    case T_EOL:
    case T_EOF:
    case T_COMMA:
    case T_CLOSE_BRACKET:
      if (bracket_depth == 0)
        is_end_token = true;
      break;
    default:
      break;
    }

    if (is_end_token) {
      DEBUG_LOG("[DEBUG] reached end token at depth 0, expect_operand = %d\n",
                expect_operand);
      if (expect_operand)
        return SYN_ANAL_ERR;
      break;
    }

    if (is_operand_token(lookahead.token_type) && expect_operand) {
      DEBUG_LOG("[DEBUG] branch: OPERAND token %d\n", lookahead.token_type);
      ASTNode *leaf = NULL;
      ExprType expr_type = expr_type_from_token(lookahead.token_type);
      bool is_call = false;
      if (lookahead.token_type == T_IDENTIFICATOR) {
        SymtableEntry *sym = NULL;
        is_call = peek_token(parser).token_type == T_OPEN_BRACKET ||
                  (smt_search(&parser->smt,
                              (char *)lookahead.token_lexeme.lexeme, &sym) &&
                   sym->function);
      }

      if (is_call) {
        Error err = parse_call(parser, &expr_type, &leaf);
        if (err != SUCCESS)
          return err;
      } else if (lookahead.token_type == T_IDENTIFICATOR) {
        Error err = check_identifier_in_expr(parser, &lookahead, &expr_type);
        if (err != SUCCESS)
          return err;

        leaf = ast_create_node(AST_IDENTIFIER);
        leaf->data.string_val = strdup((char *)lookahead.token_lexeme.lexeme);
      } else if (lookahead.token_type == T_INT_LITERAL) {
        char *clean = strip_us(lookahead.token_lexeme.lexeme);
        leaf = ast_make_int(clean ? atoi(clean) : 0);
        free(clean);
      } else if (lookahead.token_type == T_DOUBLE_LITERAL) {
        char *clean = strip_us(lookahead.token_lexeme.lexeme);
        leaf = ast_make_double(clean ? strtod(clean, NULL) : 0.0);
        free(clean);
      } else if (lookahead.token_type == T_STRING_LITERAL) {
        leaf = ast_make_string(strdup((char *)lookahead.token_lexeme.lexeme));
      } else if (lookahead.token_type == T_KW_TRUE ||
                 lookahead.token_type == T_KW_FALSE) {
        leaf = ast_make_bool(lookahead.token_type == T_KW_TRUE);
      } else {
        leaf = ast_create_node(AST_LITERAL_NULL);
      }
      if (leaf)
        leaf->expr_type = expr_type;
      if (!tstack_push(&tstack, expr_type) ||
          !ast_stack_push(&ast_stack, leaf)) {
        return COMPILER_ERR;
      }
      if (!is_call) {
        CONSUME_TOKEN_GUARD(parser);
      }
      expect_operand = false;
      Error merr = apply_member(parser, &tstack, &ast_stack);
      if (merr != SUCCESS)
        return merr;
      Error uerr = reduce_unary(&parser->stack, &tstack, &ast_stack);
      if (uerr != SUCCESS)
        return uerr;

    } else if (lookahead.token_type == T_KW_IF && expect_operand) {
      DEBUG_LOG("[DEBUG] branch: IF EXPRESSION\n");
      ASTNode *leaf = NULL;
      int t_int = EXPR_TYPE_UNKNOWN;
      int perr = parse_if_expression(parser, &leaf, &t_int);
      if (perr != SUCCESS)
        return (Error)perr;
      ExprType expr_type = (ExprType)t_int;
      if (leaf)
        leaf->expr_type = expr_type;
      if (!tstack_push(&tstack, expr_type) || !ast_stack_push(&ast_stack, leaf))
        return COMPILER_ERR;
      expect_operand = false;
      Error merr = apply_member(parser, &tstack, &ast_stack);
      if (merr != SUCCESS)
        return merr;
      Error uerr = reduce_unary(&parser->stack, &tstack, &ast_stack);
      if (uerr != SUCCESS)
        return uerr;

    } else if (is_open_bracket(lookahead.token_type) && expect_operand) {
      DEBUG_LOG("[DEBUG] branch: OPEN BRACKET (\n");
      stack_push(&parser->stack, lookahead.token_type);
      CONSUME_TOKEN_GUARD(parser);
      bracket_depth++;

    } else if (lookahead.token_type == T_MINUS_SIGN && expect_operand) {
      DEBUG_LOG("[DEBUG] branch: UNARY MINUS\n");
      stack_push(&parser->stack, T_UNARY_MINUS_MARK);
      CONSUME_TOKEN_GUARD(parser);

    } else if (lookahead.token_type == T_NOT && expect_operand) {
      DEBUG_LOG("[DEBUG] branch: UNARY NOT\n");
      stack_push(&parser->stack, T_UNARY_NOT_MARK);
      CONSUME_TOKEN_GUARD(parser);

    } else if (is_close_bracket(lookahead.token_type)) {
      DEBUG_LOG("[DEBUG] branch: CLOSE BRACKET ), expect_operand = %d\n",
                expect_operand);
      if (expect_operand) {
        if (parser->stack.top >= 0 &&
            is_open_bracket(parser->stack.items[parser->stack.top])) {
          DEBUG_LOG("[DEBUG] found empty brackets ()\n");
          stack_pop(&parser->stack);
          bracket_depth--;

          ExprType expr_type = EXPR_TYPE_UNIT;
          ASTNode *unit_node = ast_create_node(AST_LITERAL_UNIT);

          unit_node->expr_type = EXPR_TYPE_UNIT;

          if (!tstack_push(&tstack, expr_type) ||
              !ast_stack_push(&ast_stack, unit_node)) {
            return COMPILER_ERR;
          }
          CONSUME_TOKEN_GUARD(parser);
          expect_operand = false;
          Error merr = apply_member(parser, &tstack, &ast_stack);
          if (merr != SUCCESS)
            return merr;
          Error uerr = reduce_unary(&parser->stack, &tstack, &ast_stack);
          if (uerr != SUCCESS)
            return uerr;
        } else {
          DEBUG_LOG("[DEBUG] empty bracket check failed, stack top "
                    "mismatch (SYN_ANAL_ERR)\n");
          return SYN_ANAL_ERR;
        }
      } else {
        bool found = false;
        while (parser->stack.top >= 0) {
          TokenType top = parser->stack.items[parser->stack.top];
          DEBUG_LOG(
              "[DEBUG] closing bracket cleanup loop: inspecting stack top "
              "token = %d\n",
              top);
          if (is_open_bracket(top)) {
            found = true;
            stack_pop(&parser->stack);
            bracket_depth--;
            break;
          } else if (is_operator_token(top)) {
            stack_pop(&parser->stack);
            Error err = apply_operator(top, &tstack, &ast_stack);
            if (err != SUCCESS)
              return err;
          } else {
            DEBUG_LOG(
                "[DEBUG] unexpected token in bracket cleanup (SYN_ANAL_ERR)\n");
            return SYN_ANAL_ERR;
          }
        }
        if (!found) {
          DEBUG_LOG("[DEBUG] open bracket not found for closing bracket "
                    "(SYN_ANAL_ERR)\n");
          return SYN_ANAL_ERR;
        }
        CONSUME_TOKEN_GUARD(parser);
        expect_operand = false;
        Error merr = apply_member(parser, &tstack, &ast_stack);
        if (merr != SUCCESS)
          return merr;
        Error uerr = reduce_unary(&parser->stack, &tstack, &ast_stack);
        if (uerr != SUCCESS)
          return uerr;
      }
    } else if (is_operator_token(lookahead.token_type) && !expect_operand) {
      TokenType next = lookahead.token_type;
      DEBUG_LOG("[DEBUG] branch: OPERATOR token %d\n", next);
      bool pushed = false;
      while (!pushed) {
        TokenType top = top_terminal(&parser->stack);
        DEBUG_LOG(
            "[DEBUG] precedence loop: top terminal = %d, next operator = %d\n",
            top, next);
        ExprColumn left = token_to_column(top);
        ExprColumn right = token_to_column(next);

        if (left == EX_COUNT || right == EX_COUNT)
          return SYN_ANAL_ERR;

        PrecedenceAction action = precedence_table[left][right];
        switch (action) {
        case PREC_SHIFT:
          stack_push(&parser->stack, next);
          CONSUME_TOKEN_GUARD(parser);
          expect_operand = true;
          pushed = true;
          break;
        case PREC_REDUCE: {
          stack_pop(&parser->stack);
          if (!is_operator_token(top))
            return SYN_ANAL_ERR;

          Error err = apply_operator(top, &tstack, &ast_stack);
          if (err != SUCCESS)
            return err;
          break;
        }
        case PREC_EQUAL:
          return SYN_ANAL_ERR;
        case PREC_ERROR:
        default:
          return SYN_ANAL_ERR;
        }
      }
    } else {
      if (expect_operand)
        return SYN_ANAL_ERR;
      break;
    }
    lookahead = look_next_eol(parser);
  }

  while (parser->stack.top >= 0) {
    TokenType top = parser->stack.items[parser->stack.top];
    if (is_open_bracket(top) || is_close_bracket(top))
      return SYN_ANAL_ERR;
    if (!is_operator_token(top))
      return SYN_ANAL_ERR;

    stack_pop(&parser->stack);
    Error err = apply_operator(top, &tstack, &ast_stack);
    if (err != SUCCESS)
      return err;
  }

  ExprType final_type = EXPR_TYPE_UNKNOWN;
  tstack_peek(&tstack, &final_type);
  if (out_type)
    *out_type = final_type;

  ASTNode *final_ast = NULL;
  ast_stack_pop(&ast_stack, &final_ast);
  if (out_ast)
    *out_ast = final_ast;

  return SUCCESS;
}
