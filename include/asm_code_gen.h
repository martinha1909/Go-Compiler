#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <stdbool.h>
#include "ast.h"

typedef enum ast_node_type_e ast_node_type_t;

typedef struct asm_tmp_reg {
    char* reg;
    bool in_use;
} asm_tmp_reg_t;

typedef struct asm_func_arg {
    ast_node_type_t* arg_type;
    char* symbol;
} asm_func_arg_t;

void asm_code_gen(ast_node_t** nodes, int num_children);
asm_tmp_reg_t* asm_register_alloc(const char* reg_name);