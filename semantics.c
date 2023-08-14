#include <string.h>
#include "include/semantics.h"
#include "include/linked_list.h"
#include "include/stack.h"

static semantics_stab_record_t UNIVERSE_SCOPE_SEMANTICS[] = {
    /* sname,   sig,                         rv_sig,                is_const, is_type */
    {"$void",   SEMANTICS_TYPE_VOID,          SEMANTICS_NO_TYPE,     false, true},
    {"bool",    SEMANTICS_TYPE_BOOL,          SEMANTICS_NO_TYPE,     false, true},
    {"int",     SEMANTICS_TYPE_INT,           SEMANTICS_NO_TYPE,     false, true},
    {"string",  SEMANTICS_TYPE_STRING,        SEMANTICS_NO_TYPE,     false, true},
    {"$true",   SEMANTICS_TYPE_BOOL,          SEMANTICS_NO_TYPE,     true,  false},
    {"true",    SEMANTICS_TYPE_BOOL,          SEMANTICS_NO_TYPE,     true,  false},
    {"false",   SEMANTICS_TYPE_BOOL,          SEMANTICS_NO_TYPE,     true,  false},
    {"printb",  "f("SEMANTICS_TYPE_BOOL")",   SEMANTICS_TYPE_VOID,   false, false},
    {"printc",  "f("SEMANTICS_TYPE_INT")",    SEMANTICS_TYPE_VOID,   false, false},
    {"printi",  "f("SEMANTICS_TYPE_INT")",    SEMANTICS_TYPE_VOID,   false, false},
    {"prints",  "f("SEMANTICS_TYPE_STRING")", SEMANTICS_TYPE_VOID,   false, false},
    {"getchar", "f()",                        SEMANTICS_TYPE_INT,    false, false},
    {"halt",    "f()",                        SEMANTICS_TYPE_VOID,   false, false},
    {"len",     "f("SEMANTICS_TYPE_STRING")", SEMANTICS_TYPE_INT,    false, false}
};

static const semantics_legal_operators_t LEGAL_OPERATOR_OPERANDS[] = {
    {
        "||",  
        {
            /* right operand,       left operand,           return value */
            { SEMANTICS_TYPE_BOOL, SEMANTICS_TYPE_BOOL, SEMANTICS_TYPE_BOOL },
            { NULL, NULL, NULL },
            { NULL, NULL, NULL }
        }
    },
    {   
        "&&", 
        {
            { SEMANTICS_TYPE_BOOL, SEMANTICS_TYPE_BOOL, SEMANTICS_TYPE_BOOL },
            { NULL, NULL, NULL },
            { NULL, NULL, NULL }
        }
    },
    {   
        "==", 
        {
            { SEMANTICS_TYPE_BOOL, SEMANTICS_TYPE_BOOL, SEMANTICS_TYPE_BOOL }, 
            { SEMANTICS_TYPE_INT, SEMANTICS_TYPE_INT, SEMANTICS_TYPE_BOOL },
            { SEMANTICS_TYPE_STRING, SEMANTICS_TYPE_STRING, SEMANTICS_TYPE_BOOL }
        }
    },
    {   
        "!=", 
        {
            { SEMANTICS_TYPE_BOOL, SEMANTICS_TYPE_BOOL, SEMANTICS_TYPE_BOOL }, 
            { SEMANTICS_TYPE_INT, SEMANTICS_TYPE_INT, SEMANTICS_TYPE_BOOL },
            { SEMANTICS_TYPE_STRING, SEMANTICS_TYPE_STRING, SEMANTICS_TYPE_BOOL }
        }
    },
    {   
        "=", 
        {
            { SEMANTICS_TYPE_BOOL, SEMANTICS_TYPE_BOOL, SEMANTICS_TYPE_BOOL }, 
            { SEMANTICS_TYPE_INT, SEMANTICS_TYPE_INT, SEMANTICS_TYPE_INT },
            { SEMANTICS_TYPE_STRING, SEMANTICS_TYPE_STRING, SEMANTICS_TYPE_STRING }
        }
    },
    {   
        "<", 
        {
            { SEMANTICS_TYPE_INT, SEMANTICS_TYPE_INT, SEMANTICS_TYPE_BOOL },
            { SEMANTICS_TYPE_STRING, SEMANTICS_TYPE_STRING, SEMANTICS_TYPE_BOOL },
            { NULL, NULL, NULL }
        }
    },
    {   
        ">", 
        {
            { SEMANTICS_TYPE_INT, SEMANTICS_TYPE_INT, SEMANTICS_TYPE_BOOL },
            { SEMANTICS_TYPE_STRING, SEMANTICS_TYPE_STRING, SEMANTICS_TYPE_BOOL },
            { NULL, NULL, NULL }
        }
    },
    {   
        "<=", 
        {
            { SEMANTICS_TYPE_INT, SEMANTICS_TYPE_INT, SEMANTICS_TYPE_BOOL },
            { SEMANTICS_TYPE_STRING, SEMANTICS_TYPE_STRING, SEMANTICS_TYPE_BOOL },
            { NULL, NULL, NULL }
        }
    },
    {   
        ">=", 
        {
            { SEMANTICS_TYPE_INT, SEMANTICS_TYPE_INT, SEMANTICS_TYPE_BOOL },
            { SEMANTICS_TYPE_STRING, SEMANTICS_TYPE_STRING, SEMANTICS_TYPE_BOOL },
            { NULL, NULL, NULL }
        }
    },
    {   
        "+", 
        {
            { SEMANTICS_TYPE_INT, SEMANTICS_TYPE_INT, SEMANTICS_TYPE_INT },
            { NULL, NULL, NULL },
            { NULL, NULL, NULL }
        }
    },
    {   
        "*", 
        {
            { SEMANTICS_TYPE_INT, SEMANTICS_TYPE_INT, SEMANTICS_TYPE_INT },
            { NULL, NULL, NULL },
            { NULL, NULL, NULL }
        }
    },
    {   
        "/", 
        {
            { SEMANTICS_TYPE_INT, SEMANTICS_TYPE_INT, SEMANTICS_TYPE_INT },
            { NULL, NULL, NULL },
            { NULL, NULL, NULL }
        }
    },
    {   
        "%", 
        {
            { SEMANTICS_TYPE_INT, SEMANTICS_TYPE_INT, SEMANTICS_TYPE_INT },
            { NULL, NULL, NULL },
            { NULL, NULL, NULL }
        }
    },
    {   
        "!", 
        {
            /* left operand, right operand         return type */
            { NULL, SEMANTICS_TYPE_BOOL, SEMANTICS_TYPE_BOOL},
            { NULL, NULL, NULL },
            { NULL, NULL, NULL }
        }
    },
    {   
        "u!", 
        {
            /* left operand, right operand         return type */
            { NULL, SEMANTICS_TYPE_BOOL, SEMANTICS_TYPE_BOOL},
            { NULL, NULL, NULL },
            { NULL, NULL, NULL }
        }
    },
    {   
        "-", 
        {
            { SEMANTICS_TYPE_INT, SEMANTICS_TYPE_INT, SEMANTICS_TYPE_INT },
            { NULL, NULL, NULL },
            { NULL, NULL, NULL }
        }
    },
    {   
        "u-", 
        {
            { NULL, SEMANTICS_TYPE_INT, SEMANTICS_TYPE_INT },
            { NULL, NULL, NULL },
            { NULL, NULL, NULL }
        }
    }
};

