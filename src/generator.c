/**
 * @file generator.c
 * @brief Generátor cílového kódu IFJcode26 z abstraktního syntaktického stromu
 *
 * Generátor prochází AST vytvořený parserem a vypisuje kód IFJcode26.
 * Použité principy:
 *  - výrazy se počítají zásobníkově, výsledek je vždy na datovém zásobníku,
 *  - všechny DEFVAR funkce se vypisují najednou na jejím začátku,
 *  - proměnné se jmenují LF@$v<id>, kde id je pořadové číslo deklarace
 *    (parametry mají id 0..k-1),
 *  - návratová hodnota funkce se předává přes globální proměnnou GF@_ret,
 *  - vestavěné funkce se generují přímo v místě volání, složitější
 *    (ord, subStr, celočíselné dělení) jako pomocné rutiny volané přes CALL,
 *  - výstup se zapíše na stdout až po úspěšném průchodu celým AST.
 */

#include "generator.h"
#include "DynBuff.h"
#include "Error.h"
#include "lexer.h"
#include "precedence.h" /* ExprType (EXPR_TYPE_*) */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Záznam pro mapování jména proměnné na její číslo v generovaném kódu
 *
 * Díky číslování se stejně pojmenované proměnné z různých bloků (shadowing)
 * v IFJcode26 nepletou, každá z nich je jiná proměnná LF@$v<id>.
 */
typedef struct {
  const char *name; /**< jméno proměnné (ukazuje do AST, není kopírováno) */
  int id;           /**< číslo proměnné, výsledné jméno je LF@$v<id> */
} VarRec;

/**
 * @brief Kontext generátoru
 *
 * Veškerý proměnlivý stav generátoru je soustředěn v této struktuře.
 * Generátor díky tomu nepoužívá globální stav a jednotlivá volání
 * generate_program() jsou na sobě nezávislá.
 */
typedef struct GeneratorContext {
  StringBuff out; /**< buffer s vygenerovaným kódem */

  int label_cnt; /**< čítač pro unikátní návěští (if, while) */
  int var_cnt;   /**< číslo, které dostane další deklarovaná proměnná */
  int failed;    /**< příznak chyby, po jeho nastavení se přestane generovat */

  ASTNode *cur_fn; /**< funkce, která se právě generuje */
  ASTNode *prog;   /**< kořen AST (AST_PROGRAM) */

  VarRec *vars;    /**< zásobník viditelných proměnných (jméno -> id) */
  size_t vars_n;   /**< počet použitých záznamů v poli vars */
  size_t vars_cap; /**< alokovaná kapacita pole vars */
} GeneratorContext;

/* ===== emitování ===== */

/**
 * @brief Naformátuje text (ve stylu printf) a připojí ho do výstupního bufferu
 *
 * Délka jedné instrukce není omezena, velikost pomocného bufferu se zjistí
 * z vsnprintf. Při chybě (alokace, formátování) se nastaví ctx->failed.
 *
 * @param ctx: kontext generátoru
 * @param fmt: formátovací řetězec ve stylu printf
 */
static void emit(GeneratorContext *ctx, const char *fmt, ...) {
  if (!ctx || ctx->failed) {
    return;
  }

  va_list args;
  va_list args_copy;
  va_start(args, fmt);
  va_copy(args_copy, args);

  int needed = vsnprintf(NULL, 0, fmt, args_copy);
  va_end(args_copy);

  if (needed < 0) {
    ctx->failed = 1;
    va_end(args);
    return;
  }

  size_t buffer_size = (size_t)needed + 1;
  char *tmp = malloc(buffer_size);
  if (!tmp) {
    ctx->failed = 1;
    va_end(args);
    return;
  }

  int written = vsnprintf(tmp, buffer_size, fmt, args);
  va_end(args);

  if (written < 0 || written != needed) {
    ctx->failed = 1;
    free(tmp);
    return;
  }

  sb_append_str(&ctx->out, tmp);
  free(tmp);
}

/**
 * @brief Zapíše řetězcový literál ve tvaru konstanty IFJcode26 (string@...)
 *
 * Znaky s ASCII kódem 0-32, 35 (#), 92 (zpětné lomítko) a 127 se zapisují jako escape
 * sekvence \xyz, kde xyz je třímístný dekadický kód znaku.
 *
 * @param ctx: kontext generátoru
 * @param s: řetězec ke vložení (už se skutečnými znaky, ne escape sekvencemi)
 */
