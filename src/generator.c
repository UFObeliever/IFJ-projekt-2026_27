#include "generator.h"
#include "DynBuff.h"
#include "Error.h"
#include "lexer.h"
#include "precedence.h" /* ExprType (EXPR_TYPE_*) */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Zaznam pro mapovani jmena promenne na interni ID generatoru. */
typedef struct {
  const char *name;
  int id;
} VarRec;

/*
 * Veskery promenlivy stav generatoru je soustreden v teto strukture.
 * Generator diky tomu nepouziva globalni stav a jednotliva volani
 * generate_program() jsou na sobe nezavisla.
 */
typedef struct GeneratorContext {
  StringBuff out;

  int label_cnt;
  int var_cnt;
  int failed;

  ASTNode *cur_fn;
  ASTNode *prog;

  VarRec *vars;
  size_t vars_n;
  size_t vars_cap;
} GeneratorContext;

/* ===== emitovani ===== */

/*
 * Naformatuje text do dynamicky velkeho bufferu a prida ho do vystupu.
 * Na rozdil od puvodni verze neni delka jedne instrukce omezena na 1023 znaku.
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

static void emit_string_lit(GeneratorContext *ctx, const char *s) {
  if (!ctx || ctx->failed || !s) {
    if (ctx) {
      ctx->failed = 1;
    }
    return;
  }

  sb_append_str(&ctx->out, "string@");
  for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
    /* IFJcode26 vyzaduje escapovat ridici znaky, # a zpetne lomitko. */
    if (*p <= 32 || *p == 35 || *p == 92 || *p == 127) {
      char escaped[8];
      snprintf(escaped, sizeof escaped, "\\%03u", (unsigned)*p);
      sb_append_str(&ctx->out, escaped);
    } else {
      sb_append_char(&ctx->out, (char)*p);
    }
  }
}

/* ===== promenne: jmeno -> cislo (LF@$v<id>) ===== */
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

  ctx->failed = 1; /* parser/semanticka analyza mela chybu odchytit drive */
  return 0;
}

/* ===== pomocne ===== */
static void convert(GeneratorContext *ctx, int from, int to) {
  if (from == EXPR_TYPE_INT && to == EXPR_TYPE_DOUBLE) {
    emit(ctx, "INT2FLOATS\n");
  }
}

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

static int count_list(ASTNode *p) {
  int n = 0;
  for (; p; p = p->next) {
    n++;
  }
  return n;
}

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

/* ===== vyrazy: vysledek vzdy na datovem zasobniku ===== */
static void gen_expr(GeneratorContext *ctx, ASTNode *e);
static void gen_block(GeneratorContext *ctx, ASTNode *b);

static void gen_builtin(GeneratorContext *ctx, ASTNode *c) {
  const char *name = c->data.func_call.name;
  ASTNode *args = c->data.func_call.arguments;

  /* Argumenty vestavenych funkci se vyhodnocuji zleva doprava. */
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
    emit(ctx, "READ GF@_t1 int\nPUSHS GF@_t1\n");
  } else if (!strcmp(name, "readDouble")) {
    emit(ctx, "READ GF@_t1 float\nPUSHS GF@_t1\n");
  } else if (!strcmp(name, "toStr")) {
    if (args->expr_type == EXPR_TYPE_INT) {
      emit(ctx, "INT2STRS\n");
    } else if (args->expr_type == EXPR_TYPE_DOUBLE) {
      emit(ctx, "FLOAT2STRS\n");
    } else if (args->expr_type == EXPR_TYPE_UNIT) {
      emit(ctx, "POPS GF@_t1\nPUSHS string@()\n");
    }
    /* String: hodnota uz je na zasobniku. */
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

static void gen_call(GeneratorContext *ctx, ASTNode *c) {
  const char *name = c->data.func_call.name;
  ASTNode *fn = find_func(ctx, name);

  if (!fn) {
    gen_builtin(ctx, c);
    return;
  }

  /*
   * 1) Vsechny argumenty vyhodnotime pred vytvorenim TF.
   *    Vnorena volani tak nemohou prepsat docasny ramec volajici funkce.
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

  /* 2) Novy ramec; argumenty se vyzvednou ze zasobniku odzadu. */
  emit(ctx, "CREATEFRAME\n");
  for (int i = n - 1; i >= 0; i--) {
    emit(ctx, "DEFVAR TF@$v%d\nPOPS TF@$v%d\n", i, i);
  }
  emit(ctx, "CALL $fn_%s\nPUSHS GF@_ret\n", name);
}

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
     * NOT (x > y) neni ekvivalentni pro NaN: NOT(false) by dalo true.
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
     * I zde zustava vysledek pro NaN false.
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

/* ===== prikazy ===== */

/* Do GF@_ret ulozi hodnotu expr (po konverzi na navratovy typ funkce).
 * Ve funkci s navratovym typem Unit se hodnota zahodi a vraci se nil. */
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
    int id = declare(ctx, s->data.var_assign.identifier); /* az po inicializaci */
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

static void gen_block(GeneratorContext *ctx, ASTNode *b) {
  size_t mark = ctx->vars_n; /* konec bloku = konec platnosti jeho promennych */

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
static void gen_function(GeneratorContext *ctx, ASTNode *fn) {
  ctx->cur_fn = fn;
  ctx->vars_n = 0;
  ctx->var_cnt = 0;

  emit(ctx, "LABEL $fn_%s\nPUSHFRAME\n", fn->data.func_def.name);

  for (ASTNode *p = fn->data.func_def.params; p; p = p->next) {
    declare(ctx, p->data.string_val); /* ID parametru 0..k-1 */
  }

  /*
   * Vsechny DEFVAR se generuji najednou na zacatku funkce.
   * Jinak by se deklarace uvnitr cyklu provedla pri kazde iteraci znovu.
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
      /* Posledni vyraz tela funkce urcuje navratovou hodnotu. */
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

/* ===== pomocne rutiny ===== */
static const char *HELPERS =
    /* Int / Int zaokrouhlene k nule (IDIV zaokrouhluje k -inf) */
    "LABEL $$idiv\n"
    "POPS GF@_t2\nPOPS GF@_t1\n"
    "IDIV GF@_t3 GF@_t1 GF@_t2\n"
    "MUL GF@_t4 GF@_t3 GF@_t2\n"
    "JUMPIFEQ $$idiv_ok GF@_t4 GF@_t1\n"
    "LT GF@_t5 GF@_t1 int@0\nLT GF@_t6 GF@_t2 int@0\n"
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
    /* subStr(str, start, end): spatne meze -> EXIT 40 */
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
    "LABEL $$sub_err\nEXIT int@40\n\n";

/* ===== vstupni bod ===== */
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