static semantics_stab_record_t* _return_func_symbol_from_scope(int curr_linenum)
{
    ast_parse_info_t curr_func_info = {};
    ast_node_t* curr_func_node = NULL;

    curr_func_info.lexeme = (char*)stack_peek()->scope_name;
    /* return statement outside of any function */
    if (strcmp(curr_func_info.lexeme, "File") == 0) {
        SEMANTICS_ERROR(curr_linenum, SEMANTICS_NO_FUNC_RETURN_ERR);
    }

    /* just a temporary node for symbol lookup, no need to have proper information, just need the function id */
    curr_func_node = ast_node_alloc(&curr_func_info,
                                    AST_NODE_FUNC,
                                    0,
                                    NULL);
    return semantics_lookup(curr_func_node);
}

static bool _found_return_stmt(ast_node_t** func_children, int func_num_children)
{
    int i;
    bool ret = false;

    if (func_children == NULL || func_num_children == 0) {
        return ret;
    }

    for (i = 0; i < func_num_children; i++) {
        if (func_children[i]->type == AST_NODE_RETURN) {
            return true;
        }

        ret = _found_return_stmt(func_children[i]->children, func_children[i]->num_children);
    }

    return ret;
}

static const char* _operands_type_check(ast_node_t* operator_node)
{
    int i;
    const char* ret = NULL;

    for (i = 0; i < ARRAY_LENGTH(LEGAL_OPERATOR_OPERANDS); i++) {
        if (strcmp(ast_str_node_type(operator_node->type), LEGAL_OPERATOR_OPERANDS[i].op) == 0) {
            bool type_mismatched = true;
            /* if there are 2 operands, it is not a unary operator */
            if (operator_node->num_children == 2) {
                int j;
                char* left_operand_type = NULL;
                char* right_operand_type = NULL;
                ast_node_t* left_operand = operator_node->children[0];
                ast_node_t* right_operand = operator_node->children[1];

                if (left_operand->type == AST_NODE_BLOCK_EXPRESSION_STMT) {
                    left_operand = left_operand->children[0];
                }

                if (right_operand->type == AST_NODE_BLOCK_EXPRESSION_STMT) {
                    right_operand = right_operand->children[0];
                }

                if (semantics_node_type_is_operator(left_operand->type)) {
                    left_operand_type = (char*)_operands_type_check(left_operand);
                } else {
                    if (strcmp(ast_str_node_type(left_operand->type), "id") != 0) {
                        if (operator_node->type == AST_NODE_BINARY_OP_ASSIGNMENT) {
                            if (left_operand->type == AST_NODE_FUNC_CALL) {
                                SEMANTICS_ERROR(operator_node->node_info->linenum, SEMANTICS_NON_VAR_ASSIGMENT_ERR);
                            }
                        }
                        if (left_operand->type == AST_NODE_FUNC_CALL) {
                            ast_node_t* func_call_sym = left_operand->children[0];
                            left_operand_type = func_call_sym->symbol_loc->rv_sig;
                        } else {
                            left_operand_type = ast_str_node_type(left_operand->type);
                        }
                    } else {
                        if (operator_node->type == AST_NODE_BINARY_OP_ASSIGNMENT) {
                            /* left operand cannot be a constant for assignment */
                            if (left_operand->symbol_loc->is_const) {
                                SEMANTICS_ERROR(left_operand->node_info->linenum, SEMANTICS_CONSTANT_ASSIGNMENT_ERR);
                            }
                            /* an ID will have a location on the symbol table */
                            left_operand_type = left_operand->symbol_loc->sig;
                        } else {
                            /* an ID will have a location on the symbol table */
                            left_operand_type = left_operand->symbol_loc->sig;
                        }
                    }
                }

                if (semantics_node_type_is_operator(right_operand->type)) {
                    right_operand_type = (char*)_operands_type_check(right_operand);
                } else {
                    if (strcmp(ast_str_node_type(right_operand->type), "id") != 0) {
                        if (right_operand->type == AST_NODE_NUM) {
                            size_t max_int_str_len = 10;

                            if (right_operand->node_info->lexeme[0] == '-') {
                                max_int_str_len = 11;
                            }
                            if (strlen(right_operand->node_info->lexeme) > max_int_str_len) {
                                SEMANTICS_ERROR(right_operand->node_info->linenum, SEMANTICS_INT_OUT_OF_RANGE_ERR)
                            }
                            if (atol(right_operand->node_info->lexeme) > 2147483647) {
                                SEMANTICS_ERROR(right_operand->node_info->linenum, SEMANTICS_INT_TOO_LARGE_ERR)
                            }
                            if (atol(right_operand->node_info->lexeme) < -2147483648) {
                                SEMANTICS_ERROR(right_operand->node_info->linenum, SEMANTICS_INT_TOO_SMALL_ERR)
                            }
                            right_operand_type = ast_str_node_type(right_operand->type);
                        } else if (right_operand->type == AST_NODE_FUNC_CALL) {
                            right_operand_type = right_operand->children[0]->symbol_loc->rv_sig;
                        } else {
                            right_operand_type = ast_str_node_type(right_operand->type);
                        }
                    } else {
                        /* an ID will have a location on the symbol table 
                           However, cannot assign to a type */
                        if (right_operand->symbol_loc->is_type) {
                            SEMANTICS_ERROR(right_operand->node_info->linenum, 
                                            SEMANTICS_USE_TYPE_FOR_ASSIGNMENT_ERR, 
                                            right_operand->symbol_loc->symbol_name);
                        }
                        right_operand_type = right_operand->symbol_loc->sig;
                    }
                }

                for (j = 0; j < 3; j++) {
                    if (left_operand_type != NULL &&
                        right_operand_type != NULL &&
                        LEGAL_OPERATOR_OPERANDS[i].operands[j][0] != NULL &&
                        LEGAL_OPERATOR_OPERANDS[i].operands[j][1] != NULL) {
                        if (strcmp(left_operand_type, LEGAL_OPERATOR_OPERANDS[i].operands[j][0]) == 0 &&
                            strcmp(right_operand_type, LEGAL_OPERATOR_OPERANDS[i].operands[j][1]) == 0) {
                            ret = LEGAL_OPERATOR_OPERANDS[i].operands[j][2];
                            operator_node->symbol_loc->sig = (char*)LEGAL_OPERATOR_OPERANDS[i].operands[j][2];
                            type_mismatched = false;
                            break;
                        }
                    }
                }
            } else {
                char* right_operand_type;
                ast_node_t* right_operand = operator_node->children[0];

                if (semantics_node_type_is_operator(right_operand->type)) {
                    right_operand_type = (char*)_operands_type_check(right_operand);
                } else {
                    if (operator_node->type == AST_NODE_BINARY_OP_NEGATE) {
                        operator_node->type = AST_NODE_UNARY_OP_LOGICAL_NEGATE;
                    }
                    if (strcmp(ast_str_node_type(right_operand->type), "id") != 0) {
                        if (right_operand->type == AST_NODE_FUNC_CALL) {
                            right_operand_type = right_operand->children[0]->symbol_loc->rv_sig;
                        } else {
                            if (strcmp(right_operand->node_info->lexeme, "true") == 0 ||
                                strcmp(right_operand->node_info->lexeme, "false") == 0) {
                                right_operand_type = SEMANTICS_TYPE_BOOL;
                            } else {
                                right_operand_type = ast_str_node_type(right_operand->type);
                            }
                        }
                    } else {
                        /* an ID will have a location on the symbol table */
                        right_operand_type = right_operand->symbol_loc->sig;
                    }
                }

                /* unary operator ! */
                if (operator_node->type == AST_NODE_UNARY_OP_LOGICAL_NEGATE) {
                    if (right_operand_type != NULL &&
                        LEGAL_OPERATOR_OPERANDS[i].operands[0][1] != NULL) {
                        if (strcmp(right_operand_type, LEGAL_OPERATOR_OPERANDS[i].operands[0][1]) == 0) {
                            ret = LEGAL_OPERATOR_OPERANDS[i].operands[0][2];
                            operator_node->symbol_loc->sig = (char*)LEGAL_OPERATOR_OPERANDS[i].operands[0][2];
                            type_mismatched = false;
                        }
                    }
                }
                /* unary operator - */
                if (operator_node->type == AST_NODE_UNARY_OP_NEGATE) {
                    if (right_operand_type != NULL &&
                        LEGAL_OPERATOR_OPERANDS[i].operands[0][1] != NULL) {
                        if (strcmp(right_operand_type, LEGAL_OPERATOR_OPERANDS[i].operands[0][1]) == 0) {
                            ret = LEGAL_OPERATOR_OPERANDS[i].operands[0][2];
                            operator_node->symbol_loc->sig = (char*)LEGAL_OPERATOR_OPERANDS[i].operands[0][2];
                            type_mismatched = false;
                        }
                    }
                }
            }
            if (type_mismatched) {
                SEMANTICS_ERROR(operator_node->node_info->linenum, 
                                SEMANTICS_OPERATOR_OPERAND_TYPE_ERR_FMT, 
                                ast_str_node_type(operator_node->type));
            }
            break;
        }
    }

    return ret;
}