static void emit_string_lit(GeneratorContext *ctx, const char *s) {
  if (!ctx || ctx->failed || !s) {
    if (ctx) {
      ctx->failed = 1;
    }
    return;
  }

  sb_append_str(&ctx->out, "string@");
  for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
    /* IFJcode26 vyžaduje escapovat řídicí znaky, # a zpětné lomítko. */
    if (*p <= 32 || *p == 35 || *p == 92 || *p == 127) {
      char escaped[8];
      snprintf(escaped, sizeof escaped, "\\%03u", (unsigned)*p);
      sb_append_str(&ctx->out, escaped);
    } else {
      sb_append_char(&ctx->out, (char)*p);
    }
  }
}

/* ===== proměnné: jméno -> číslo (LF@$v<id>) ===== */

/**
 * @brief Zaregistruje novou proměnnou a přidělí jí další volné číslo
 *
 * Záznam zůstane platný, dokud se na konci bloku nezkrátí zásobník
 * proměnných (ctx->vars_n), tím končí i viditelnost proměnné.
 *
 * @param ctx: kontext generátoru
 * @param name: jméno proměnné (ukazatel se nekopíruje, ukazuje do AST)
 * @return číslo přidělené proměnné, při chybě 0 a nastavené ctx->failed
 */
static int declare(GeneratorContext *ctx, const char *name) {
  if (!ctx || !name) {
    if (ctx) {
      ctx->failed = 1;
    }
    return 0;
  }

  if (ctx->vars_n == ctx->vars_cap) {
    size_t new_cap = ctx->vars_cap ? ctx->vars_cap * 2 : 32;
    if (new_cap < ctx->vars_cap ||
        new_cap > (size_t)-1 / sizeof *ctx->vars) {
      ctx->failed = 1;
      return 0;
    }

    VarRec *new_vars = realloc(ctx->vars, new_cap * sizeof *ctx->vars);
    if (!new_vars) {
      ctx->failed = 1;
      return 0;
    }

    ctx->vars = new_vars;
    ctx->vars_cap = new_cap;
  }

  ctx->vars[ctx->vars_n].name = name;
  ctx->vars[ctx->vars_n].id = ctx->var_cnt;
  ctx->vars_n++;
  return ctx->var_cnt++;
}

/**
 * @brief Najde číslo nejbližší (nejvnitřnější) viditelné proměnné daného jména
 *
 * Hledá se od konce zásobníku proměnných, takže vnořená deklarace překryje
 * stejně pojmenovanou proměnnou z nadřazeného bloku.
 *
 * @param ctx: kontext generátoru
 * @param name: jméno hledané proměnné
 * @return číslo proměnné, při nenalezení 0 a nastavené ctx->failed
 */
static int lookup(GeneratorContext *ctx, const char *name) {
  if (!ctx || !name) {
    if (ctx) {
      ctx->failed = 1;
    }
    return 0;
  }

  for (size_t i = ctx->vars_n; i > 0; i--) {
    if (strcmp(ctx->vars[i - 1].name, name) == 0) {
      return ctx->vars[i - 1].id;
    }
  }

  ctx->failed = 1; /* parser/sémantická analýza měla chybu odchytit dříve */
  return 0;
}

/* ===== pomocné ===== */

/**
 * @brief Převede hodnotu na vrcholu zásobníku z typu from na typ to
 *
 * Jazyk IFJ26 má jedinou implicitní konverzi, rozšíření Int na Double.
 * Ve všech ostatních případech se nevygeneruje nic.
 *
 * @param ctx: kontext generátoru
 * @param from: typ hodnoty na zásobníku (hodnota ExprType)
 * @param to: požadovaný typ (hodnota ExprType)
 */
static void convert(GeneratorContext *ctx, int from, int to) {
  if (from == EXPR_TYPE_INT && to == EXPR_TYPE_DOUBLE) {
    emit(ctx, "INT2FLOATS\n");
  }
}

/**
 * @brief Vyhledá definici uživatelské funkce v AST podle jména
 *
 * @param ctx: kontext generátoru
 * @param name: jméno hledané funkce
 * @return uzel AST_FUNC_DEF, nebo NULL (volaná funkce je pak vestavěná)
 */
static ASTNode *find_func(GeneratorContext *ctx, const char *name) {
  if (!ctx || !ctx->prog || !name) {
    return NULL;
  }

  for (ASTNode *f = ctx->prog->data.block.statements; f; f = f->next) {
    if (f->type == AST_FUNC_DEF &&
        strcmp(f->data.func_def.name, name) == 0) {
      return f;
    }
  }

  return NULL;
}

