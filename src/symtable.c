#include "symtable.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

symtable_entry *symtable_entry_init(void) {
  symtable_entry *entry = calloc(1, sizeof(symtable_entry));
  if (entry == NULL) {
    fprintf(stderr, "Symtable entry memory allocation failed.\n");
    exit(COMPILER_ERR);
  }
  entry->function = false;
  entry->defined = false;
  entry->declared = false;
  entry->f_param_count = 0;
  entry->f_params = NULL;
  return entry;
}

void symtable_entry_dispose(symtable_entry *entry) {
  if (entry != NULL) {
    if (entry->function && entry->f_params != NULL) {
      free(entry->f_params);
    }
    free(entry);
  }
}

int max(int a, int b) { return (a > b) ? a : b; }

int avl_height(avl_tree *tree) {
  if (tree == NULL) {
    return 0;
  }
  return tree->height;
}

int node_balance(avl_tree *tree) {
  if (tree == NULL) {
    return 0;
  }
  return avl_height(tree->left) - avl_height(tree->right);
}

void avl_init(struct avl_tree **tree) { *tree = NULL; }

bool avl_search(struct avl_tree *tree, char *key, symtable_entry **data) {
  if (tree == NULL) {
    return false;
  }
  int cmp = strcmp(key, tree->key);
  if (cmp == 0) {
    *data = tree->data;
    return true;
  } else if (cmp < 0) {
    return avl_search(tree->left, key, data);
  } else {
    return avl_search(tree->right, key, data);
  }
}

void avl_insert(struct avl_tree **tree, char *key, symtable_entry *data) {
  if (*tree == NULL) {
    *tree = malloc(sizeof(avl_tree));
    if (*tree == NULL) {
      fprintf(stderr, "Memory allocation error.\n");
      exit(COMPILER_ERR);
    }
    (*tree)->key = strdup(key);
    (*tree)->data = data;
    (*tree)->left = NULL;
    (*tree)->right = NULL;
    (*tree)->height = 1;
    return;
  }
  int cmp = strcmp(key, (*tree)->key);

  if (cmp == 0) {
    (*tree)->data = data;
    return;
  } else if (cmp < 0) {
    avl_insert(&(*tree)->left, key, data);
  } else {
    avl_insert(&(*tree)->right, key, data);
  }
  (*tree)->height =
      1 + max(avl_height((*tree)->left), avl_height((*tree)->right));
  *tree = balance(*tree);
}

void avl_dispose(struct avl_tree **tree) {
  if (*tree == NULL) {
    return;
  }
  avl_dispose(&(*tree)->left);
  avl_dispose(&(*tree)->right);
  symtable_entry_dispose((*tree)->data);
  free((*tree)->key);
  free(*tree);
  *tree = NULL;
}

avl_tree *rotate_right(avl_tree *tree) {
  avl_tree *tmp = tree->left;
  tree->left = tmp->right;
  tmp->right = tree;
  tree->height = 1 + max(avl_height(tree->left), avl_height(tree->right));
  tmp->height = 1 + max(avl_height(tmp->left), avl_height(tmp->right));
  return tmp;
}

avl_tree *rotate_left(avl_tree *tree) {
  avl_tree *tmp = tree->right;
  tree->right = tmp->left;
  tmp->left = tree;
  tree->height = 1 + max(avl_height(tree->left), avl_height(tree->right));
  tmp->height = 1 + max(avl_height(tmp->left), avl_height(tmp->right));
  return tmp;
}

avl_tree *balance(avl_tree *tree) {
  int balance_factor = node_balance(tree);
  if (balance_factor > 1) {
    if (node_balance(tree->left) < 0) {
      tree->left = rotate_left(tree->left);
    }
    return rotate_right(tree);
  }
  if (balance_factor < -1) {
    if (node_balance(tree->right) > 0) {
      tree->right = rotate_right(tree->right);
    }
    return rotate_left(tree);
  }
  return tree;
}

void smt_init(symtable *smt) {
  if (smt != NULL) {
    smt->size = 256;
    smt->table = malloc(sizeof(avl_tree *) * smt->size);
    if (smt->table == NULL) {
      fprintf(stderr, "Memory allocation error.\n");
      exit(COMPILER_ERR);
    }
    smt->top = -1;
    smt_scope_add(smt);
  }
}

void smt_scope_add(symtable *smt) {
  if (smt != NULL) {
    if (smt->top + 1 >= smt->size) {
      fprintf(stderr, "Symbol table scope adding error.\n");
      exit(COMPILER_ERR);
    }
    smt->top++;
    avl_init(&smt->table[smt->top]);
  }
}

void smt_scope_remove(symtable *smt) {
  if (smt != NULL) {
    if (smt->top < 0) {
      fprintf(stderr, "Symbol table scope removal error.\n");
      exit(COMPILER_ERR);
    }
    avl_dispose(&smt->table[smt->top]);
    smt->top--;
  }
}

bool smt_search(symtable *smt, char *key, symtable_entry **entry) {
  if (smt != NULL && entry != NULL) {
    int idx = smt->top;
    while (idx >= 0) {
      if (avl_search(smt->table[idx], key, entry)) {
        return true;
      }
      idx--;
    }
  }
  return false;
}

bool smt_search_global(symtable *smt, char *key, symtable_entry **entry) {
  if (smt != NULL && entry != NULL) {
    if (avl_search(smt->table[0], key, entry)) {
      return true;
    }
  }
  return false;
}

void smt_insert(symtable *smt, char *key, symtable_entry *entry) {
  if (smt != NULL && key != NULL && entry != NULL) {
    if (smt->top < 0) {
      fprintf(stderr, "Symtable insertion error.\n");
      exit(COMPILER_ERR);
    }
    symtable_entry *temp = NULL;
    if (avl_search(smt->table[smt->top], key, &temp)) {
      fprintf(stderr, "Redefinition error\n");
      exit(REDEF_FN_VAR_ERR);
    }
    avl_insert(&smt->table[smt->top], key, entry);
  }
}

void smt_insert_global(symtable *smt, char *key, symtable_entry *entry) {
  if (smt == NULL || key == NULL || entry == NULL) {
    return;
  }

  symtable_entry *head = NULL;

  if (avl_search(smt->table[0], key, &head)) {
    fprintf(stderr, "Redefinition error: global symbol '%s' already exists\n",
            key);
    exit(REDEF_FN_VAR_ERR);
  }

  avl_insert(&smt->table[0], key, entry);
}

void smt_dispose(symtable *smt) {
  if (smt != NULL) {
    for (int i = smt->top; i >= 0; i--) {
      avl_dispose(&smt->table[i]);
    }
    free(smt->table);
    smt->top = -1;
    smt->size = 0;
  }
}