static char** _get_func_sig_params(const char* func_sig, int *num, int linenum)
{
    char** ret = NULL;
    char* func_sig_start = strstr(func_sig, "(");
    int size = 0;
    
    if (func_sig_start != NULL) {
        if (func_sig_start[1] != ')') {
            int i;
            int ret_idx = 0;

            size++;
            func_sig_start++;

            ret = (char**)calloc(sizeof(char*), size);
            ret[size - 1] = (char*)calloc(sizeof(char), 64);
            for (i = 0; i < strlen(func_sig_start) - 1; i++) {
                if (func_sig_start[i] == ',') {
                    size++;
                    ret = (char**)realloc(ret, sizeof(char*) * size);
                    ret[size - 1] = (char*)calloc(sizeof(char), 64);
                    ret_idx = 0;
                } else {
                    ret[size - 1][ret_idx] = func_sig_start[i];
                    ret_idx++;
                }
            }
        }
    } else {
        SEMANTICS_ERROR(linenum, SEMANTICS_FUNC_CALL_NOT_FUNC_ERR);
    }

    *num = size;

    return ret;
}

static void _var_decl(semantics_stab_record_t** sym_loc, 
                      ast_node_t* var_decl_id,
                      ast_node_t* var_decl_type)
{
    semantics_stab_record_t record = {};
    int linenum = 0;

    if (var_decl_id != NULL) {
        record.symbol_name = var_decl_id->node_info->lexeme;
        linenum = var_decl_id->node_info->linenum;
    } else {
        record.symbol_name = "";
    }

    if (var_decl_type != NULL) {
        /* checks if variable type exists */
        var_decl_type->symbol_loc = semantics_lookup(var_decl_type);
        record.sig = var_decl_type->node_info->lexeme;
    } else {
        record.sig = "";
    }

    /* variables don't have return type */
    record.rv_sig = SEMANTICS_NO_TYPE;
    record.is_type = false;
    record.is_const = false;

    *sym_loc = semantics_define(&record, linenum);
}

