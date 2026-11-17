/**
 * @file generator.c
 * @brief Generátor IFJcode26 z abstraktního syntaktického stromu
 *
 * Prochází AST a vytváří cílový kód. Výrazy počítá zásobníkově,
 * proměnné čísluje podle deklarací a návratové hodnoty předává přes GF@_ret.
 * Vestavěné funkce generuje přímo nebo pomocí pomocných rutin.
 * Výstup zapíše až po úspěšném zpracování celého AST.
 */

#include "generator.h"
#include "DynBuff.h"
#include "Error.h"
#include "lexer.h"
#include "precedence.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief Mapování jména proměnné na její číslo v kódu
 *
 * Číslování odlišuje i stejně pojmenované proměnné z různých bloků.
 */
typedef struct {
  const char *name; /**< jméno proměnné z AST (nekopíruje se) */
  int id;           /**< ID proměnné, použité v LF@$v<id> */
} VarRec;

/**
 * @brief Kontext generátoru
 *
 * Uchovává veškerý stav generování, takže není potřeba globální stav.
 */
typedef struct GeneratorContext {
  StringBuff out; /**< buffer výstupního kódu */

  int label_cnt; /**< čítač návěští */
  int var_cnt;   /**< ID další proměnné */
  int failed;    /**< příznak chyby generování */

  ASTNode *cur_fn; /**< právě generovaná funkce */
  ASTNode *prog;   /**< kořen AST programu */

  VarRec *vars;    /**< viditelné proměnné (jméno -> ID) */
  size_t vars_n;   /**< počet záznamů v poli vars */
  size_t vars_cap; /**< kapacita pole vars */
} GeneratorContext;

/**
 * @brief Naformátuje text a připojí ho do výstupního bufferu
 *
 * Velikost pomocného bufferu zjistí pomocí vsnprintf.
 * Při chybě nastaví ctx->failed.
 *
 * @param ctx: kontext generátoru
 * @param fmt: formátovací řetězec
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
 * @brief Zapíše řetězcový literál ve formátu IFJcode26
 *
 * Řídicí znaky, #, zpětné lomítko a znak 127 převede na escape sekvence \xyz.
 *
 * @param ctx: kontext generátoru
 * @param s: řetězec se skutečnými znaky
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
    if (*p <= 32 || *p == 35 || *p == 92 || *p == 127) {
      char escaped[8];
      snprintf(escaped, sizeof escaped, "\\%03u", (unsigned)*p);
      sb_append_str(&ctx->out, escaped);
    } else {
      sb_append_char(&ctx->out, (char)*p);
    }
  }
}

/**
 * @brief Zaregistruje proměnnou a přidělí jí volné ID
 *
 * Proměnná je viditelná do zkrácení zásobníku vars na konci bloku.
 *
 * @param ctx: kontext generátoru
 * @param name: jméno proměnné z AST
 * @return přidělené ID, při chybě 0 a ctx->failed
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
 * @brief Najde ID nejbližší viditelné proměnné daného jména
 *
 * Hledá od konce zásobníku, takže vnořená deklarace překryje nadřazenou.
 *
 * @param ctx: kontext generátoru
 * @param name: hledané jméno
 * @return ID proměnné, při nenalezení 0 a ctx->failed
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

  ctx->failed = 1; // měl by zachytit už parser
  return 0;
}

/**
 * @brief Převede hodnotu na vrcholu zásobníku z from na to
 *
 * Jedinou implicitní konverzí IFJ26 je Int na Double.
 *
 * @param ctx: kontext generátoru
 * @param from: původní typ (ExprType)
 * @param to: požadovaný typ (ExprType)
 */
static void convert(GeneratorContext *ctx, int from, int to) {
  if (from == EXPR_TYPE_INT && to == EXPR_TYPE_DOUBLE) {
    emit(ctx, "INT2FLOATS\n");
  }
}