/**
 * @brief Spočítá prvky seznamu uzlů propojeného přes ukazatel next
 *
 * @param p: první uzel seznamu (např. parametry funkce), může být NULL
 * @return počet uzlů v seznamu
 */
static int count_list(ASTNode *p) {
  int n = 0;
  for (; p; p = p->next) {
    n++;
  }
  return n;
}

/**
 * @brief Spočítá deklarace proměnných v bloku včetně vnořených bloků
 *
 * Rekurzivně prochází větve if a těla while. Výsledek určuje, kolik příkazů
 * DEFVAR je potřeba vypsat na začátku funkce (viz gen_function).
 *
 * @param block: uzel AST_BLOCK, může být NULL
 * @return počet příkazů AST_VAR_DECL v bloku a ve všech jeho podblocích
 */
static int count_decls(ASTNode *block) {
  int n = 0;
  if (!block) {
    return 0;
  }

  for (ASTNode *s = block->data.block.statements; s; s = s->next) {
    switch (s->type) {
    case AST_VAR_DECL:
      n++;
      break;
    case AST_IF:
      n += count_decls(s->data.if_stmt.body);
      n += count_decls(s->data.if_stmt.else_body);
      break;
    case AST_WHILE:
      n += count_decls(s->data.while_loop.body);
      break;
    default:
      break;
    }
  }

  return n;
}

/**
 * @brief Zjistí, zda je uzel výraz (a ne příkaz)
 *
 * @param s: uzel AST, může být NULL
 * @return 1 pokud jde o výraz (literál, identifikátor, operátor, volání),
 *         jinak 0
 */
static int is_expr(ASTNode *s) {
  if (!s) {
    return 0;
  }

  switch (s->type) {
  case AST_FUNC_CALL:
  case AST_BIN_OP:
  case AST_UNARY_OP:
  case AST_IDENTIFIER:
  case AST_LITERAL_INT:
  case AST_LITERAL_FLOAT:
  case AST_LITERAL_STRING:
  case AST_LITERAL_UNIT:
  case AST_LITERAL_NULL:
    return 1;
  default:
    return 0;
  }
}

/* ===== výrazy: výsledek vždy na datovém zásobníku ===== */
static void gen_expr(GeneratorContext *ctx, ASTNode *e);
static void gen_block(GeneratorContext *ctx, ASTNode *b);

/**
 * @brief Vygeneruje volání vestavěné funkce přímo v místě volání
 *
 * Argumenty se vyhodnotí zleva doprava na datový zásobník. Podle jména
 * funkce a statického typu argumentu se pak vypíše odpovídající instrukce.
 * Výsledek funkce (u print hodnota Unit) zůstane na zásobníku.
 * Funkce ord a subStr jsou pomocné rutiny volané přes CALL.
 *
 * @param ctx: kontext generátoru
 * @param c: uzel AST_FUNC_CALL s jménem vestavěné funkce
 */
static void gen_builtin(GeneratorContext *ctx, ASTNode *c) {
  const char *name = c->data.func_call.name;
  ASTNode *args = c->data.func_call.arguments;

  /* Argumenty vestavěných funkcí se vyhodnocují zleva doprava. */
  for (ASTNode *arg = args; arg; arg = arg->next) {
    gen_expr(ctx, arg);
  }

  if (!strcmp(name, "print")) {
    emit(ctx, "POPS GF@_t1\n");
    if (args && args->expr_type == EXPR_TYPE_UNIT) {
      emit(ctx, "WRITE string@()\n");
    } else {
      emit(ctx, "WRITE GF@_t1\n");
    }
    emit(ctx, "PUSHS nil@nil\n");
  } else if (!strcmp(name, "readLine")) {
    emit(ctx, "READ GF@_t1 string\nPUSHS GF@_t1\n");
  } else if (!strcmp(name, "readInt")) {
    emit(ctx, "CALL $$readint\n");
  } else if (!strcmp(name, "readDouble")) {
    emit(ctx, "CALL $$readdouble\n");
  } else if (!strcmp(name, "toStr")) {
    if (args->expr_type == EXPR_TYPE_INT) {
      emit(ctx, "INT2STRS\n");
    } else if (args->expr_type == EXPR_TYPE_DOUBLE) {
      emit(ctx, "FLOAT2STRS\n");
    } else if (args->expr_type == EXPR_TYPE_UNIT) {
      emit(ctx, "POPS GF@_t1\nPUSHS string@()\n");
    }
    /* String: hodnota už je na zásobníku. */
  } else if (!strcmp(name, "toInt")) {
    emit(ctx, "FLOAT2INTS\n");
  } else if (!strcmp(name, "strLength")) {
    emit(ctx, "POPS GF@_t1\nSTRLEN GF@_t2 GF@_t1\nPUSHS GF@_t2\n");
  } else if (!strcmp(name, "chr")) {
    emit(ctx, "INT2CHARS\n"); /* mimo 0..127 -> chyba 58 interpretu */
  } else if (!strcmp(name, "ord")) {
    emit(ctx, "CALL $$ord\n");
  } else if (!strcmp(name, "subStr")) {
    emit(ctx, "CALL $$substr\n");
  } else {
    ctx->failed = 1;
  }
}