static void _semantics_decl_check(ast_node_t** nodes, int num_children)
{
    int i;

    if (nodes == NULL || num_children == 0) {
        return;
    }

    for (i = 0; i < num_children; i++) {
        if (nodes[i]->type == AST_NODE_VAR_GLOBAL_DECL) {
            ast_node_t* var_type = nodes[i]->children[1];

            var_type->symbol_loc = semantics_lookup(var_type);
            continue;
        }
        if (nodes[i]->type == AST_NODE_FUNC) {
            int j;
            ast_node_t* func_name = nodes[i]->children[0];
            ast_node_t* func_params = nodes[i]->children[1]->children[0];
            ast_node_t* func_body = nodes[i]->children[2];

            semantics_open_scope(SEMANTICS_FUNCTION_NAME_SCOPE("%s", func_name->node_info->lexeme));
            for (j = 0; j < func_params->num_children; j++) {
                ast_node_t* func_param_var_id = func_params->children[j]->children[0];
                ast_node_t* func_param_var_type = func_params->children[j]->children[1];

                _var_decl(&(func_param_var_id->symbol_loc), 
                            func_param_var_id, 
                            func_param_var_type);
            }

            _semantics_decl_check(func_body->children, func_body->num_children);
            semantics_close_scope();
            continue;
        }
        if (nodes[i]->type == AST_NODE_VAR_DECL) {
            ast_node_t* var_id = nodes[i]->children[0];
            ast_node_t* var_type = nodes[i]->children[1];
            
            _var_decl(&(nodes[i]->symbol_loc),
                        var_id,
                        var_type);
        }
        if (nodes[i]->type == AST_NODE_FUNC_CALL) {
            int j;
            ast_node_t* func_call_id = nodes[i]->children[0];
            ast_node_t* func_call_params = nodes[i]->children[1];
            bool func_call_next = false;

            /* check if the function that is being called exists */
            func_call_id->symbol_loc = semantics_lookup(func_call_id);

            if (func_call_params->num_children > 0) {
                for (j = 0; j < func_call_params->num_children; j++) {
                    ast_node_t* param = func_call_params->children[j];

                    if (param->type == AST_NODE_FUNC_CALL) {
                        _semantics_decl_check(func_call_params->children, func_call_params->num_children);
                        func_call_next = true;
                    } else {
                        /* check to see if the parameters passed to the function call exist */
                        param->symbol_loc = semantics_lookup(param);
                    }
                }
            }

            if (func_call_next) {
                continue;
            }
        }
        if (nodes[i]->type == AST_NODE_RETURN) {
            if (nodes[i]->num_children > 0) {
                ast_node_t* return_id = nodes[i]->children[0];

                /* if it's a function call, check to see if the function id exists */
                if (return_id->type == AST_NODE_FUNC_CALL) {
                    return_id = return_id->children[0];
                }
                /* check to see if we are returning an existing variable or not */
                return_id->symbol_loc = semantics_lookup(return_id);
            }
        }
        if (nodes[i]->type == AST_NODE_ID ||
            nodes[i]->type == AST_NODE_FOR_LOOP_ID) {
            nodes[i]->symbol_loc = semantics_lookup(nodes[i]);
        }
        if (semantics_node_type_is_operator(nodes[i]->type)) {
            nodes[i]->symbol_loc = semantics_lookup(nodes[i]);
        }
        if (nodes[i]->type == AST_NODE_BLOCK) {
            semantics_open_scope("Block");
        }

        _semantics_decl_check(nodes[i]->children, nodes[i]->num_children);
        if (nodes[i]->type == AST_NODE_BLOCK) {
            semantics_close_scope();
        }
    }
}

