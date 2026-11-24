/**
 * @file symtable.h
 *
 * Překladač jazyka IFJ26
 *
 * @brief Rozhraní tabulky symbolů nad zásobníkem AVL stromů.
 *
 * @author Alex Oľšavský (xolsava00)
 * @author Jaroslav Vančura (xvancuj00)
 */

#ifndef SYMTABLE_H
#define SYMTABLE_H

#include "Error.h"
#include "lexer.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define T_TYPE_BOOLEAN T_KW_BOOLEAN

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
  int f_list_count;
  int *f_list_sizes;
  BodyState body_state;
  bool is_main;
  size_t def_pos;
  int def_indent;
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

/**
 * @brief Inicializuje novou položku symbolu
 */
symtable_entry *symtable_entry_init(void);

/**
 * @brief Uvolní položku symbolu
 *
 * @param entry: Položka tabulky symbolů
 */
void symtable_entry_dispose(symtable_entry *entry);

/**
 * @brief Vrátí větší ze dvou celých čísel
 *
 * @param a: První číslo
 * @param b: Druhé číslo
 */
int maxInt(int a, int b);

/**
 * @brief Vrátí výšku AVL podstromu
 *
 * @param tree: Kořen podstromu
 */
int avl_height(avl_tree *tree);

/**
 * @brief Vrátí faktor vyvážení uzlu AVL stromu
 *
 * @param tree: Uzel AVL stromu
 */
int node_balance(avl_tree *tree);

/**
 * @brief Inicializuje ukazatel na AVL strom
 *
 * @param tree: Ukazatel na kořen stromu
 */
void avl_init(struct avl_tree **tree);

/**
 * @brief Vyhledá záznam v AVL stromu podle klíče
 *
 * @param tree: Kořen AVL stromu
 * @param key: Hledaný klíč
 * @param data: Výstupní ukazatel na položku
 */
bool avl_search(struct avl_tree *tree, char *key, symtable_entry **data);

/**
 * @brief Vloží položku do AVL stromu a vyváží ho
 *
 * @param tree: Ukazatel na kořen stromu
 * @param key: Klíč symbolu
 * @param data: Vkládaná položka
 */
void avl_insert(struct avl_tree **tree, char *key, symtable_entry *data);

/**
 * @brief Uvolní všechny uzly a data AVL stromu
 *
 * @param tree: Ukazatel na kořen stromu
 */
void avl_dispose(struct avl_tree **tree);

/**
 * @brief Provede pravou rotaci AVL stromu
 *
 * @param tree: Kořen rotovaného podstromu
 */
avl_tree *rotate_right(avl_tree *tree);

/**
 * @brief Provede levou rotaci AVL stromu
 *
 * @param tree: Kořen rotovaného podstromu
 */
avl_tree *rotate_left(avl_tree *tree);

/**
 * @brief Vyváží uzel AVL stromu
 *
 * @param tree: Uzel k vyvážení
 */
avl_tree *balance(avl_tree *tree);

/**
 * @brief Inicializuje tabulku symbolů
 *
 * @param smt: Tabulka symbolů
 */
void smt_init(symtable *smt);

/**
 * @brief Přidá nový vnořený rozsah
 *
 * @param smt: Tabulka symbolů
 */
void smt_scope_add(symtable *smt);

/**
 * @brief Odstraní aktuální rozsah
 *
 * @param smt: Tabulka symbolů
 */
void smt_scope_remove(symtable *smt);

/**
 * @brief Vyhledá symbol od aktuálního rozsahu směrem ke globálnímu
 *
 * @param smt: Tabulka symbolů
 * @param key: Název symbolu
 * @param entry: Výstupní položka symbolu
 */
bool smt_search(symtable *smt, char *key, symtable_entry **entry);

/**
 * @brief Vyhledá symbol pouze v globálním rozsahu
 *
 * @param smt: Tabulka symbolů
 * @param key: Název symbolu
 * @param entry: Výstupní položka symbolu
 */
bool smt_search_global(symtable *smt, char *key, symtable_entry **entry);

/**
 * @brief Vloží symbol do aktuálního rozsahu
 *
 * @param smt: Tabulka symbolů
 * @param key: Název symbolu
 * @param entry: Vkládaná položka
 */
void smt_insert(symtable *smt, char *key, symtable_entry *entry);

/**
 * @brief Vloží symbol do globálního rozsahu
 *
 * @param smt: Tabulka symbolů
 * @param key: Název symbolu
 * @param entry: Vkládaná položka
 */
void smt_insert_global(symtable *smt, char *key, symtable_entry *entry);

/**
 * @brief Uvolní všechny rozsahy a tabulku symbolů
 *
 * @param smt: Tabulka symbolů
 */
void smt_dispose(symtable *smt);

/**
 * @brief Vloží vestavěné funkce jazyka do globálního rozsahu
 *
 * @param smt: Tabulka symbolů
 */
void smt_insert_builtin_funcs(symtable *smt);

#endif
