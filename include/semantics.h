#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>
#include "ast.h"

#define SEMANTICS_STR_MAX_LEN                   1048
#define SEMANTICS_SCOPE_MAX_NUM                 100

#define SEMANTICS_NO_TYPE                       ""
#define SEMANTICS_TYPE_VOID                     "void"
#define SEMANTICS_TYPE_BOOL                     "bool"
#define SEMANTICS_TYPE_INT                      "int"
#define SEMANTICS_TYPE_STRING                   "string"

#define SEMANTICS_FUNCTION_NAME_SCOPE(fmt, ...) ({\
                                                    char *scope_name = strdup(fmt);\
                                                    sprintf(scope_name, fmt, ##__VA_ARGS__);\
                                                    scope_name;\
                                                })           

#define SEMANTICS_VAR_REDECLARE_ERR_FMT         "Redeclaration of '%s'"
#define SEMANTICS_VAR_UNDEFINED_ERR_FMT         "Undefine reference to '%s'"
#define SEMANTICS_TYPE_ERROR                    "expected type, got '%s'"
#define SEMANTICS_LOOP_ID_TYPE_ERR              "%s expression must be boolean type"
#define SEMANTICS_USE_TYPE_FOR_ASSIGNMENT_ERR   "Can't use type '%s' for assignment"
#define SEMANTICS_FUNC_CALL_NUM_ARGS_ERR        "number/type of arguments doesn't match function declaration"
#define SEMANTICS_CONSTANT_ASSIGNMENT_ERR       "can't assign to a constant"
#define SEMANTICS_NON_VAR_ASSIGMENT_ERR         "can only assign to a variable"
#define SEMANTICS_INT_OUT_OF_RANGE_ERR          "integer literal out of range"
#define SEMANTICS_INT_TOO_LARGE_ERR             "integer literal too large"
#define SEMANTICS_INT_TOO_SMALL_ERR             "integer literal too small"
#define SEMANTICS_BREAK_ERR                     "break must be inside 'for'"
#define SEMANTICS_NO_RETURN_STMT_FOUND          "no return statement in function"
#define SEMANTICS_RETURN_NO_VALUE_NON_VOID_FUNC "this function must return a value"
#define SEMANTICS_VOID_FUNC_RETURN_ERR          "this function can't return a value"
#define SEMANTICS_FUNC_CALL_NOT_FUNC_ERR        "can't call something that isn't a function"
#define SEMANTICS_FUNC_RETURN_ERR               "returned value has the wrong type"
#define SEMANTICS_NO_FUNC_RETURN_ERR            "return statement must be inside a function"
#define SEMANTICS_MAIN_MISSING                  "missing main() function"
#define SEMANTICS_MAIN_ARGS_NUM_ERROR           "main() can't have arguments"
#define SEMANTICS_MAIN_RETURN_ERR               "main() can't have a return value"
#define SEMANTICS_FUNC_CALL_ARGS_TYPE_ERR       SEMANTICS_FUNC_CALL_NUM_ARGS_ERR
#define SEMANTICS_OPERATOR_OPERAND_TYPE_ERR_FMT "operand type mismatch for '%s'"
#define SEMANTICS_ERROR(linenum, fmt, ...)      {\
                                                    char err[SEMANTICS_STR_MAX_LEN * 2];\
                                                    snprintf(err, SEMANTICS_STR_MAX_LEN * 2, fmt, ##__VA_ARGS__);\
                                                    if (linenum > 0) {\
                                                        fprintf(stderr, "error: %s at or near line %d\n", err, linenum);\
                                                    } else {\
                                                        fprintf(stderr, "error: %s\n", err);\
                                                    }\
                                                    exit(EXIT_FAILURE);\
                                                }

typedef struct node node_t;
typedef struct ast_start ast_start_t;
typedef enum ast_node_type_e ast_node_type_t;

typedef struct semantics_stab_record {
    char* symbol_name;
    char* sig;
    char* rv_sig;
    bool is_const;
    bool is_type;
} semantics_stab_record_t;

typedef struct semantics_legal_operators {
    const char* op;
    const char* operands[3][3];
} semantics_legal_operators_t;

typedef struct semantics_scope {
    const char* scope_name;
    node_t* contents;
} semantics_scope_t;

void semantics_init();
bool semantics_node_type_is_operator(ast_node_type_t node_type);
semantics_stab_record_t* semantics_define(semantics_stab_record_t* record, int linenum);
semantics_stab_record_t* semantics_lookup(ast_node_t* symbol);
void semantics_open_scope(const char* scope_name);
void semantics_close_scope();
void semantics_global_decl_check(ast_start_t* ast_annotated, int ast_annotated_node_count);
void semantics_decl_check(ast_start_t* ast_annotated, int ast_annotated_node_count);
void semantics_type_check(ast_start_t* ast_annotated, int ast_annotated_node_count);
void semantics_miscellaneous_check(ast_start_t* ast_annotated, int ast_annotated_node_count);
void semantics_record_print(semantics_stab_record_t* record);
void semantics_stab_print();