static void _semantics_type_check(ast_node_t** nodes, int num_children)
{
    int i;
    if (nodes == NULL || num_children == 0) {
        return;
    }

    for (i = 0; i < num_children; i++) {
        if (nodes[i]->type == AST_NODE_VAR_DECL ||
            nodes[i]->type == AST_NODE_VAR_GLOBAL_DECL) {
            ast_node_t* var_name = nodes[i]->children[0];
            ast_node_t* var_type = nodes[i]->children[1];

            if (strcmp(var_name->node_info->lexeme, var_type->node_info->lexeme) == 0) {
                if (nodes[i]->type == AST_NODE_VAR_GLOBAL_DECL) {
                    SEMANTICS_ERROR(var_type->node_info->linenum, SEMANTICS_TYPE_ERROR, var_type->node_info->lexeme);
                }
            } else {
                if (!var_type->symbol_loc->is_type) {
                    SEMANTICS_ERROR(var_type->node_info->linenum, SEMANTICS_TYPE_ERROR, var_type->node_info->lexeme);
                }
            }
        }
        if (nodes[i]->type == AST_NODE_FUNC_CALL) {
            int j;
            ast_node_t* func_call_id = nodes[i]->children[0];
            ast_node_t* func_call_params = nodes[i]->children[1];
            int num_func_sig_params = 0;
            char** func_sig_params = NULL;

            /* literals are not functions */
            if (func_call_id->type == AST_NODE_NUM ||
                func_call_id->type == AST_NODE_STRING) {
                SEMANTICS_ERROR(func_call_id->node_info->linenum, SEMANTICS_FUNC_CALL_NOT_FUNC_ERR);
            }
            
            func_sig_params = _get_func_sig_params(func_call_id->symbol_loc->sig, 
                                                   &num_func_sig_params,
                                                   func_call_id->node_info->linenum);
            if (func_sig_params == NULL) {
                if (func_call_params->num_children > 0) {
                    SEMANTICS_ERROR(func_call_id->node_info->linenum, SEMANTICS_FUNC_CALL_NUM_ARGS_ERR);
                }
            } else {
                if (func_call_params->num_children != num_func_sig_params) {
                    SEMANTICS_ERROR(func_call_id->node_info->linenum, SEMANTICS_FUNC_CALL_NUM_ARGS_ERR);
                }
            }

            /* check if the type of arguments passed to function matches with function signature */            
            for (j = 0; j < num_func_sig_params; j++) {
                int z = 0;
                while (func_sig_params[j][z] == ' ') {
                    func_sig_params[j]++;
                    z++;
                }
                if (func_call_params->children[j]->type == AST_NODE_FUNC_CALL) {
                    /* if the argument of a function call is another function call, we check the return type of that function call */
                    ast_node_t* next_func_call_id = func_call_params->children[j]->children[0];

                    if (next_func_call_id->symbol_loc != NULL) {
                        if (strcmp(next_func_call_id->symbol_loc->rv_sig, func_sig_params[j]) != 0) {
                            SEMANTICS_ERROR(next_func_call_id->node_info->linenum, SEMANTICS_FUNC_CALL_ARGS_TYPE_ERR);
                        }
                    }
                    _semantics_type_check(func_call_params->children, func_call_params->num_children);
                } else {
                    /* if we know the symbol location, that means it's not a literal constant, otherwise it is either an integer, boolean, or string constant */
                    if (func_call_params->children[j]->symbol_loc != NULL) {
                        if (strcmp(func_sig_params[j], func_call_params->children[j]->symbol_loc->sig) != 0) {
                            SEMANTICS_ERROR(func_call_params->children[j]->node_info->linenum, SEMANTICS_FUNC_CALL_ARGS_TYPE_ERR);
                        }
                    } else {
                        if (strcmp(func_sig_params[j], ast_str_node_type(func_call_params->children[j]->type)) != 0) {
                            SEMANTICS_ERROR(func_call_params->node_info->linenum, SEMANTICS_FUNC_CALL_ARGS_TYPE_ERR);
                        }
                    }
                }
            }
        }
        if (nodes[i]->type == AST_NODE_FUNC) {
            ast_node_t* func_name = nodes[i]->children[0];
            ast_node_t* func_sig = nodes[i]->children[1];
            ast_node_t* func_sig_param = func_sig->children[0];
            ast_node_t* func_rt_type = func_sig->children[1];
            int j;

            semantics_open_scope(SEMANTICS_FUNCTION_NAME_SCOPE("%s", func_name->node_info->lexeme));

            for (j = 0; j < func_sig_param->num_children; j++) {
                ast_node_t* func_sig_param_type = func_sig_param->children[j]->children[1];
                if (!func_sig_param_type->symbol_loc->is_type) {
                    SEMANTICS_ERROR(func_sig_param_type->node_info->linenum, SEMANTICS_TYPE_ERROR, func_sig_param_type->symbol_loc->symbol_name);
                }
            }

            if (!func_rt_type->symbol_loc->is_type) {
                SEMANTICS_ERROR(func_rt_type->node_info->linenum, SEMANTICS_TYPE_ERROR, func_rt_type->symbol_loc->symbol_name);
            }
        }
        if (semantics_node_type_is_operator(nodes[i]->type)) {
            (void)_operands_type_check(nodes[i]);
        }
        if (nodes[i]->type == AST_NODE_IF_LOOP ||
            nodes[i]->type == AST_NODE_FOR_LOOP ||
            nodes[i]->type == AST_NODE_IF_ELSE_LOOP) {
            char* loop_id_type = NULL;

            if (semantics_node_type_is_operator(nodes[i]->children[0]->type)) {
                loop_id_type = (char*)_operands_type_check(nodes[i]->children[0]);    
            } else if (nodes[i]->children[0]->type == AST_NODE_NUM) {
                loop_id_type = ast_str_node_type(nodes[i]->children[0]->type);
            } else if (nodes[i]->children[0]->type == AST_NODE_ID ||
                       nodes[i]->children[0]->type == AST_NODE_FOR_LOOP_ID) {
                loop_id_type = nodes[i]->children[0]->symbol_loc->sig;
            }

            if (strcmp(loop_id_type, SEMANTICS_TYPE_BOOL) != 0) {
                SEMANTICS_ERROR(nodes[i]->children[0]->node_info->linenum, 
                                SEMANTICS_LOOP_ID_TYPE_ERR, 
                                ast_str_node_type(nodes[i]->type));
            }

            continue;
        }
        if (nodes[i]->type == AST_NODE_RETURN) {
            /* if we aren't in any function scope, _return_func_symbol_from_scope() should fail due to retuning outside of a function */
            semantics_stab_record_t* curr_func_symbol = _return_func_symbol_from_scope(nodes[i]->node_info->linenum);

            if (strcmp(curr_func_symbol->rv_sig, "$void") == 0) {
                curr_func_symbol->rv_sig = SEMANTICS_TYPE_VOID;
            }
            if (nodes[i]->num_children > 0) {
                ast_node_t* return_id = nodes[i]->children[0];

                /* can't return values for void function */
                if (return_id->num_children > 0 && (strcmp(curr_func_symbol->rv_sig, SEMANTICS_TYPE_VOID) == 0)) {
                    SEMANTICS_ERROR(nodes[i]->node_info->linenum, SEMANTICS_VOID_FUNC_RETURN_ERR);
                }

                /* if it's a function call, check to see if the function returns the correct type */
                if (return_id->type == AST_NODE_FUNC_CALL) {
                    return_id = return_id->children[0];
                    if (strcmp(return_id->symbol_loc->rv_sig, curr_func_symbol->rv_sig) != 0) {
                        SEMANTICS_ERROR(nodes[i]->node_info->linenum, SEMANTICS_FUNC_RETURN_ERR);
                    }
                } else {
                    if (return_id->type == AST_NODE_NUM ||
                        return_id->type == AST_NODE_STRING) {
                        if (strcmp(ast_str_node_type(return_id->type), curr_func_symbol->rv_sig) != 0) {
                            SEMANTICS_ERROR(nodes[i]->node_info->linenum, SEMANTICS_FUNC_RETURN_ERR);
                        }
                    } else {
                        if (strcmp(return_id->symbol_loc->sig, curr_func_symbol->rv_sig) != 0) {
                            SEMANTICS_ERROR(nodes[i]->node_info->linenum, SEMANTICS_FUNC_RETURN_ERR);
                        }
                    }
                }
            }
        }
        _semantics_type_check(nodes[i]->children, nodes[i]->num_children);
        if (nodes[i]->type == AST_NODE_FUNC) {
            semantics_close_scope();
        }
    }
}