/**
 * @brief Vygeneruje volání funkce (uživatelské, jinak vestavěné)
 *
 * Konvence volání uživatelské funkce:
 *  - volající vyhodnotí všechny argumenty na datový zásobník,
 *  - vytvoří nový dočasný rámec a argumenty z něj vyzvedne odzadu do
 *    proměnných TF@$v<i> (po PUSHFRAME ve volané funkci jsou to LF@$v<i>),
 *  - po CALL volající uloží návratovou hodnotu z GF@_ret na zásobník.
 *
 * @param ctx: kontext generátoru
 * @param c: uzel AST_FUNC_CALL
 */
static void gen_call(GeneratorContext *ctx, ASTNode *c) {
  const char *name = c->data.func_call.name;
  ASTNode *fn = find_func(ctx, name);

  if (!fn) {
    gen_builtin(ctx, c);
    return;
  }

  /*
   * 1) Všechny argumenty vyhodnotíme před vytvořením TF.
   *    Vnořená volání tak nemohou přepsat dočasný rámec volající funkce.
   */
  int n = 0;
  ASTNode *param = fn->data.func_def.params;
  for (ASTNode *arg = c->data.func_call.arguments;
       arg; arg = arg->next, n++) {
    gen_expr(ctx, arg);
    if (param) {
      convert(ctx, arg->expr_type, param->expr_type);
      param = param->next;
    }
  }

  /* 2) Nový rámec; argumenty se vyzvednou ze zásobníku odzadu. */
  emit(ctx, "CREATEFRAME\n");
  for (int i = n - 1; i >= 0; i--) {
    emit(ctx, "DEFVAR TF@$v%d\nPOPS TF@$v%d\n", i, i);
  }
  emit(ctx, "CALL $fn_%s\nPUSHS GF@_ret\n", name);
}

/**
 * @brief Vygeneruje binární operátor
 *
 * Nejprve se vyhodnotí levý a pravý operand (v tomto pořadí). Pokud je jeden
 * z nich Int a druhý Double, Int se převede na Double. Pak se vypíše
 * instrukce operátoru. Zvláštní případy:
 *  - + nad řetězci je CONCAT (není zásobníková, jde přes pomocné proměnné),
 *  - / nad dvěma Int volá rutinu $$idiv (zaokrouhlení k nule),
 *  - <= a >= se skládají z <, > a ==, protože NOT s opačným operátorem
 *    by dával chybný výsledek pro NaN.
 *
 * @param ctx: kontext generátoru
 * @param e: uzel AST_BIN_OP
 */
