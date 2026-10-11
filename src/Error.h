#ifndef _ERRORS_H_
#define _ERRORS_H_

typedef enum error_value {
  SUCCESS,      // No error (return value 0)
  LEX_ANAL_ERR, // Error in the program within lexical analysis (incorrect
                // structure of the current lexeme).
  SYN_ANAL_ERR, // Error in the program within syntactic analysis (incorrect
                // program syntax, missing constructs, etc.).
  UNDEF_FN_ERR, // Semantic error - use of an undefined function or variable.
  REDEF_FN_VAR_ERR, // Semantic error - redefinition of a function or variable.
  STATIC_SEM_ERR, // Static semantic error - unexpected number of arguments when
                  // calling a function or an incorrect type of built-in
                  // function parameter.
  STATIC_SEM_TYPE_ERR, // Static semantic error - type compatibility in
                       // arithmetic, string, and relational expressions.
  WRITE_ERR, // Semantic error - writing into unmodifiable variable or formal
             // function parameter.
  ERR_OTHERS = 10,   // Other semantic errors.
  COMPILER_ERR = 99, // Internal compiler error (e.g., memory allocation
                     // failure, not influenced by the input program).
} Error;

#endif