static void _semantics_miscellaneous_check(ast_node_t** nodes, int num_children)
{
    int i;
    if (nodes == NULL || num_children == 0) {
        return;
    }

    for (i = 0; i < num_children; i++) {
        if (nodes[i]->type == AST_NODE_FUNC) {
            ast_node_t* func_block = nodes[i]->children[2];
            ast_node_t* return_stmt_node = NULL;
            bool return_stmt_found = _found_return_stmt(func_block->children, func_block->num_children);

            if (strcmp(nodes[i]->symbol_loc->rv_sig, "void") == 0 ||
                strcmp(nodes[i]->symbol_loc->rv_sig, "$void") == 0) {
                if (return_stmt_found && return_stmt_node != NULL) {
                    if (return_stmt_node->num_children > 0) {
                        SEMANTICS_ERROR(return_stmt_node->node_info->linenum, SEMANTICS_VOID_FUNC_RETURN_ERR);
                    }
                }
            } else {
                if (!return_stmt_found) {
                    SEMANTICS_ERROR(nodes[i]->node_info->linenum, SEMANTICS_NO_RETURN_STMT_FOUND);
                } else {
                    if (return_stmt_node != NULL) {
                        if (return_stmt_node->num_children == 0) {
                            SEMANTICS_ERROR(return_stmt_node->node_info->linenum, SEMANTICS_RETURN_NO_VALUE_NON_VOID_FUNC);
                        }
                    }
                }
            }
        }
        if (nodes[i]->type == AST_NODE_FOR_LOOP) {
            /* ignore break keyword here */
            continue;
        }
        if (nodes[i]->type == AST_NODE_BREAK) {
            SEMANTICS_ERROR(nodes[i]->node_info->linenum, SEMANTICS_BREAK_ERR);
        }
        _semantics_miscellaneous_check(nodes[i]->children, nodes[i]->num_children);
    }
}