static void gen_binop(GeneratorContext *ctx, ASTNode *e) {
  ASTNode *left = e->data.bin_op.left;
  ASTNode *right = e->data.bin_op.right;
  int op = e->data.bin_op.operator_type;

  gen_expr(ctx, left);
  if (left->expr_type == EXPR_TYPE_INT &&
      right->expr_type == EXPR_TYPE_DOUBLE) {
    emit(ctx, "INT2FLOATS\n");
  }

  gen_expr(ctx, right);
  if (right->expr_type == EXPR_TYPE_INT &&
      left->expr_type == EXPR_TYPE_DOUBLE) {
    emit(ctx, "INT2FLOATS\n");
  }

  int is_string = (left->expr_type == EXPR_TYPE_STRING);
  int both_int = (left->expr_type == EXPR_TYPE_INT &&
                  right->expr_type == EXPR_TYPE_INT);

  if (op == T_PLUS_SIGN && is_string) {
    emit(ctx,
         "POPS GF@_t2\nPOPS GF@_t1\nCONCAT GF@_t3 GF@_t1 GF@_t2\n"
         "PUSHS GF@_t3\n");
  } else if (op == T_PLUS_SIGN) {
    emit(ctx, "ADDS\n");
  } else if (op == T_MINUS_SIGN) {
    emit(ctx, "SUBS\n");
  } else if (op == T_MULT_SIGN) {
    emit(ctx, "MULS\n");
  } else if (op == T_DIVIDE) {
    emit(ctx, both_int ? "CALL $$idiv\n" : "DIVS\n");
  } else if (op == T_LT) {
    emit(ctx, "LTS\n");
  } else if (op == T_GT) {
    emit(ctx, "GTS\n");
  } else if (op == T_LTE) {
    /*
     * x <= y je (x < y) OR (x == y).
     * NOT (x > y) není ekvivalentní pro NaN: NOT(false) by dalo true.
     */
    emit(ctx,
         "POPS GF@_t2\nPOPS GF@_t1\n"
         "LT GF@_t3 GF@_t1 GF@_t2\n"
         "EQ GF@_t4 GF@_t1 GF@_t2\n"
         "OR GF@_t3 GF@_t3 GF@_t4\n"
         "PUSHS GF@_t3\n");
  } else if (op == T_GTE) {
    /*
     * x >= y je (x > y) OR (x == y).
     * I zde zůstává výsledek pro NaN false.
     */
    emit(ctx,
         "POPS GF@_t2\nPOPS GF@_t1\n"
         "GT GF@_t3 GF@_t1 GF@_t2\n"
         "EQ GF@_t4 GF@_t1 GF@_t2\n"
         "OR GF@_t3 GF@_t3 GF@_t4\n"
         "PUSHS GF@_t3\n");
  } else if (op == T_EQUAL) {
    emit(ctx, "EQS\n");
  } else if (op == T_NOT_EQUAL) {
    emit(ctx, "EQS\nNOTS\n");
  } else {
    ctx->failed = 1;
  }
}

/**
 * @brief Vygeneruje výraz, jehož hodnota zůstane na vrcholu datového zásobníku
 *
 * Unární mínus se generuje jako 0 - x, unární plus nemění nic.
 * Literál () (a AST_LITERAL_NULL) je hodnota nil@nil.
 *
 * @param ctx: kontext generátoru
 * @param e: uzel výrazu (literál, identifikátor, volání, operátor)
 */
static void gen_expr(GeneratorContext *ctx, ASTNode *e) {
  if (!e || ctx->failed) {
    if (!e) {
      ctx->failed = 1;
    }
    return;
  }

  switch (e->type) {
  case AST_LITERAL_INT:
    emit(ctx, "PUSHS int@%d\n", e->data.int_val);
    break;

  case AST_LITERAL_FLOAT:
    emit(ctx, "PUSHS float@%a\n", e->data.float_val);
    break;

  case AST_LITERAL_STRING:
    emit(ctx, "PUSHS ");
    emit_string_lit(ctx, e->data.string_val);
    emit(ctx, "\n");
    break;

  case AST_LITERAL_UNIT:
  case AST_LITERAL_NULL:
    emit(ctx, "PUSHS nil@nil\n");
    break;

  case AST_IDENTIFIER:
    emit(ctx, "PUSHS LF@$v%d\n", lookup(ctx, e->data.string_val));
    break;

  case AST_FUNC_CALL:
    gen_call(ctx, e);
    break;

  case AST_BIN_OP:
    gen_binop(ctx, e);
    break;

  case AST_UNARY_OP:
    if (e->data.unary_op.operator_type == T_MINUS_SIGN) {
      if (e->expr_type == EXPR_TYPE_DOUBLE) {
        emit(ctx, "PUSHS float@%a\n", 0.0);
      } else {
        emit(ctx, "PUSHS int@0\n");
      }
      gen_expr(ctx, e->data.unary_op.operand);
      emit(ctx, "SUBS\n");
    } else {
      gen_expr(ctx, e->data.unary_op.operand);
    }
    break;

  default:
    ctx->failed = 1;
    break;
  }
}

/* ===== příkazy ===== */

/**
 * @brief Do GF@_ret uloží hodnotu výrazu jako návratovou hodnotu funkce
 *
 * Hodnota se před uložením převede na návratový typ funkce. Ve funkci
 * s návratovým typem Unit se hodnota výrazu vyhodnotí, zahodí a vrací se nil.
 * Instrukce POPFRAME a RETURN se nevypisují, ty doplňuje volající funkce.
 *
 * @param ctx: kontext generátoru
 * @param expr: vracený výraz, NULL pro return bez hodnoty
 */
static void gen_return_value(GeneratorContext *ctx, ASTNode *expr) {
  int return_type = ctx->cur_fn->expr_type;

  if (return_type == EXPR_TYPE_UNIT || !expr) {
    if (expr) {
      gen_expr(ctx, expr);
      emit(ctx, "POPS GF@_t1\n");
    }
    emit(ctx, "MOVE GF@_ret nil@nil\n");
  } else {
    gen_expr(ctx, expr);
    convert(ctx, expr->expr_type, return_type);
    emit(ctx, "POPS GF@_ret\n");
  }
}

