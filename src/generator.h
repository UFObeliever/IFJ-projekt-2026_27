#ifndef GENERATOR_H
#define GENERATOR_H

#include "ast.h"
#include <stdio.h>

/**
 * Vygeneruje IFJcode26 z AST (kořen = AST_PROGRAM, funkce v
 * data.block.statements) a vypíše ho do `out`.
 * Při chybě se nevypíše nic. Vrací SUCCESS / UNDEF_FN_ERR / COMPILER_ERR.
 */
int generate_program(ASTNode *root, FILE *out);

#endif