void semantics_init()
{
    int i;

    stack_init();

    /* universal scope starts immediately */
    semantics_open_scope("Universal");
    for (i = 0; i < ARRAY_LENGTH(UNIVERSE_SCOPE_SEMANTICS); i++) {
        (void)semantics_define(&UNIVERSE_SCOPE_SEMANTICS[i], 0);
    }
    for (i = 0; i < ARRAY_LENGTH(LEGAL_OPERATOR_OPERANDS); i++) {
        semantics_stab_record_t record = {};

        record.symbol_name = (char*)LEGAL_OPERATOR_OPERANDS[i].op;
        record.sig = (char*)LEGAL_OPERATOR_OPERANDS[i].operands[0][2];
        record.rv_sig = SEMANTICS_NO_TYPE;
        record.is_const = false;
        record.is_type = false;

        (void)semantics_define(&record, 0);
    }
}

bool semantics_node_type_is_operator(ast_node_type_t node_type) 
{
    return  (
                node_type == AST_NODE_BINARY_OR_CMP ||
                node_type == AST_NODE_BINARY_AND_CMP ||
                node_type == AST_NODE_BINARY_OP_EQ_CMP ||
                node_type == AST_NODE_BINARY_OP_NOT_EQ_CMP ||
                node_type == AST_NODE_BINARY_OP_ASSIGNMENT ||
                node_type == AST_NODE_BINARY_OP_LT ||
                node_type == AST_NODE_BINARY_OP_GT ||
                node_type == AST_NODE_BINARY_OP_LE ||
                node_type == AST_NODE_BINARY_OP_GE ||
                node_type == AST_NODE_BINARY_OP_ADD ||
                node_type == AST_NODE_BINARY_OP_SUB ||
                node_type == AST_NODE_BINARY_OP_MULT ||
                node_type == AST_NODE_BINARY_OP_DIV ||
                node_type == AST_NODE_BINARY_OP_MOD ||
                node_type == AST_NODE_BINARY_OP_NEGATE ||
                node_type == AST_NODE_UNARY_OP_NEGATE ||
                node_type == AST_NODE_UNARY_OP_LOGICAL_NEGATE
            );
}

bool semantics_node_type_is_unary_operator(ast_node_type_t node_type)
{
    return  (
                node_type == AST_NODE_UNARY_OP_LOGICAL_NEGATE ||
                node_type == AST_NODE_UNARY_OP_NEGATE
            );
}

semantics_stab_record_t* semantics_define(semantics_stab_record_t* record, int linenum)
{
    node_t* current_scope_stab = stack_peek()->contents;

    while (current_scope_stab != NULL) {
        if (strcmp(current_scope_stab->data.symbol_name, record->symbol_name) == 0) {
            SEMANTICS_ERROR(linenum, SEMANTICS_VAR_REDECLARE_ERR_FMT, record->symbol_name);
        }
        current_scope_stab = current_scope_stab->next;
    }

    return linked_list_append(&(stack_peek()->contents), record);
}

semantics_stab_record_t* semantics_lookup(ast_node_t* symbol)
{
    semantics_stab_record_t* ret = NULL;
    int curr_stack_index = stack_top_index();
    int linenum = 0;
    bool found = false;
    char* symbol_name = NULL;

    /* no need to look up for literals */
    if (symbol->type != AST_NODE_NUM &&
        symbol->type != AST_NODE_STRING) {
        if (symbol->node_info == NULL) {
            symbol_name = ast_str_node_type(symbol->type);
        } else {
            if (symbol->node_info->lexeme == NULL) {
                symbol_name = ast_str_node_type(symbol->type);
            } else {
                symbol_name = symbol->node_info->lexeme;
                linenum = symbol->node_info->linenum;
            }
        }
        
        while (curr_stack_index >= 0) {
            semantics_scope_t* curr_scope = stack_peek_at(curr_stack_index);
            
            if (linked_list_symbol_found(curr_scope->contents, symbol_name, &ret)) {
                found = true;
                break;
            }

            curr_stack_index--;
        }

        if (!found) {
            if (strcmp(symbol_name, "main") == 0) {
                SEMANTICS_ERROR(linenum, SEMANTICS_MAIN_MISSING);
            } else {
                SEMANTICS_ERROR(linenum, SEMANTICS_VAR_UNDEFINED_ERR_FMT, symbol_name);
            }
        }
    }

    return ret;
}

void semantics_open_scope(const char* scope_name)
{
    semantics_scope_t* new_scope = (semantics_scope_t*)calloc(sizeof(semantics_scope_t), 1);

    if (new_scope != NULL) {
        new_scope->scope_name = scope_name;
        stack_push(new_scope);
    }
}

void semantics_close_scope()
{
    (void)stack_pop();
}