/**
 * @brief Najde v AST uživatelskou funkci podle jména
 *
 * @param ctx: kontext generátoru
 * @param name: jméno hledané funkce
 * @return uzel AST_FUNC_DEF, nebo NULL pro vestavěnou funkci
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
 * @brief Spočítá uzly seznamu propojeného přes next
 *
 * @param p: první uzel seznamu, může být NULL
 * @return počet uzlů
 */
static int count_list(ASTNode *p) {
  int n = 0;
  for (; p; p = p->next) {
    n++;
  }
  return n;
}

/**
 * @brief Spočítá deklarace proměnných včetně vnořených bloků
 *
 * Prochází větve if a těla while; počet určuje potřebné instrukce DEFVAR.
 *
 * @param block: uzel AST_BLOCK, může být NULL
 * @return počet deklarací AST_VAR_DECL
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
 * @brief Ověří, zda je uzel výrazem
 *
 * @param s: uzel AST, může být NULL
 * @return 1 pro výraz, jinak 0
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

static void gen_expr(GeneratorContext *ctx, ASTNode *e);
static void gen_block(GeneratorContext *ctx, ASTNode *b);

/**
 * @brief Vygeneruje volání vestavěné funkce
 *
 * Argumenty vyhodnotí zleva doprava a podle funkce vypíše příslušné instrukce.
 * Výsledek zůstane na zásobníku; ord a subStr volají pomocné rutiny.
 *
 * @param ctx: kontext generátoru
 * @param c: uzel AST_FUNC_CALL vestavěné funkce
 */
static void gen_builtin(GeneratorContext *ctx, ASTNode *c) {
  const char *name = c->data.func_call.name;
  ASTNode *args = c->data.func_call.arguments;

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
  } else if (!strcmp(name, "toInt")) {
    emit(ctx, "FLOAT2INTS\n");
  } else if (!strcmp(name, "strLength")) {
    emit(ctx, "POPS GF@_t1\nSTRLEN GF@_t2 GF@_t1\nPUSHS GF@_t2\n");
  } else if (!strcmp(name, "chr")) {
    emit(ctx, "INT2CHARS\n");
  } else if (!strcmp(name, "ord")) {
    emit(ctx, "CALL $$ord\n");
  } else if (!strcmp(name, "subStr")) {
    emit(ctx, "CALL $$substr\n");
  } else {
    ctx->failed = 1;
  }
}

/**
 * @brief Vygeneruje volání uživatelské nebo vestavěné funkce
 *
 * U uživatelské funkce se argumenty vyhodnotí na zásobník, přesunou
 * odzadu do TF@$v<i> a po CALL se GF@_ret vloží zpět na zásobník.
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
   * Argumenty se vyhodnotí před vytvořením TF, aby vnořená volání
   * nepřepsala dočasný rámec.
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

  /* Vytvoří rámec a přesune do něj argumenty odzadu. */
  emit(ctx, "CREATEFRAME\n");
  for (int i = n - 1; i >= 0; i--) {
    emit(ctx, "DEFVAR TF@$v%d\nPOPS TF@$v%d\n", i, i);
  }
  emit(ctx, "CALL $fn_%s\nPUSHS GF@_ret\n", name);
}