/**
 * @brief Vygeneruje jeden příkaz
 *
 * Řeší definici proměnné, přiřazení, return, if a while. Samostatný výraz
 * se vyhodnotí a jeho hodnota se zahodí ze zásobníku. Každý příkaz nechá
 * datový zásobník ve stejném stavu, v jakém byl před ním.
 *
 * @param ctx: kontext generátoru
 * @param s: uzel příkazu
 */
static void gen_stmt(GeneratorContext *ctx, ASTNode *s) {
  if (!s || ctx->failed) {
    if (!s) {
      ctx->failed = 1;
    }
    return;
  }

  switch (s->type) {
  case AST_VAR_DECL: {
    ASTNode *init = s->data.var_assign.expression;
    gen_expr(ctx, init);
    convert(ctx, init->expr_type, s->expr_type);
    int id = declare(ctx, s->data.var_assign.identifier); /* až po inicializaci */
    emit(ctx, "POPS LF@$v%d\n", id);
    break;
  }

  case AST_ASSIGNMENT: {
    ASTNode *rhs = s->data.var_assign.expression;
    gen_expr(ctx, rhs);
    convert(ctx, rhs->expr_type, s->expr_type);
    emit(ctx, "POPS LF@$v%d\n",
         lookup(ctx, s->data.var_assign.identifier));
    break;
  }

  case AST_RETURN:
    gen_return_value(ctx, s->data.return_stmt.expression);
    emit(ctx, "POPFRAME\nRETURN\n");
    break;

  case AST_IF: {
    int id = ctx->label_cnt++;
    gen_expr(ctx, s->data.if_stmt.condition);
    emit(ctx, "PUSHS bool@false\nJUMPIFEQS $else_%d\n", id);
    gen_block(ctx, s->data.if_stmt.body);
    emit(ctx, "JUMP $endif_%d\nLABEL $else_%d\n", id, id);
    gen_block(ctx, s->data.if_stmt.else_body);
    emit(ctx, "LABEL $endif_%d\n", id);
    break;
  }

  case AST_WHILE: {
    int id = ctx->label_cnt++;
    emit(ctx, "LABEL $while_%d\n", id);
    gen_expr(ctx, s->data.while_loop.condition);
    emit(ctx, "PUSHS bool@false\nJUMPIFEQS $endwhile_%d\n", id);
    gen_block(ctx, s->data.while_loop.body);
    emit(ctx, "JUMP $while_%d\nLABEL $endwhile_%d\n", id, id);
    break;
  }

  default:
    if (is_expr(s)) {
      gen_expr(ctx, s);
      emit(ctx, "POPS GF@_t1\n");
    } else {
      ctx->failed = 1;
    }
    break;
  }
}

/**
 * @brief Vygeneruje všechny příkazy bloku a ukončí platnost jeho proměnných
 *
 * @param ctx: kontext generátoru
 * @param b: uzel AST_BLOCK, může být NULL
 */
static void gen_block(GeneratorContext *ctx, ASTNode *b) {
  size_t mark = ctx->vars_n; /* konec bloku = konec platnosti jeho proměnných */

  if (b) {
    for (ASTNode *s = b->data.block.statements; s; s = s->next) {
      gen_stmt(ctx, s);
      if (ctx->failed) {
        break;
      }
    }
  }

  ctx->vars_n = mark;
}

/* ===== funkce ===== */

/**
 * @brief Vygeneruje definici jedné funkce
 *
 * Postup: návěští $fn_<jméno>, PUSHFRAME, očíslování parametrů (0..k-1),
 * DEFVAR všech lokálních proměnných najednou, tělo funkce a nakonec
 * POPFRAME s RETURN. Hodnotu posledního výrazu těla funkce uloží do GF@_ret
 * jako návratovou hodnotu, pokud tělo nekončí výrazem, vrací se nil.
 *
 * @param ctx: kontext generátoru
 * @param fn: uzel AST_FUNC_DEF
 */