void semantics_global_decl_check(ast_start_t* ast_annotated, int ast_annotated_node_count)
{
    int i;
    ast_node_t** nodes = ast_annotated->nodes;
    int num_children = ast_annotated_node_count;
    ast_parse_info_t node_info = {
        .lexeme = "main",
        .linenum = 0,
        .token_count = 0
    };
    ast_node_t* main_func_node = ast_node_alloc(&node_info,
                                                AST_NODE_ID,
                                                0,
                                                NULL);

    if (ast_annotated->type == AST_NODE_START) {
        semantics_stab_record_t record = {};

        semantics_open_scope("File");

        record.symbol_name = ast_annotated->node_name;
        record.sig = SEMANTICS_TYPE_VOID;
        record.rv_sig = SEMANTICS_NO_TYPE;
        record.is_const = false;
        record.is_type = false;

        ast_annotated->symbol_loc = semantics_define(&record, 0);
    }

    for (i = 0; i < num_children; i++) {
        if (nodes[i]->type == AST_NODE_VAR_GLOBAL_DECL) {
            semantics_stab_record_t record = {};

            /* if it is a variable declaration node, the 1st child node is the symbol name and the 2nd child node is the type */
            if (nodes[i]->children[0]->node_info->lexeme != NULL) {
                record.symbol_name = nodes[i]->children[0]->node_info->lexeme;
            }

            /* check if type exists*/
            nodes[i]->children[1]->symbol_loc = semantics_lookup(nodes[i]->children[1]);
            /* the variable type should be a predeclared one */
            record.sig = nodes[i]->children[1]->node_info->lexeme;
            record.rv_sig = SEMANTICS_NO_TYPE;
            record.is_const = false;
            record.is_type = false;

            nodes[i]->symbol_loc = semantics_define(&record, nodes[i]->node_info->linenum);
        } else if (nodes[i]->type == AST_NODE_FUNC) {
            semantics_stab_record_t record = {};
            ast_node_t* func_sig = nodes[i]->children[1];
            ast_node_t* func_id = nodes[i]->children[0];

            /* special case: main cannot have any arguments */
            if (strcmp(func_id->node_info->lexeme, "main") == 0) {
                ast_node_t* func_sig_rv = func_sig->children[1];

                if (func_sig->children[0]->num_children > 0) {
                    SEMANTICS_ERROR(func_id->node_info->linenum, SEMANTICS_MAIN_ARGS_NUM_ERROR);
                }
                if (strcmp(func_sig_rv->node_info->lexeme, "void") != 0 &&
                    strcmp(func_sig_rv->node_info->lexeme, "$void") != 0) {
                    SEMANTICS_ERROR(func_sig_rv->node_info->linenum, SEMANTICS_MAIN_RETURN_ERR);
                }
            }

            /* if it is a function declaration node, the 1st child node is the symbol name and the 2nd child node is the signature,
               we do not care about blocks for pass 1, which is the 3rd child */
            if (nodes[i]->children[0]->node_info->lexeme != NULL) {
                record.symbol_name = nodes[i]->children[0]->node_info->lexeme;
            }

            /* for function signature, 1st child is the formals, 2nd child is the return type */
            /* no function parameters */
            if (func_sig->children[0]->num_children == 0) {
                record.sig = "f()";
            } else {
                /* only dealing with function signature, pass1 does not consider the actual symbol names that are the parameters */
                int i;
                char func_sig_str[64];


                strcpy(func_sig_str, "f(");
                
                for (i = 0; i < func_sig->children[0]->num_children; i++) {
                    ast_node_t* func_formal = func_sig->children[0]->children[i];

                    /* for each parameter, the first child is the id and the 2nd child is the type */
                    if (i == 0) {
                        strcat(func_sig_str, func_formal->children[1]->node_info->lexeme);
                    } else {
                        strcat(func_sig_str, ", ");
                        strcat(func_sig_str, func_formal->children[1]->node_info->lexeme);
                    }
                }
                strcat(func_sig_str, ")");
                record.sig = strdup(func_sig_str);
            }
            /* check if return type exists */
            func_sig->children[1]->symbol_loc = semantics_lookup(func_sig->children[1]);
            record.rv_sig = func_sig->children[1]->node_info->lexeme;
            record.is_const = false;
            record.is_type = false;

            nodes[i]->symbol_loc = semantics_define(&record, nodes[i]->node_info->linenum);
        }
    }

    /* check to see if main was ever defined, if it is check if see if it is a function */
    if (strstr(semantics_lookup(main_func_node)->sig, "f(") == NULL) {
        SEMANTICS_ERROR(0, SEMANTICS_MAIN_MISSING);
    }
}

void semantics_decl_check(ast_start_t* ast_annotated, int ast_annotated_node_count)
{
    _semantics_decl_check(ast_annotated->nodes, ast_annotated_node_count);
}

void semantics_type_check(ast_start_t* ast_annotated, int ast_annotated_node_count)
{
    stack_reset_to_file_scope();
    _semantics_type_check(ast_annotated->nodes, ast_annotated_node_count);
}

void semantics_miscellaneous_check(ast_start_t* ast_annotated, int ast_annotated_node_count)
{
    _semantics_miscellaneous_check(ast_annotated->nodes, ast_annotated_node_count);
}

void semantics_stab_print()
{
    int curr_scope_index = stack_top_index();
    
    while (curr_scope_index >= 0) {
        semantics_scope_t* curr_scope = stack_peek_at(curr_scope_index);

        printf("%s scope:\n", curr_scope->scope_name);
        linked_list_print(curr_scope->contents);
        printf("\n");

        curr_scope_index--;
    }
}

void semantics_record_print(semantics_stab_record_t* record)
{
    if (record != NULL) {
        printf("{%s, %s, %s, %s, %s}\n", strcmp(record->symbol_name, SEMANTICS_NO_TYPE) == 0 ? "\"\"" : record->symbol_name,
                                         strcmp(record->sig, SEMANTICS_NO_TYPE) == 0 ? "\"\"" : record->sig,
                                         strcmp(record->rv_sig, SEMANTICS_NO_TYPE) == 0 ? "\"\"" : record->rv_sig,
                                         record->is_const == true ? "true" : "false",
                                         record->is_type == true ? "true" : "false");
    }
}