/**
 * @brief Vygeneruje binární operátor
 *
 * Vyhodnotí operandy, případně převede Int na Double a vypíše operaci.
 * Řetězcové + používá CONCAT, celočíselné / rutinu $$idiv.
 * Operátory <= a >= skládá z porovnání a rovnosti kvůli správnému NaN.
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
    //x <= y je (x < y) OR (x == y)
    emit(ctx,
         "POPS GF@_t2\nPOPS GF@_t1\n"
         "LT GF@_t3 GF@_t1 GF@_t2\n"
         "EQ GF@_t4 GF@_t1 GF@_t2\n"
         "OR GF@_t3 GF@_t3 GF@_t4\n"
         "PUSHS GF@_t3\n");
  } else if (op == T_GTE) {
    //x >= y je (x > y) OR (x == y)
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
 * @brief Vygeneruje výraz a ponechá jeho hodnotu na zásobníku
 *
 * Unární mínus tvoří 0 - x, plus nemění hodnotu.
 * Literály Unit a Null se zapisují jako nil@nil.
 *
 * @param ctx: kontext generátoru
 * @param e: uzel výrazu
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

/**
 * @brief Uloží výraz do GF@_ret jako návratovou hodnotu
 *
 * Hodnotu převede na návratový typ. Funkce Unit výraz zahodí a vrací nil.
 * POPFRAME a RETURN doplňuje volající část generátoru.
 *
 * @param ctx: kontext generátoru
 * @param expr: vracený výraz, NULL pro prázdný return
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
 * Zpracuje deklaraci, přiřazení, return, if, while nebo samostatný výraz.
 * Po příkazu zůstane datový zásobník ve stejném stavu.
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
    int id = declare(ctx, s->data.var_assign.identifier);
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
 * @brief Vygeneruje příkazy bloku a ukončí platnost jeho proměnných
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

/**
 * @brief Vygeneruje definici funkce
 *
 * Vytvoří rámec, zaregistruje parametry, deklaruje lokální proměnné
 * a vygeneruje tělo. Poslední výraz určuje návratovou hodnotu,
 * jinak funkce vrací nil.
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
    declare(ctx, p->data.string_val);
  }

  /*
   * Všechny DEFVAR se vypíší na začátku funkce, aby se deklarace
   * uvnitř cyklu neopakovaly při každé iteraci.
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

/**
 * @brief Pomocné rutiny IFJcode26 volané instrukcí CALL
 *
 * Argumenty přijímají ze zásobníku a výsledek na něm ponechají.
 * Používají globální proměnné GF@_t1 až GF@_t6:
 *  - $$idiv: celočíselné dělení se zaokrouhlením k nule,
 *  - $$readint, $$readdouble: při chybném vstupu nebo EOF vrací 0,
 *  - $$ord: mimo rozsah vrací -1,
 *  - $$substr: při neplatných mezích ukončí program kódem 40.
 */
static const char *HELPERS =
    /* Int / Int se zaokrouhlením k nule. Výsledek IDIV upraví podle zbytku
       r = a - q*b, protože interprety mohou zaokrouhlovat odlišně. */
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
    "LABEL $$ord\n"
    "POPS GF@_t2\nPOPS GF@_t1\nSTRLEN GF@_t4 GF@_t1\n"
    "LT GF@_t5 GF@_t2 int@0\nJUMPIFEQ $$ord_bad GF@_t5 bool@true\n"
    "LT GF@_t5 GF@_t2 GF@_t4\nJUMPIFEQ $$ord_bad GF@_t5 bool@false\n"
    "STRI2INT GF@_t3 GF@_t1 GF@_t2\nPUSHS GF@_t3\nRETURN\n"
    "LABEL $$ord_bad\nPUSHS int@-1\nRETURN\n\n"
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
    "LABEL $$readint\n"
    "READ GF@_t1 int\n"
    "TYPE GF@_t2 GF@_t1\n"
    "JUMPIFEQ $$readint_ok GF@_t2 string@int\n"
    "MOVE GF@_t1 int@0\n"
    "LABEL $$readint_ok\nPUSHS GF@_t1\nRETURN\n\n"
    "LABEL $$readdouble\n"
    "READ GF@_t1 float\n"
    "TYPE GF@_t2 GF@_t1\n"
    "JUMPIFEQ $$readdouble_ok GF@_t2 string@float\n"
    "MOVE GF@_t1 float@0x0p+0\n"
    "LABEL $$readdouble_ok\nPUSHS GF@_t1\nRETURN\n\n";

/**
 * @brief Vygeneruje program IFJcode26 z AST a zapíše ho do souboru
 *
 * Výstup obsahuje hlavičku, globální proměnné, volání @main,
 * pomocné rutiny a definice funkcí. Kód se nejprve sestaví v bufferu
 * a zapíše až po úspěšném průchodu AST.
 *
 * @param root: kořen AST_PROGRAM
 * @param outf: výstupní soubor (typicky stdout)
 * @return SUCCESS, UNDEF_FN_ERR při chybějící @main, jinak COMPILER_ERR
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