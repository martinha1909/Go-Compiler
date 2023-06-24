#pragma once

#include <string.h>
#include "token.h"
#include "semantics.h"

#define AST_NODE_MAX_NUM                2048
#define AST_NODE_NAME_MAX_LEN           TOKEN_LEXEME_MAX_LEN

#define AST_FUNC_SIG_MAX_CHILDREN       2
#define AST_MATH_EXPR_MAX_CHILDREN      2

typedef enum ast_node_type_e {
    AST_NODE_NONE = 0,
    AST_NODE_START,
    AST_NODE_ID,
    /* literal node group*/
    AST_NODE_NUM,
    AST_NODE_STRING,
    /* function node group */
    AST_NODE_FUNC,
    AST_NODE_FUNC_ID,
    AST_NODE_FUNC_SIG,
    AST_NODE_FUNC_SIG_FORMAL,
    AST_NODE_FUNC_SIG_FORMAL_CHILD,
    AST_NODE_FUNC_SIG_FORMAL_TYPE_ID,
    AST_NODE_FUNC_SIG_FORMAL_NEW_ID,
    AST_NODE_FUNC_CALL,
    AST_NODE_FUNC_CALL_ACTUAL,
    /* block node group*/
    AST_NODE_BLOCK,
    AST_NODE_BLOCK_EMPTY_STMT,
    AST_NODE_BLOCK_EXPRESSION_STMT,
    /* loop node group*/
    AST_NODE_FOR_LOOP,
    AST_NODE_FOR_LOOP_ID,
    AST_NODE_IF_LOOP,
    AST_NODE_IF_ELSE_LOOP,
    /* binary operator node group */
    AST_NODE_BINARY_OP_ADD,
    AST_NODE_BINARY_OP_SUB,
    AST_NODE_BINARY_OP_MULT,
    AST_NODE_BINARY_OP_DIV,
    AST_NODE_BINARY_OP_MOD,
    AST_NODE_BINARY_OP_NEGATE,
    AST_NODE_BINARY_OP_ASSIGNMENT,
    AST_NODE_BINARY_OP_EQ_CMP,
    AST_NODE_BINARY_OP_NOT_EQ_CMP,
    AST_NODE_BINARY_OP_LT,
    AST_NODE_BINARY_OP_LE,
    AST_NODE_BINARY_OP_GT,
    AST_NODE_BINARY_OP_GE,
    AST_NODE_BINARY_OR_CMP,
    AST_NODE_BINARY_AND_CMP,
    /* unary operator node group */
    AST_NODE_UNARY_OP_NEGATE,
    AST_NODE_UNARY_OP_LOGICAL_NEGATE,
    /* variable declaration node group */
    AST_NODE_VAR_DECL,
    AST_NODE_VAR_GLOBAL_DECL,
    AST_NODE_VAR_NEW_ID,
    AST_NODE_VAR_TYPE_ID,
    /* return keyword */
    AST_NODE_RETURN,
    /* break keyword */
    AST_NODE_BREAK
} ast_node_type_t;

typedef struct semantics_stab_record semantics_stab_record_t;

typedef struct ast_parse_info {
    char* lexeme;
    int linenum;
    int token_count;
} ast_parse_info_t;

typedef struct ast_node {
    ast_parse_info_t* node_info;
    semantics_stab_record_t* symbol_loc;
    ast_node_type_t type;
    int num_children;
    struct ast_node **children;
} ast_node_t;

typedef struct ast_start {
    ast_node_type_t type;
    semantics_stab_record_t* symbol_loc;
    char node_name[AST_NODE_NAME_MAX_LEN];
    ast_node_t** nodes;
} ast_start_t;

int ast_init();
void ast_prune(ast_node_t** nodes, int num_children);
char* ast_str_node_type(ast_node_type_t node_type);
void ast_node_collection_append(ast_node_t* node);
void ast_repete_rules_nodes_append(ast_node_t* node);
void ast_assembled_nodes_append(ast_node_t* node);
void ast_add_node(ast_node_t* node);
ast_node_t* ast_node_alloc(ast_parse_info_t* node_info,
                           ast_node_type_t node_type,
                           int num_children,
                           ast_node_t** children);
ast_parse_info_t* ast_node_info_alloc(char* lexeme, 
                                      const int linenum, 
                                      const int token_count);
ast_node_t* ast_node_alloc_from_collection();
void ast_record_block_num_children();
void ast_record_func_call_actuals();
void ast_block_num_children_increment();
void ast_func_call_actual_increment();
ast_node_t* ast_node_alloc_from_assembled_nodes();
ast_node_t* ast_repete_rules_nodes_alloc_single(int index);
void ast_func_formals_children_alloc(ast_node_t*** children_node, int* num_children);
ast_node_t* ast_func_call_node_alloc();
void ast_node_separate_multiple_unary_op_if_any(ast_node_t* node);
ast_node_t* ast_node_collection_pop();
ast_node_t* ast_assembled_nodes_pop();
int ast_blocks_num_children_pop();
int ast_func_call_actuals_pop();
ast_node_t* ast_repete_rules_nodes_pop();
int ast_repete_rules_nodes_size();
int ast_node_collection_size();
int ast_blocks_num_children_size();
int ast_func_call_actuals_size();
void ast_in_block_check();
void ast_in_func_call_actual_check();
bool ast_repete_rules_nodes_empty();
bool ast_assembled_nodes_empty();
void ast_node_collection_reset();
void ast_repete_rules_nodes_reset();
void ast_assembled_nodes_reset();
void ast_blocks_num_children_reset();
ast_node_t** ast_children_alloc(ast_parse_info_t** children_info,
                                ast_node_type_t* children_types,
                                int num_children_alloc);
void ast_children_append(ast_node_t*** children, int* current_children_size, ast_node_t* item);
void ast_children_push_front(ast_node_t*** children, 
                             int* current_children_size, 
                             ast_node_t* item);
ast_start_t* ast_get();
int ast_node_count_get();
void ast_print();
void ast_print_specific(ast_start_t* ast_start, int node_count);
void ast_node_collection_print();
void ast_repete_rules_nodes_print();
void ast_assembled_nodes_print();
void ast_node_print(ast_node_t* node);
void ast_deinit();