static void gen_function(GeneratorContext *ctx, ASTNode *fn) {
  ctx->cur_fn = fn;
  ctx->vars_n = 0;
  ctx->var_cnt = 0;

  emit(ctx, "LABEL $fn_%s\nPUSHFRAME\n", fn->data.func_def.name);

  for (ASTNode *p = fn->data.func_def.params; p; p = p->next) {
    declare(ctx, p->data.string_val); /* číslo parametru 0..k-1 */
  }

  /*
   * Všechny DEFVAR se generují najednou na začátku funkce.
   * Jinak by se deklarace uvnitř cyklu provedla při každé iteraci znovu.
   */
  int param_count = count_list(fn->data.func_def.params);
  int total_decls = param_count + count_decls(fn->data.func_def.body);
  for (int i = param_count; i < total_decls; i++) {
    emit(ctx, "DEFVAR LF@$v%d\n", i);
  }

  int returned = 0;
  size_t mark = ctx->vars_n;
  for (ASTNode *s = fn->data.func_def.body->data.block.statements;
       s; s = s->next) {
    if (!s->next && is_expr(s)) {
      /* Poslední výraz těla funkce určuje návratovou hodnotu. */
      gen_return_value(ctx, s);
      returned = 1;
    } else {
      gen_stmt(ctx, s);
    }

    if (ctx->failed) {
      break;
    }
  }

  ctx->vars_n = mark;
  if (!returned && !ctx->failed) {
    emit(ctx, "MOVE GF@_ret nil@nil\n");
  }
  emit(ctx, "POPFRAME\nRETURN\n\n");
}

/* ===== pomocné rutiny ===== */

/**
 * @brief Pomocné rutiny v IFJcode26, které se volají instrukcí CALL
 *
 * Argumenty dostávají na datovém zásobníku, výsledek na něm zanechávají.
 * Pracují s globálními proměnnými GF@_t1 až GF@_t6.
 *  - $$idiv: celočíselné dělení Int / Int zaokrouhlené k nule (funguje, ať
 *    IDIV zaokrouhluje k nule nebo k -inf),
 *  - $$readint, $$readdouble: čtení čísla, při neplatném vstupu nebo EOF
 *    vrací 0 (instrukce READ by uložila nil),
 *  - $$ord: ord(řetězec, index), mimo rozsah vrací -1,
 *  - $$substr: subStr(řetězec, začátek, konec), při špatných mezích
 *    ukončí program s kódem 40.
 */
static const char *HELPERS =
    /* Int / Int zaokrouhlené k nule. Zadání říká, že IDIV zaokrouhluje k -inf,
       ale ic26int dělí k nule. Výsledek se proto upravuje podle zbytku
       r = a - q*b: zbytek ve znaménku dělence znamená, že q je už správně. */
    "LABEL $$idiv\n"
    "POPS GF@_t2\nPOPS GF@_t1\n"
    "IDIV GF@_t3 GF@_t1 GF@_t2\n"
    "MUL GF@_t4 GF@_t3 GF@_t2\n"
    "SUB GF@_t4 GF@_t1 GF@_t4\n"
    "JUMPIFEQ $$idiv_ok GF@_t4 int@0\n"
    "LT GF@_t5 GF@_t4 int@0\nLT GF@_t6 GF@_t1 int@0\n"
    "JUMPIFEQ $$idiv_ok GF@_t5 GF@_t6\n"
    "ADD GF@_t3 GF@_t3 int@1\n"
    "LABEL $$idiv_ok\nPUSHS GF@_t3\nRETURN\n\n"
    /* ord(str, idx): mimo rozsah -> -1 */
    "LABEL $$ord\n"
    "POPS GF@_t2\nPOPS GF@_t1\nSTRLEN GF@_t4 GF@_t1\n"
    "LT GF@_t5 GF@_t2 int@0\nJUMPIFEQ $$ord_bad GF@_t5 bool@true\n"
    "LT GF@_t5 GF@_t2 GF@_t4\nJUMPIFEQ $$ord_bad GF@_t5 bool@false\n"
    "STRI2INT GF@_t3 GF@_t1 GF@_t2\nPUSHS GF@_t3\nRETURN\n"
    "LABEL $$ord_bad\nPUSHS int@-1\nRETURN\n\n"
    /* subStr(str, start, end): špatné meze -> EXIT 40 */
    "LABEL $$substr\n"
    "POPS GF@_t3\nPOPS GF@_t2\nPOPS GF@_t1\nSTRLEN GF@_t4 GF@_t1\n"
    "LT GF@_t5 GF@_t2 int@0\nJUMPIFEQ $$sub_err GF@_t5 bool@true\n"
    "LT GF@_t5 GF@_t3 int@0\nJUMPIFEQ $$sub_err GF@_t5 bool@true\n"
    "GT GF@_t5 GF@_t2 GF@_t3\nJUMPIFEQ $$sub_err GF@_t5 bool@true\n"
    "LT GF@_t5 GF@_t2 GF@_t4\nJUMPIFEQ $$sub_err GF@_t5 bool@false\n"
    "GT GF@_t5 GF@_t3 GF@_t4\nJUMPIFEQ $$sub_err GF@_t5 bool@true\n"
    "MOVE GF@_t6 string@\n"
    "LABEL $$sub_loop\n"
    "JUMPIFEQ $$sub_end GF@_t2 GF@_t3\n"
    "GETCHAR GF@_t5 GF@_t1 GF@_t2\nCONCAT GF@_t6 GF@_t6 GF@_t5\n"
    "ADD GF@_t2 GF@_t2 int@1\nJUMP $$sub_loop\n"
    "LABEL $$sub_end\nPUSHS GF@_t6\nRETURN\n"
    "LABEL $$sub_err\nEXIT int@40\n\n"
    /* readInt: při chybném vstupu a EOF vrací READ nil, IFJ26 chce 0 */
    "LABEL $$readint\n"
    "READ GF@_t1 int\n"
    "TYPE GF@_t2 GF@_t1\n"
    "JUMPIFEQ $$readint_ok GF@_t2 string@int\n"
    "MOVE GF@_t1 int@0\n"
    "LABEL $$readint_ok\nPUSHS GF@_t1\nRETURN\n\n"
    /* readDouble: stejně jako readInt, náhradní hodnota je 0.0 */
    "LABEL $$readdouble\n"
    "READ GF@_t1 float\n"
    "TYPE GF@_t2 GF@_t1\n"
    "JUMPIFEQ $$readdouble_ok GF@_t2 string@float\n"
    "MOVE GF@_t1 float@0x0p+0\n"
    "LABEL $$readdouble_ok\nPUSHS GF@_t1\nRETURN\n\n";

