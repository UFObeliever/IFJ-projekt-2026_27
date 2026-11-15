#ifndef GENERATOR_H
#define GENERATOR_H

#include "ast.h"
#include <stdio.h>

/**
 * @brief Vygeneruje program v IFJcode26 z AST a vypíše ho do souboru
 *
 * Kód se nejdřív celý složí v paměti a zapíše se až po úspěšném průchodu AST,
 * takže při chybě nevznikne žádný částečný výstup.
 *
 * @param root: kořen AST (AST_PROGRAM), definice funkcí jsou v
 * data.block.statements
 * @param out: soubor, do kterého se kód zapíše (stdout)
 * @return SUCCESS, UNDEF_FN_ERR pokud chybí funkce s anotací @main,
 *         COMPILER_ERR při interní chybě generátoru
 */
int generate_program(ASTNode *root, FILE *out);

#endif
