#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "Error.h"
#include "lexer.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct f_param {
  TokenType type;
  char *name;
} f_param;

typedef struct SourceMark {
  size_t pos;
  Token tok;
  int line_indent;
  bool at_bol;
} SourceMark;

typedef enum BodyState {
  BODY_NONE = 0,
  BODY_PENDING,
  BODY_IN_PROGRESS,
  BODY_DONE,
} BodyState;

typedef struct symtable_entry {
  TokenType type;
  bool declared;
  bool defined;
  bool function;
  bool variable;
  bool value;
  int f_param_count;
  f_param *f_params;
  BodyState body_state;
  bool is_main;
  size_t def_pos;
  SourceMark body_start;
  SourceMark body_end;
} symtable_entry;

typedef struct avl_tree {
  int height;
  symtable_entry *data;
  char *key;
  struct avl_tree *left;
  struct avl_tree *right;
} avl_tree;

typedef struct symtable {
  avl_tree **table;
  int size;
  int top;
} symtable;

symtable_entry *symtable_entry_init(void);
void symtable_entry_dispose(symtable_entry *entry);

int maxInt(int a, int b);
int avl_height(avl_tree *tree);
int node_balance(avl_tree *tree);

void avl_init(struct avl_tree **tree);
bool avl_search(struct avl_tree *tree, char *key, symtable_entry **data);
void avl_insert(struct avl_tree **tree, char *key, symtable_entry *data);
void avl_dispose(struct avl_tree **tree);
avl_tree *rotate_right(avl_tree *tree);
avl_tree *rotate_left(avl_tree *tree);
avl_tree *balance(avl_tree *tree);

void smt_init(symtable *smt);
void smt_scope_add(symtable *smt);
void smt_scope_remove(symtable *smt);
bool smt_search(symtable *smt, char *key, symtable_entry **entry);
bool smt_search_global(symtable *smt, char *key, symtable_entry **entry);
void smt_insert(symtable *smt, char *key, symtable_entry *entry);
void smt_insert_global(symtable *smt, char *key, symtable_entry *entry);
void smt_dispose(symtable *smt);
void smt_insert_builtin_funcs(symtable *smt);

#endif