/* ===== vstupní bod ===== */

/**
 * @brief Vygeneruje celý program v IFJcode26 z AST a vypíše ho do souboru
 *
 * Výstup má tuto strukturu: hlavička .IFJcode26, definice globálních
 * proměnných (GF@_ret, GF@_t1 až GF@_t6), volání funkce s anotací @main
 * a EXIT 0, pomocné rutiny a nakonec všechny funkce programu.
 * Kód se nejdřív celý složí v paměti a zapíše se až po úspěšném průchodu AST,
 * takže při chybě nevznikne žádný částečný výstup.
 *
 * @param root: kořen AST (AST_PROGRAM), funkce jsou v data.block.statements
 * @param outf: soubor, do kterého se kód zapíše (typicky stdout)
 * @return SUCCESS, UNDEF_FN_ERR pokud chybí funkce @main,
 *         COMPILER_ERR při interní chybě generátoru
 */
int generate_program(ASTNode *root, FILE *outf) {
  if (!root || !outf) {
    return COMPILER_ERR;
  }

  GeneratorContext ctx = {0};
  ctx.prog = root;
  ctx.failed = 0;
  ctx.label_cnt = 0;
  ctx.var_cnt = 0;

  sb_init(&ctx.out, 4096);

  const char *main_name = NULL;
  for (ASTNode *f = root->data.block.statements; f; f = f->next) {
    if (f->type == AST_FUNC_DEF && f->is_main) {
      main_name = f->data.func_def.name;
      break;
    }
  }

  if (!main_name) {
    sb_free(&ctx.out);
    free(ctx.vars);
    return UNDEF_FN_ERR;
  }

  emit(&ctx, ".IFJcode26\n");
  emit(&ctx,
       "DEFVAR GF@_ret\nDEFVAR GF@_t1\nDEFVAR GF@_t2\nDEFVAR GF@_t3\n"
       "DEFVAR GF@_t4\nDEFVAR GF@_t5\nDEFVAR GF@_t6\n");
  emit(&ctx, "CREATEFRAME\nCALL $fn_%s\nEXIT int@0\n\n", main_name);
  sb_append_str(&ctx.out, HELPERS);

  for (ASTNode *f = root->data.block.statements; f; f = f->next) {
    if (f->type == AST_FUNC_DEF) {
      gen_function(&ctx, f);
      if (ctx.failed) {
        break;
      }
    }
  }

  int rc = SUCCESS;
  if (ctx.failed) {
    rc = COMPILER_ERR;
  } else if (fwrite(ctx.out.data, 1, ctx.out.length, outf) != ctx.out.length) {
    rc = COMPILER_ERR;
  }

  sb_free(&ctx.out);
  free(ctx.vars);
  return rc;
}