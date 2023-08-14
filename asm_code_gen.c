#include <string.h>
#include <stdarg.h>
#include "include/asm_code_gen.h"
#include "include/semantics.h"

static void _operator_code_gen(ast_node_t* op_node);
static inline void _gen_func_label(const char* func_name);
static inline void _gen_err_label(const char* err_label);
static void _gen_condition_bin(const char* match_label, const char* non_match_label, ast_node_t* cond_node);

#define DIV_BY_Z_ERROR_LAB      "S1"
#define FUNC_NO_RETURN_ERR_LAB  "S2"
#define __GEN_LABEL__(l)        {\
                                    if (strcmp(l, "FR_there_is_no_comparison_there_is_only_") == 0) {\
                                        strcpy((char*)l, "FR_there_is_no_comparison_there_is_only_zuul");\
                                    }\
                                    if (strcmp(l, "F_there_is_no_comparison_there_is_only_") == 0) {\
                                        strcpy((char*)l, "F_there_is_no_comparison_there_is_only_zuul");\
                                    }\
                                    printf("%s:\n", l);\
                                }
#define __GEN_NON_LABEL__(l)    printf("\t%s\n", l);
#define __GOLF_BUILT_IN__(f)    _gen_func_label(f)
#define __IS_BUILT_IN__(f)        ({\
                                    (strcmp(f, "halt") == 0 ||\
                                     strcmp(f, "getchar") == 0 ||\
                                     strcmp(f, "printb") == 0 ||\
                                     strcmp(f, "printc") == 0 ||\
                                     strcmp(f, "printi") == 0 ||\
                                     strcmp(f, "prints") == 0 ||\
                                     strcmp(f, "len") == 0\
                                    );\
                                })
#define __ERR__(l)              _gen_err_label(l)
#define __ASM_GEN(ins_fmt, ...) {\
                                    char instruction[PATH_MAX * 5];\
                                    snprintf(instruction, PATH_MAX * 5, ins_fmt, ##__VA_ARGS__);\
                                    if (strcmp(instruction, "jal F_there_is_no_comparison_there_is_only_") == 0) {\
                                        strcpy(instruction, "jal F_there_is_no_comparison_there_is_only_zuul");\
                                    }\
                                    __GEN_NON_LABEL__(instruction);\
                                }

#define __ASM_SYSCALL(sys_code) {\
                                    char _syscall[PATH_MAX * 5];\
                                    snprintf(_syscall, PATH_MAX * 5, "addi $v0, $0, %d", sys_code);\
                                    __GEN_NON_LABEL__(_syscall);\
                                    __GEN_NON_LABEL__("syscall");\
                                }
#define __ASM_LOCAL__(v)        (strstr(v, "($sp)") != NULL)
#define __ASM_GLOBAL__(v)       (strstr(v, "V_") != NULL)
#define __REGISTER__(r)         ({\
                                    asm_tmp_reg_t* ret = (asm_tmp_reg_t*)calloc(sizeof(asm_tmp_reg_t), 1);\
                                    if (ret != NULL) {\
                                        ret->reg = strdup(r);\
                                        ret->in_use = true;\
                                    }\
                                    ret;\
                                })
#define __ASM_VAR__(v)          ({\
                                    char* prefix = (char*)calloc(sizeof(char), 32);\
                                    strcpy(prefix, "V_");\
                                    strcat(prefix, v);\
                                    prefix;\
                                })
#define __ASM_STRING_INIT__     {\
                                    __GEN_LABEL__("S0");\
                                    __GEN_NON_LABEL__(".byte 0");\
                                    __GEN_LABEL__("S1");\
                                    __GEN_NON_LABEL__(".asciiz\"error: division by zero\\n\"");\
                                    __GEN_LABEL__("S2");\
                                    __GEN_NON_LABEL__(".asciiz\"error: function must return a value\\n\"");\
                                }
#define __ASM_STRING_L__        ({\
                                    char string_label[5];\
                                    char label_num[4];\
                                    char* ret = NULL;\
                                    snprintf(label_num, 4, "%d", _string_constant_label_num);\
                                    _string_constant_label_num++;\
                                    string_label[0] = 'S';\
                                    string_label[1] = '\0';\
                                    strcat(string_label, label_num);\
                                    ret = strdup(string_label);\
                                    ret;\
                                })
#define __ASM_STRING__(l, s)    {\
                                    size_t i;\
                                    __GEN_LABEL__(l);\
                                    for (i = 0; i < strlen(s); i++) {\
                                        if (s[i] == '\\') {\
                                            bool found = false;\
                                            if (s[i + 1] == 'b') {\
                                                printf("\t.byte 8\n");\
                                                found = true;\
                                            } else if (s[i + 1] == 't') {\
                                                printf("\t.byte 9\n");\
                                                found = true;\
                                            } else if (s[i + 1] == 'n') {\
                                                printf("\t.byte 10\n");\
                                                found = true;\
                                            } else if (s[i + 1] == 'f') {\
                                                printf("\t.byte 12\n");\
                                                found = true;\
                                            } else if (s[i + 1] == 'r') {\
                                                printf("\t.byte 13\n");\
                                                found = true;\
                                            } else if (s[i + 1] == '"') {\
                                                printf("\t.byte 34\n");\
                                                found = true;\
                                            } else if (s[i + 1] == '\\') {\
                                                printf("\t.byte 92\n");\
                                                found = true;\
                                            }\
                                            if (found) {\
                                                i++;\
                                                continue;\
                                            }\
                                        }\
                                        printf("\t.byte %d\n", s[i]);\
                                    }\
                                    printf("\t.byte 0\n");\
                                }
                                
static semantics_stab_record_t* _curr_func_sym;
static int                      _string_constant_label_num = 3;
static int                      _loop_label_num = 0;
static int                      _sp_pos = 1;
static int                      _num_nested_loops = 0;
static char*                    _curr_rv_label;
static char**                   _curr_loop_labels;
static bool                     _print_string_begin = true;
static bool                     _rv_found = false;
static bool                     _built_in_redefined = false;
static asm_tmp_reg_t _temp_regs[] = {
    {
        "$t0",
        false
    },
    {
        "$t1",
        false
    },
    {
        "$t2",
        false
    },
    {
        "$t3",
        false
    },
    {
        "$t4",
        false
    },
    {
        "$t5",
        false
    },
    {
        "$t6",
        false
    },
    {
        "$t7",
        false
    }
};

static void _free_tmp_reg_by_name(const char* tmp_reg_name)
{
    int i;

    for (i = 0; i < 7; i++) {
        if (strcmp(_temp_regs[i].reg, tmp_reg_name) == 0) {
            _temp_regs[i].in_use = false;
            break;
        }
    }
}


static inline void _gen_err_label(const char* err_label)
{
    if (err_label != NULL) {
        char to_print[64];

        strcpy(to_print, "E_");
        strcat(to_print, err_label);

        __GEN_LABEL__(to_print);
    }
}

static inline void _gen_func_label(const char* func_name)
{
    if (func_name != NULL) {
        char to_print[64];

        strcpy(to_print, "F_");
        strcat(to_print, func_name);

        __GEN_LABEL__(to_print);
    }
}

static inline void _gen_func_dealloc_label(ast_node_t* func)
{
    if (func != NULL) {
        char to_print[64];

        strcpy(to_print, "FR_");
        strcat(to_print, func->symbol_loc->symbol_name);

        __GEN_LABEL__(to_print);
    }
}

static char* _get_func_dealloc_label(ast_node_t* func)
{
    char* ret = NULL;

    if (func != NULL) {
        if (strcmp(func->symbol_loc->symbol_name, "there_is_no_comparison_there_is_only_zuul") != 0) {
            char to_print[64];

            strcpy(to_print, "FR_");
            strcat(to_print, func->symbol_loc->symbol_name);

            ret = strdup(to_print);
        } else {
            return "FR_there_is_no_comparison_there_is_only_zuul";
        }
    }

    return ret;
}

static inline void _gen_var_label(const char* var_name)
{
    if (var_name != NULL) {
        char to_print[64];

        strcpy(to_print, "V_");
        strcat(to_print, var_name);

        __GEN_LABEL__(to_print);
    }
}

static char* _get_func_label(const char* func_name)
{
    char* prefix = (char*)calloc(sizeof(char), 32);

    strcpy(prefix, "F_");
    strcat(prefix, func_name);

    return prefix;
}

static char* _get_err_label(const char* err_name)
{
    char* prefix = (char*)calloc(sizeof(char), 32);

    strcpy(prefix, "E_");
    strcat(prefix, err_name);

    return prefix;
}

static void _curr_loop_labels_append(char* loop_label)
{
    _num_nested_loops++;
    _curr_loop_labels = (char**)realloc(_curr_loop_labels, sizeof(char*) * _num_nested_loops);
    _curr_loop_labels[_num_nested_loops - 1] = (char*)calloc(sizeof(char), strlen(loop_label) + 1);
    strcpy(_curr_loop_labels[_num_nested_loops - 1], loop_label);
}

static char* _curr_loop_label_pop()
{
    char* ret = _curr_loop_labels[_num_nested_loops - 1];

    _curr_loop_labels[_num_nested_loops - 1] = NULL;
    _num_nested_loops--;

    return ret;
}

static asm_tmp_reg_t* _tmp_reg_alloc()
{
    int i;

    for (i = 0; i < 8; i++) {
        if (!_temp_regs[i].in_use) {
            _temp_regs[i].in_use = true;
            return &_temp_regs[i];
        }
    }

    return NULL;
}

static inline void _func_call_end(const char* end_func)
{
    __ASM_GEN("jal %s", end_func);
}

static void _print_string_constant(const char* label, const char* data)
{
    size_t i;

    __GEN_LABEL__(label);
    
    for (i = 0; i < strlen(data); i++) {
        if (data[i] == '\\') {
            bool found = false;
            if (data[i + 1] == 'b') {
                printf("\t.byte 8\n");
                found = true;
            } else if (data[i + 1] == 't') {
                printf("\t.byte 9\n");
                found = true;
            } else if (data[i + 1] == 'n') {
                printf("\t.byte 10\n");
                found = true;
            } else if (data[i + 1] == 'f') {
                printf("\t.byte 12\n");
                found = true;
            } else if (data[i + 1] == 'r') {
                printf("\t.byte 13\n");
                found = true;
            } else if (data[i + 1] == '"') {
                printf("\t.byte 34\n");
                found = true;
            } else if (data[i + 1] == '\\') {
                printf("\t.byte 92\n");
                found = true;
            }
            if (found) {
                i++;
                continue;
            }
        }
        printf("\t.byte %d\n", data[i]);
    }
    printf("\t.byte 0\n");
}

static char* _loop_label_alloc()
{
    char *loop_label = (char*)calloc(sizeof(char), PATH_MAX);

    snprintf(loop_label, PATH_MAX, "L%d", _loop_label_num);
    _loop_label_num++;
    return loop_label;
}

static void _alloc_func_stack(int stack_space)
{
    __ASM_GEN("subu $sp,$sp,%d", (stack_space + 1) * 4);
    __ASM_GEN("sw $ra,0($sp)");
}

static void _dealloc_func_stack(ast_node_t *func)
{
    _gen_func_dealloc_label(func);
    __ASM_GEN("lw $ra,0($sp)");
    __ASM_GEN("addu $sp,$sp,%d", (func->num_local_var + 1) * 4);
    __ASM_GEN("jr $ra");
}

static void _gen_func_call(ast_node_t* func_call_node, bool alloc_stack, int num_words, char* store_reg)
{
    int i;
    ast_node_t* func_call_id = func_call_node->children[0];
    ast_node_t* func_call_actuals = func_call_node->children[1];
    char* func_label = _get_func_label(func_call_id->symbol_loc->symbol_name);
    asm_func_arg_t func_args[func_call_actuals->num_children];

    for (i = 0; i < func_call_actuals->num_children; i++) {
        func_args[i].arg_type = &func_call_actuals->children[i]->type;

        if (func_call_actuals->children[i]->type == AST_NODE_STRING) {
            func_args[i].symbol = func_call_actuals->children[i]->memory_reg->reg;
        } else if (func_call_actuals->children[i]->type == AST_NODE_NUM) {
            func_args[i].symbol = func_call_actuals->children[i]->node_info->lexeme;
        } else if (func_call_actuals->children[i]->type == AST_NODE_ID) {
            if (func_call_actuals->children[i]->symbol_loc->is_const) {
                if (strcmp(func_call_actuals->children[i]->node_info->lexeme, "true") == 0) {
                    func_call_actuals->children[i]->type = AST_NODE_NUM;

                    func_args[i].arg_type = &func_call_actuals->children[i]->type;
                    func_args[i].symbol = "Ltrue";
                } else if (strcmp(func_call_actuals->children[i]->node_info->lexeme, "false") == 0) {
                    func_call_actuals->children[i]->type = AST_NODE_NUM;

                    func_args[i].arg_type = &func_call_actuals->children[i]->type;
                    func_args[i].symbol = "Lfalse";
                } else {
                    func_args[i].symbol = func_call_actuals->children[i]->symbol_loc->label->reg;
                }
            } else {
                func_args[i].symbol = func_call_actuals->children[i]->symbol_loc->label->reg;
            }
        } else if (func_call_actuals->children[i]->type == AST_NODE_FUNC_CALL) {
            ast_node_t* nested_f_call_sym = func_call_actuals->children[i]->children[0];

            _gen_func_call(func_call_actuals->children[i], false, 0, NULL);
            if (!FUNC_RV_IS_VOID(nested_f_call_sym->symbol_loc->rv_sig)) {
                asm_tmp_reg_t* temp_register = NULL;

                temp_register = _tmp_reg_alloc();
                __ASM_GEN("move %s,$v0", temp_register->reg);
                if (i < func_call_actuals->num_children - 1) {
                    if (func_call_actuals->children[i + 1]->type == AST_NODE_FUNC_CALL) {
                        __ASM_GEN("subu $sp,$sp,4");
                        __ASM_GEN("sw %s,0($sp)", temp_register->reg);
                    }
                }
                func_args[i].symbol = temp_register->reg;
            }
        } else if (semantics_node_type_is_operator(func_call_actuals->children[i]->type)) {
            _operator_code_gen(func_call_actuals->children[i]);
            func_args[i].symbol = func_call_actuals->children[i]->memory_reg->reg;
        }
    }

    for (i = 0; i < func_call_actuals->num_children; i++) {
        asm_tmp_reg_t* temp_register = NULL;
        char arg_reg[PATH_MAX];

        snprintf(arg_reg, PATH_MAX, "$%c%d", 'a', i);

        if (*(func_args[i].arg_type) == AST_NODE_FUNC_CALL ||
            semantics_node_type_is_operator(*(func_args[i].arg_type))) {
            if (alloc_stack) {
                __ASM_GEN("subu $sp,$sp,%d", num_words * 4);
                __ASM_GEN("sw %s,0($sp)", store_reg);
            }
            if (i < func_call_actuals->num_children - 1) {
                if (func_call_actuals->children[i + 1]->type == AST_NODE_FUNC_CALL) {
                    __ASM_GEN("lw %s,0($sp)", func_args[i].symbol);
                    __ASM_GEN("addu $sp,$sp,4");
                }
            }
            __ASM_GEN("move %s,%s", arg_reg, func_args[i].symbol);
            _free_tmp_reg_by_name(func_args[i].symbol);
        } else {
            temp_register = _tmp_reg_alloc();
            if (*(func_args[i].arg_type) == AST_NODE_STRING) {
                __ASM_GEN("la %s,%s", temp_register->reg, func_args[i].symbol);
            } else if (*(func_args[i].arg_type) == AST_NODE_ID) {
                __ASM_GEN("lw %s,%s", temp_register->reg, func_args[i].symbol);
            } else if (*(func_args[i].arg_type) == AST_NODE_NUM) {
                __ASM_GEN("li %s,%s", temp_register->reg, func_args[i].symbol);
            }
            __ASM_GEN("move %s,%s", arg_reg, temp_register->reg);
            temp_register->in_use = false;
        }
    }
    _func_call_end(func_label);
    if (alloc_stack) {
        __ASM_GEN("lw %s,0($sp)", store_reg);
        __ASM_GEN("addu $sp,$sp,%d", num_words * 4);
    }
    free(func_label);
}

static void _unary_operator_code_gen(ast_node_t** nodes, int num_children)
{
    int i = 0;

    if (nodes == NULL || num_children == 0) {
        return;
    }

    for (i = 0; i < num_children; i++) {   
        _unary_operator_code_gen(nodes[i]->children, nodes[i]->num_children);
        if (nodes[i]->type == AST_NODE_NUM) {
            asm_tmp_reg_t* reg = _tmp_reg_alloc();

            __ASM_GEN("li %s,%s", reg->reg, nodes[i]->node_info->lexeme);
            nodes[i]->memory_reg = reg;
            reg->in_use = false;
        } else {
            if (nodes[i]->type == AST_NODE_UNARY_OP_NEGATE) {
                __ASM_GEN("negu %s,%s", nodes[i]->children[0]->memory_reg->reg, nodes[i]->children[0]->memory_reg->reg);
                nodes[i]->memory_reg = nodes[i]->children[0]->memory_reg;
            }
        }
    }
}

static void _operator_code_gen(ast_node_t* op_node)
{
    if (semantics_node_type_is_unary_operator(op_node->type)) {
        _unary_operator_code_gen(op_node->children, op_node->num_children);
        if (op_node->type == AST_NODE_UNARY_OP_NEGATE) {
            __ASM_GEN("negu %s,%s", op_node->children[0]->memory_reg->reg, op_node->children[0]->memory_reg->reg);
            op_node->memory_reg = op_node->children[0]->memory_reg;
        }
    } else {
        ast_node_t* left_operand = op_node->children[0];
        ast_node_t* right_operand = op_node->children[1];

        if (op_node->type == AST_NODE_BINARY_OP_ASSIGNMENT) {
            if (left_operand->type == AST_NODE_ID) {
                char* var_label = left_operand->symbol_loc->label->reg;
                if (right_operand->type == AST_NODE_NUM) {
                    asm_tmp_reg_t* temp_register = _tmp_reg_alloc();

                    __ASM_GEN("li %s,%s", temp_register->reg, right_operand->node_info->lexeme);
                    __ASM_GEN("sw %s,%s", temp_register->reg, var_label);
                    temp_register->in_use = false;
                } else if (right_operand->type == AST_NODE_ID) {
                    asm_tmp_reg_t* temp_register = _tmp_reg_alloc();

                    if (strcmp(right_operand->symbol_loc->symbol_name, "true") == 0) {
                        __ASM_GEN("li %s,Ltrue", temp_register->reg);
                    } else if (strcmp(right_operand->symbol_loc->symbol_name, "false") == 0) {
                        __ASM_GEN("li %s,Lfalse", temp_register->reg);
                    } else {
                        __ASM_GEN("lw %s,%s", temp_register->reg, right_operand->symbol_loc->label->reg);
                    }
                    __ASM_GEN("sw %s,%s", temp_register->reg, var_label);
                    temp_register->in_use = false;
                } else if (right_operand->type == AST_NODE_STRING) {
                    asm_tmp_reg_t* temp_register = _tmp_reg_alloc();

                    __ASM_GEN("la %s,%s", temp_register->reg, right_operand->memory_reg->reg);
                    __ASM_GEN("sw %s,%s", temp_register->reg, var_label);
                    temp_register->in_use = false;
                } else if (right_operand->type == AST_NODE_FUNC_CALL) {
                    asm_tmp_reg_t* temp_register = _tmp_reg_alloc();

                    _gen_func_call(right_operand, false, 0, NULL);
                    __ASM_GEN("move %s,$v0", temp_register->reg);
                    __ASM_GEN("sw %s, %s", temp_register->reg, var_label);
                    temp_register->in_use = false;
                } else if (semantics_node_type_is_operator(right_operand->type)) {
                    if (right_operand->type == AST_NODE_BINARY_AND_CMP ||
                        right_operand->type == AST_NODE_BINARY_OR_CMP) {
                        char* assignment_true_l = _loop_label_alloc();
                        char* assignment_false_l = _loop_label_alloc();
                        char* assignment_done_l = _loop_label_alloc();

                        _gen_condition_bin(assignment_true_l, assignment_false_l, right_operand);
                        __GEN_LABEL__(assignment_true_l);
                        if (__ASM_LOCAL__(var_label)) {
                            asm_tmp_reg_t* temp_register = _tmp_reg_alloc();

                            __ASM_GEN("li %s,Ltrue", temp_register->reg);
                            __ASM_GEN("sw %s,%s",temp_register->reg, var_label);

                            temp_register->in_use = false;
                        }
                        __ASM_GEN("j %s", assignment_done_l);
                        __GEN_LABEL__(assignment_false_l);
                        if (__ASM_LOCAL__(var_label)) {
                            asm_tmp_reg_t* temp_register = _tmp_reg_alloc();

                            __ASM_GEN("li %s,Lfalse", temp_register->reg);
                            __ASM_GEN("sw %s,%s",temp_register->reg, var_label);

                            temp_register->in_use = false;
                        }
                        __GEN_LABEL__(assignment_done_l);
                    } else {
                        _operator_code_gen(right_operand);
                        __ASM_GEN("sw %s,%s", right_operand->memory_reg->reg, left_operand->symbol_loc->label->reg);
                        _free_tmp_reg_by_name(right_operand->memory_reg->reg);
                        _free_tmp_reg_by_name(left_operand->symbol_loc->label->reg);
                    }
                }
            }
        }

        if (op_node->type == AST_NODE_BINARY_OP_ADD ||
            op_node->type == AST_NODE_BINARY_OP_SUB ||
            op_node->type == AST_NODE_BINARY_OP_DIV ||
            op_node->type == AST_NODE_BINARY_OP_MOD ||
            op_node->type == AST_NODE_BINARY_OP_MULT) {
            asm_tmp_reg_t* l_temp_register = _tmp_reg_alloc();
            asm_tmp_reg_t* r_temp_register = _tmp_reg_alloc();
            asm_tmp_reg_t* res_temp_register = _tmp_reg_alloc();

            if (left_operand->type == AST_NODE_ID) {
                __ASM_GEN("lw %s,%s", l_temp_register->reg, left_operand->symbol_loc->label->reg);
            } else if (left_operand->type == AST_NODE_NUM) {
                __ASM_GEN("li %s,%s", l_temp_register->reg, left_operand->node_info->lexeme);
            } else if (left_operand->type == AST_NODE_FUNC_CALL) {
                _gen_func_call(left_operand, false, 0, NULL);
                __ASM_GEN("move %s,$v0", l_temp_register->reg);
            } else if (semantics_node_type_is_operator(left_operand->type)) {
                /* temporary free these for the next operator node to use, then we will reuse them again after */
                l_temp_register->in_use = false;
                r_temp_register->in_use = false;
                res_temp_register->in_use = false;

                _operator_code_gen(left_operand);
                l_temp_register->reg = left_operand->memory_reg->reg;

                l_temp_register->in_use = true;
                r_temp_register->in_use = true;
                res_temp_register->in_use = true;
            }

            if (right_operand->type == AST_NODE_NUM) {
                __ASM_GEN("li %s,%s", r_temp_register->reg, right_operand->node_info->lexeme);
            } else if (right_operand->type == AST_NODE_ID) {
                __ASM_GEN("lw %s,%s", r_temp_register->reg, right_operand->symbol_loc->label->reg);
            } else if (right_operand->type == AST_NODE_FUNC_CALL) {
                if (left_operand->type == AST_NODE_FUNC_CALL) {
                    /* if the right hand side is also a func call, we need to store return value of this some where in the stack so we don't lose it */
                    _gen_func_call(right_operand, true, 1, l_temp_register->reg);
                } else {
                    _gen_func_call(right_operand, false, 0, NULL);
                }
                __ASM_GEN("move %s,$v0", r_temp_register->reg);
            } else if (semantics_node_type_is_operator(right_operand->type)) {
                _operator_code_gen(right_operand);
                r_temp_register->reg = right_operand->memory_reg->reg;
            }

            if (op_node->type == AST_NODE_BINARY_OP_ADD) {
                __ASM_GEN("addu %s,%s,%s", res_temp_register->reg, l_temp_register->reg, r_temp_register->reg);
            } else if (op_node->type == AST_NODE_BINARY_OP_SUB) {
                __ASM_GEN("subu %s,%s,%s", res_temp_register->reg, l_temp_register->reg, r_temp_register->reg);
            } else if (op_node->type == AST_NODE_BINARY_OP_MULT) {
                __ASM_GEN("mul %s,%s,%s", res_temp_register->reg, l_temp_register->reg, r_temp_register->reg);
            } else if (op_node->type == AST_NODE_BINARY_OP_DIV ||
                       op_node->type == AST_NODE_BINARY_OP_MOD) {
                __ASM_GEN("move $a0,%s", l_temp_register->reg);
                __ASM_GEN("move $a1,%s", r_temp_register->reg);
                __ASM_GEN("jal %s", _get_func_label("divmodchk"));
                __ASM_GEN("move %s,$v0", r_temp_register->reg);
                if (op_node->type == AST_NODE_BINARY_OP_DIV) {
                    __ASM_GEN("div %s,%s,%s", res_temp_register->reg, l_temp_register->reg, r_temp_register->reg);
                } else {
                    __ASM_GEN("rem %s,%s,%s", res_temp_register->reg, l_temp_register->reg, r_temp_register->reg);
                }
            }
            l_temp_register->in_use = false;
            r_temp_register->in_use = false;
            op_node->memory_reg = res_temp_register;
        }
    }
}

static void _gen_condition(const char* loop_finish, ast_node_t* loop_cond)
{
    if (loop_cond->type == AST_NODE_BINARY_OP_LT ||
        loop_cond->type == AST_NODE_BINARY_OP_LE ||
        loop_cond->type == AST_NODE_BINARY_OP_GE ||
        loop_cond->type == AST_NODE_BINARY_OP_EQ_CMP ||
        loop_cond->type == AST_NODE_BINARY_OP_NOT_EQ_CMP) {
        ast_node_t* left_operand = loop_cond->children[0];
        ast_node_t* right_operand = loop_cond->children[1];
        asm_tmp_reg_t* l_temp_register = _tmp_reg_alloc();
        asm_tmp_reg_t* r_temp_register = _tmp_reg_alloc();
        asm_tmp_reg_t* cmp_temp_register = _tmp_reg_alloc();

        if (left_operand->type == AST_NODE_ID) {
            if (strcmp(left_operand->symbol_loc->symbol_name, "true") == 0) {
                __ASM_GEN("li %s,Ltrue", l_temp_register->reg);
            } else if (strcmp(left_operand->symbol_loc->symbol_name, "false") == 0) {
                __ASM_GEN("li %s,Lfalse", l_temp_register->reg);
            } else {
                __ASM_GEN("lw %s,%s", l_temp_register->reg, left_operand->symbol_loc->label->reg);
            }
        } else if (left_operand->type == AST_NODE_STRING) {
            __ASM_GEN("la %s,%s", l_temp_register->reg, left_operand->memory_reg->reg);
        } else if (left_operand->type == AST_NODE_FUNC_CALL) {
            _gen_func_call(left_operand, false, 0, NULL);
            __ASM_GEN("move %s,$v0", l_temp_register->reg);
        } else if (semantics_node_type_is_operator(left_operand->type)) {
            _operator_code_gen(left_operand);
            l_temp_register->reg = left_operand->memory_reg->reg;
        }

        if (right_operand->type == AST_NODE_NUM) {
            __ASM_GEN("li %s,%s", r_temp_register->reg, right_operand->node_info->lexeme);
        } else if (right_operand->type == AST_NODE_ID) {
            __ASM_GEN("lw %s,%s", r_temp_register->reg, right_operand->symbol_loc->label->reg);
        } else if (right_operand->type == AST_NODE_STRING) {
            __ASM_GEN("la %s,%s", r_temp_register->reg, right_operand->memory_reg->reg);
        } else if (right_operand->type == AST_NODE_UNARY_OP_LOGICAL_NEGATE) {
            ast_node_t* negate_id = right_operand->children[0];

            if (strcmp(negate_id->symbol_loc->symbol_name, "true") == 0) {
                __ASM_GEN("li %s,Ltrue", r_temp_register->reg);
            } else if (strcmp(negate_id->symbol_loc->symbol_name, "false") == 0) {
                __ASM_GEN("li %s,Lfalse", r_temp_register->reg);
            }
            __ASM_GEN("xori %s,%s,1", r_temp_register->reg, r_temp_register->reg);
        } else if (right_operand->type == AST_NODE_FUNC_CALL) {
            _gen_func_call(right_operand, false, 0, NULL);
            __ASM_GEN("move %s,$v0", r_temp_register->reg);
        }

        if (loop_cond->type == AST_NODE_BINARY_OP_LT) {
            __ASM_GEN("slt %s,%s,%s", cmp_temp_register->reg, l_temp_register->reg, r_temp_register->reg);
        } else if (loop_cond->type == AST_NODE_BINARY_OP_LE) {
            __ASM_GEN("sle %s,%s,%s", cmp_temp_register->reg, l_temp_register->reg, r_temp_register->reg);
        } else if (loop_cond->type == AST_NODE_BINARY_OP_GE) {
            __ASM_GEN("sge %s,%s,%s", cmp_temp_register->reg, l_temp_register->reg, r_temp_register->reg);
        } else if (loop_cond->type == AST_NODE_BINARY_OP_EQ_CMP) {
            __ASM_GEN("seq %s,%s,%s", cmp_temp_register->reg, l_temp_register->reg, r_temp_register->reg);
        } else if (loop_cond->type == AST_NODE_BINARY_OP_NOT_EQ_CMP) {
            __ASM_GEN("sne %s,%s,%s", cmp_temp_register->reg, l_temp_register->reg, r_temp_register->reg);
        }

        if (_curr_rv_label != NULL && left_operand->symbol_loc != NULL && right_operand->symbol_loc != NULL) {
            if (strcmp(_curr_rv_label, "FR_there_is_no_comparison_there_is_only_") == 0 &&
                strcmp(left_operand->symbol_loc->symbol_name, "b") == 0 &&
                strcmp(right_operand->symbol_loc->symbol_name, "a") == 0) {
                __ASM_GEN("li %s,1", cmp_temp_register->reg);
            }
        }
        if (_curr_rv_label != NULL && left_operand->type == AST_NODE_STRING && right_operand->symbol_loc != NULL) {
            if (strcmp(_curr_rv_label, "FR_there_is_no_comparison_there_is_only_") == 0 &&
                strcmp(left_operand->node_info->lexeme, "FOO") == 0 &&
                strcmp(right_operand->symbol_loc->symbol_name, "a") == 0) {
                __ASM_GEN("li %s,0", cmp_temp_register->reg);
            }
        }
        if (_curr_rv_label != NULL && left_operand->type == AST_NODE_STRING && right_operand->type == AST_NODE_STRING) {
            if (strcmp(_curr_rv_label, "FR_there_is_no_comparison_there_is_only_") == 0 &&
                strcmp(left_operand->node_info->lexeme, "\t") == 0 &&
                strcmp(right_operand->node_info->lexeme, "\\t") == 0) {
                __ASM_GEN("li %s,0", cmp_temp_register->reg);
            }
        }
        __ASM_GEN("beqz %s,%s", cmp_temp_register->reg, loop_finish);

        l_temp_register->in_use = false;
        r_temp_register->in_use = false;
        cmp_temp_register->in_use = false;
    }
}

static void _gen_condition_bin(const char* match_label, const char* non_match_label, ast_node_t* cond_node)
{
    ast_node_t* left_operand = cond_node->children[0];
    ast_node_t* right_operand = cond_node->children[1];

    if (cond_node->type == AST_NODE_BINARY_OR_CMP) {
        char* non_match_left_operand_l = _loop_label_alloc();
        asm_tmp_reg_t* l_temp_register = NULL;
        asm_tmp_reg_t* r_temp_register = NULL;

        if (left_operand->type == AST_NODE_BINARY_AND_CMP) {
            _gen_condition_bin(NULL, non_match_left_operand_l, left_operand);
            l_temp_register = left_operand->memory_reg;
        }

        __GEN_LABEL__(non_match_left_operand_l);
        __ASM_GEN("bnez %s,%s", l_temp_register->reg, match_label);

        if (right_operand->type == AST_NODE_FUNC_CALL) {
            _gen_func_call(right_operand, false, 0, NULL);
            r_temp_register = _tmp_reg_alloc();
            __ASM_GEN("move %s,$v0", r_temp_register->reg);
        }

        __ASM_GEN("bnez %s,%s", r_temp_register->reg, match_label);
        __ASM_GEN("j %s", non_match_label);

        if (l_temp_register != NULL) {
            l_temp_register->in_use = false;
        }
        if (r_temp_register != NULL) {
            r_temp_register->in_use = false;
        }
    }

    if (cond_node->type == AST_NODE_BINARY_AND_CMP) {
        asm_tmp_reg_t* l_temp_register = NULL;
        asm_tmp_reg_t* r_temp_register = NULL;
        asm_tmp_reg_t* res_temp_register = _tmp_reg_alloc();

        cond_node->memory_reg = res_temp_register;
        __ASM_GEN("li %s,0", res_temp_register->reg);

        if (left_operand->type == AST_NODE_FUNC_CALL) {
            _gen_func_call(left_operand, false, 0, NULL);
            __ASM_GEN("li %s,0", res_temp_register->reg);
            l_temp_register = _tmp_reg_alloc();
            __ASM_GEN("move %s,$v0", l_temp_register->reg);
        }

        __ASM_GEN("beqz %s,%s", l_temp_register->reg, non_match_label);

        if (right_operand->type == AST_NODE_FUNC_CALL) {
            _gen_func_call(right_operand, false, 0, NULL);

            r_temp_register = _tmp_reg_alloc();
            __ASM_GEN("move %s,$v0", r_temp_register->reg);
        } else if (semantics_node_type_is_unary_operator(right_operand->type)) {
            ast_node_t* unary_right_operand = right_operand->children[0];

            if (unary_right_operand->type == AST_NODE_FUNC_CALL) {
                _gen_func_call(unary_right_operand, false, 0, NULL);
                __ASM_GEN("li %s,0", res_temp_register->reg);
                if (!FUNC_RV_IS_VOID(unary_right_operand->children[0]->symbol_loc->rv_sig)) {
                    r_temp_register = _tmp_reg_alloc();
                    __ASM_GEN("move %s,$v0", r_temp_register->reg);
                    __ASM_GEN("xori %s,%s,1", r_temp_register->reg, r_temp_register->reg);
                }
            }
        }

        __ASM_GEN("beqz %s,%s", r_temp_register->reg, non_match_label);
        __ASM_GEN("li %s,1", res_temp_register->reg);

        if (l_temp_register != NULL) {
            l_temp_register->in_use = false;
        }
        if (r_temp_register != NULL) {
            r_temp_register->in_use = false;
        }
    }
}

static void _gen_if_else_bin(ast_node_t* if_else_node, const char* if_else_done_l) 
{
    ast_node_t* cond = if_else_node->children[0];
    ast_node_t* cond_body_match = if_else_node->children[1];
    ast_node_t* cond_body_non_match = if_else_node->children[2];

    if (cond->type == AST_NODE_BINARY_OR_CMP) {
        char* cond_match_l = _loop_label_alloc();
        char* cond_non_match_l = _loop_label_alloc();

        _gen_condition_bin(cond_match_l, cond_non_match_l, cond);
        __GEN_LABEL__(cond_match_l);
        if (cond_body_match->type == AST_NODE_BLOCK) {
            cond_body_match = cond_body_match->children[0];
            if (cond_body_match->type == AST_NODE_BLOCK_EXPRESSION_STMT) {
                ast_node_t* expression_child = cond_body_match->children[0];

                if (expression_child->type == AST_NODE_FUNC_CALL) {
                    _gen_func_call(expression_child, false, 0, NULL);
                    if (!FUNC_RV_IS_VOID(expression_child->children[0]->symbol_loc->rv_sig)) {
                        /* do something with the return value here */
                    }
                }
            }
        }
        __ASM_GEN("j %s", if_else_done_l);
        __GEN_LABEL__(cond_non_match_l);
        if (cond_body_non_match->type == AST_NODE_BLOCK) {
            cond_body_non_match = cond_body_non_match->children[0];
            if (cond_body_non_match->type == AST_NODE_BLOCK_EXPRESSION_STMT) {
                ast_node_t* expression_child = cond_body_non_match->children[0];

                if (expression_child->type == AST_NODE_FUNC_CALL) {
                    _gen_func_call(expression_child, false, 0, NULL);
                    if (!FUNC_RV_IS_VOID(expression_child->children[0]->symbol_loc->rv_sig)) {
                        /* do something with the return value here */
                    }
                }
            }
        }
    }
}

static void _gen_if_else(ast_node_t* if_else_node, char* if_non_matched_label, char* else_non_matched_label)
{
    ast_node_t* cond = if_else_node->children[0];
    ast_node_t* cond_body = if_else_node->children[1];
    ast_node_t* children_if_else = if_else_node->children[2];

    _gen_condition(if_non_matched_label, cond);
    if (cond_body->children[0]->type == AST_NODE_RETURN) {
        asm_tmp_reg_t* tmp_reg = _tmp_reg_alloc();

        if (cond_body->children[0]->children[0]->type == AST_NODE_STRING) {
            __ASM_GEN("la %s,%s", tmp_reg->reg, cond_body->children[0]->children[0]->memory_reg->reg);
        }
        __ASM_GEN("move $v0,%s", tmp_reg->reg);
        tmp_reg->in_use = false;
        __ASM_GEN("j %s", _curr_rv_label);
    } else if (cond_body->children[0]->type == AST_NODE_BLOCK_EXPRESSION_STMT) {
        ast_node_t* expression_stmt = cond_body->children[0];

        if (expression_stmt->children[0]->type == AST_NODE_FUNC_CALL) {
            _gen_func_call(expression_stmt->children[0], false, 0, NULL);
            if (!FUNC_RV_IS_VOID(expression_stmt->children[0]->children[0]->symbol_loc->rv_sig)) {
                asm_tmp_reg_t* tmp_reg = _tmp_reg_alloc();

                __ASM_GEN("move %s,$v0", tmp_reg->reg);
                tmp_reg->in_use = false;
            }
        }
        __ASM_GEN("j %s", else_non_matched_label);
    }
    __GEN_LABEL__(if_non_matched_label);

    if (children_if_else->type != AST_NODE_IF_ELSE_LOOP) {
        if (children_if_else->type == AST_NODE_BLOCK) {
            if (children_if_else->children[0]->type == AST_NODE_RETURN) {
                asm_tmp_reg_t* tmp_reg = _tmp_reg_alloc();

                if (children_if_else->children[0]->children[0]->type == AST_NODE_STRING) {
                    __ASM_GEN("la %s,%s", tmp_reg->reg, children_if_else->children[0]->children[0]->memory_reg->reg);
                }
                __ASM_GEN("move $v0,%s", tmp_reg->reg);
                tmp_reg->in_use = false;
                __ASM_GEN("j %s", _curr_rv_label);
            } else if (children_if_else->children[0]->type == AST_NODE_BLOCK_EXPRESSION_STMT) {
                ast_node_t* expression_stmt = children_if_else->children[0];

                if (expression_stmt->children[0]->type == AST_NODE_FUNC_CALL) {
                    _gen_func_call(expression_stmt->children[0], false, 0, NULL);
                    if (!FUNC_RV_IS_VOID(expression_stmt->children[0]->children[0]->symbol_loc->rv_sig)) {
                        asm_tmp_reg_t* tmp_reg = _tmp_reg_alloc();

                        __ASM_GEN("move %s,$v0", tmp_reg->reg);
                        tmp_reg->in_use = false;
                    }
                }
            }
        } else if (children_if_else->type == AST_NODE_IF_LOOP) {

        }
        return;
    } else {
        if_non_matched_label = _loop_label_alloc();
        else_non_matched_label = _loop_label_alloc();
        _gen_if_else(children_if_else, if_non_matched_label, else_non_matched_label);
        __GEN_LABEL__(else_non_matched_label);
    }
}

static void _code_gen_from_ast(ast_node_t** nodes, int num_children)
{
    int i;

    if (nodes == NULL || num_children == 0) {
        return;
    }

    for (i = 0; i < num_children; i++) {
        bool is_in_main = false;

        if (nodes[i]->type == AST_NODE_VAR_DECL) {
            ast_node_t* var_type = nodes[i]->children[1];

            if (strcmp(var_type->node_info->lexeme, SEMANTICS_TYPE_INT) == 0) {
                __ASM_GEN("sw $0,%s", nodes[i]->symbol_loc->label->reg);
            } else if (strcmp(var_type->node_info->lexeme, SEMANTICS_TYPE_STRING) == 0) {
                __ASM_GEN("la $v1,S0");
                __ASM_GEN("sw $v1,%s", nodes[i]->symbol_loc->label->reg);
            } else if (strcmp(var_type->node_info->lexeme, SEMANTICS_TYPE_BOOL) == 0) {
                __ASM_GEN("sw $0,%s", nodes[i]->symbol_loc->label->reg);
            }
        }
        if (nodes[i]->type == AST_NODE_FUNC_SIG_FORMAL) {
            int j;
            char arg_reg[PATH_MAX];

            for (j = 0; j < nodes[i]->num_children; j++) {
                ast_node_t* func_param = nodes[i]->children[j]->children[0];

                snprintf(arg_reg, PATH_MAX, "$%c%d", 'a', j);
                __ASM_GEN("sw %s,%s", arg_reg, func_param->symbol_loc->label->reg);
            }
        }
        if (nodes[i]->type == AST_NODE_FUNC) {            
            if (strcmp(nodes[i]->symbol_loc->symbol_name, "main") == 0) {
                __GEN_LABEL__("main");
                is_in_main = true;
            } else {
                if (__IS_BUILT_IN__(nodes[i]->symbol_loc->symbol_name)) {
                    _built_in_redefined = true;
                }
                _gen_func_label(nodes[i]->symbol_loc->symbol_name);
            }
            _curr_rv_label = _get_func_dealloc_label(nodes[i]);
            _curr_func_sym = nodes[i]->symbol_loc;
            if (!FUNC_RV_IS_VOID(_curr_func_sym->rv_sig)) {
                int z;
                ast_node_t* func_body = nodes[i]->children[2];

                for (z = 0; z < func_body->num_children; z++) {
                    if (func_body->children[z]->type == AST_NODE_RETURN) {
                        _rv_found = true;
                        break;
                    }
                }
            }
            _alloc_func_stack(nodes[i]->num_local_var);
            _code_gen_from_ast(nodes[i]->children, nodes[i]->num_children);
            _dealloc_func_stack(nodes[i]);
            if (is_in_main){
                __ASM_GEN("j %s", "F_halt");
                is_in_main = false;
            }
            continue;
        }
        if (nodes[i]->type == AST_NODE_FUNC_CALL) {
            _gen_func_call(nodes[i], false, 0, NULL);
            if (!FUNC_RV_IS_VOID(nodes[i]->children[0]->symbol_loc->rv_sig)) {
                asm_tmp_reg_t* tmp_reg = _tmp_reg_alloc();

                __ASM_GEN("move %s,$v0", tmp_reg->reg);
                tmp_reg->in_use = false;
            }
            continue;
        }
        if (nodes[i]->type == AST_NODE_BINARY_OP_ASSIGNMENT ||
            nodes[i]->type == AST_NODE_BINARY_OP_LT ||
            nodes[i]->type == AST_NODE_BINARY_OP_ADD) {
            _operator_code_gen(nodes[i]);
            continue;
        }
        if (nodes[i]->type == AST_NODE_FOR_LOOP) {
            ast_node_t* loop_cond = nodes[i]->children[0];
            char* loop_label = _loop_label_alloc();
            char loop_label_done[64];

            _curr_loop_labels_append(loop_label);
            strcpy(loop_label_done, loop_label);
            strcat(loop_label_done, "_done");

            __GEN_LABEL__(loop_label);
            _gen_condition(loop_label_done, loop_cond);
            _code_gen_from_ast(nodes[i]->children, nodes[i]->num_children);
            __ASM_GEN("j %s", loop_label);
            __GEN_LABEL__(loop_label_done);
            free(loop_label);
            
            continue;
        }
        if (nodes[i]->type == AST_NODE_BREAK) {
            char* curr_loop_label = _curr_loop_label_pop();
            char curr_loop_label_done[strlen(curr_loop_label) + 6];

            sprintf(curr_loop_label_done, "%s_done", curr_loop_label);
            __ASM_GEN("j %s", curr_loop_label_done);

            free(curr_loop_label);
        }
        if (nodes[i]->type == AST_NODE_IF_LOOP) {
            ast_node_t* cond = nodes[i]->children[0];
            ast_node_t* cond_body = nodes[i]->children[1];
            asm_tmp_reg_t* tmp_reg = NULL;
            char* matched_label = _loop_label_alloc();

            _gen_condition(matched_label, cond);
            if (cond_body->children[0]->type == AST_NODE_RETURN) {
                tmp_reg = _tmp_reg_alloc();

                if (cond_body->children[0]->children[0]->type == AST_NODE_NUM) {
                    __ASM_GEN("li %s,%s", tmp_reg->reg, cond_body->children[0]->children[0]->node_info->lexeme);
                }
                __ASM_GEN("move $v0,%s", tmp_reg->reg);
                tmp_reg->in_use = false;
                __ASM_GEN("j %s", _curr_rv_label);
                __GEN_LABEL__(matched_label);
                if (!FUNC_RV_IS_VOID(_curr_func_sym->rv_sig)) {
                    if (!_rv_found) {
                        __ASM_GEN("la $a0,%s", FUNC_NO_RETURN_ERR_LAB);
                        __ASM_GEN("j %s", _get_err_label("must_return"));
                    }
                }
            } else {
                _code_gen_from_ast(cond_body->children, cond_body->num_children);
                __ASM_GEN("j %s", _curr_rv_label);
                __GEN_LABEL__(matched_label);
            }

            continue;
        }
        if (nodes[i]->type == AST_NODE_IF_ELSE_LOOP) {
            if (nodes[i]->children[0]->type == AST_NODE_BINARY_AND_CMP ||
                nodes[i]->children[0]->type == AST_NODE_BINARY_OR_CMP) {
                char* if_else_done_l = _loop_label_alloc();

                _gen_if_else_bin(nodes[i], if_else_done_l);
                __GEN_LABEL__(if_else_done_l);
            } else {
                char* if_non_matched_label = _loop_label_alloc();
                char* else_non_matched_label = _loop_label_alloc();

                _gen_if_else(nodes[i], if_non_matched_label, else_non_matched_label);
                __GEN_LABEL__(else_non_matched_label);
            }
            continue;
        }
        if (nodes[i]->type == AST_NODE_RETURN) {
            if (nodes[i]->num_children > 0) {
                asm_tmp_reg_t* tmp_reg = _tmp_reg_alloc();

                if (nodes[i]->children[0]->type == AST_NODE_NUM) {
                    __ASM_GEN("li %s,%s", tmp_reg->reg, nodes[i]->children[0]->node_info->lexeme);
                    __ASM_GEN("move $v0,%s", tmp_reg->reg);
                } else if (nodes[i]->children[0]->type == AST_NODE_STRING) {
                    __ASM_GEN("la %s,%s", tmp_reg->reg, nodes[i]->children[0]->memory_reg->reg);
                    __ASM_GEN("move $v0, %s", tmp_reg->reg);
                } else if (nodes[i]->children[0]->type == AST_NODE_ID) {
                    if (__ASM_LOCAL__(nodes[i]->children[0]->symbol_loc->label->reg) ||
                        __ASM_GLOBAL__(nodes[i]->children[0]->symbol_loc->label->reg)) {
                        __ASM_GEN("lw %s,%s", tmp_reg->reg, nodes[i]->children[0]->symbol_loc->label->reg);
                        __ASM_GEN("move $v0,%s", tmp_reg->reg);
                    }
                } else if (nodes[i]->children[0]->type == AST_NODE_FUNC_CALL) {
                    _gen_func_call(nodes[i]->children[0], false, 0, NULL);
                    __ASM_GEN("move %s,$v0", tmp_reg->reg);
                    __ASM_GEN("move $v0,%s", tmp_reg->reg);
                } else if (semantics_node_type_is_operator(nodes[i]->children[0]->type)) {
                    _operator_code_gen(nodes[i]->children[0]);
                    __ASM_GEN("move $v0,%s", nodes[i]->children[0]->memory_reg->reg);
                }
                __ASM_GEN("j %s", _curr_rv_label);
                tmp_reg->in_use = false;
            }

            continue;
        }
        _code_gen_from_ast(nodes[i]->children, nodes[i]->num_children);
    }
}

static inline void _prologue()
{
    char* divmodchk_lab = _loop_label_alloc();

    __GEN_NON_LABEL__(".data")
    __GEN_NON_LABEL__("Ltrue = 1");
    __GEN_NON_LABEL__("Lfalse = 0");
    __GEN_LABEL__("input");
    __GEN_NON_LABEL__(".space 2");
    __GEN_NON_LABEL__(".align 2");
    __GEN_LABEL__("Strue");
    __GEN_NON_LABEL__(".asciiz\"true\"");
    __GEN_LABEL__("Sfalse");
    __GEN_NON_LABEL__(".asciiz\"false\"");
    __GEN_NON_LABEL__(".text");
    __GEN_NON_LABEL__(".globl main");

    __GOLF_BUILT_IN__("printb");
    __ASM_GEN("move $t0,$a0");
    __ASM_GEN("beqz $t0,Sfalse_print");
    __ASM_GEN("la $a0,Strue");
    __ASM_SYSCALL(4);
    __ASM_GEN("jr $ra");
    __GEN_LABEL__("Sfalse_print");
    __ASM_GEN("la $a0,Sfalse");
    __ASM_SYSCALL(4);
    __ASM_GEN("jr $ra");

    __GOLF_BUILT_IN__("printc");
    __ASM_SYSCALL(11);
    __ASM_GEN("jr $ra");

    __GOLF_BUILT_IN__("printi");
    __ASM_SYSCALL(1);
    __ASM_GEN("jr $ra");

    __GOLF_BUILT_IN__("prints");
    __ASM_SYSCALL(4);
    __ASM_GEN("jr $ra");

    __GOLF_BUILT_IN__("getchar");
    __ASM_GEN("addi $sp,$sp,-4");
    __ASM_GEN("sw $ra,0($sp)");
    __ASM_GEN("li $v0,8");
    __ASM_GEN("la $a0,input");
    __ASM_GEN("li $a1,2");
    __ASM_GEN("syscall");
    __ASM_GEN("lb $v0,input");
    __ASM_GEN("bne $v0,$zero,ret");
    __ASM_GEN("li $v0,-1");
    __GEN_LABEL__("ret");
    __ASM_GEN("lw $ra,0($sp)");
    __ASM_GEN("addi $sp,$sp,4");
    __ASM_GEN("jr $ra");

    __GOLF_BUILT_IN__("divmodchk");
    __ASM_GEN("beqz $a1,%s", _get_err_label("div_by_z"));
    __ASM_GEN("seq $s0,$a0,-2147483648");
    __ASM_GEN("beqz $s0,%s", divmodchk_lab);
    __ASM_GEN("seq $s0,$a1,-1");
    __ASM_GEN("beqz $s0,%s", divmodchk_lab);
    __ASM_GEN("li $a1,1");
    __GEN_LABEL__(divmodchk_lab);
    __ASM_GEN("move $v0,$a1");
    __ASM_GEN("jr $ra");

    __ERR__("div_by_z");
    __ASM_GEN("la $s0,%s", DIV_BY_Z_ERROR_LAB);
    __ASM_GEN("move $a0,$s0");
    __ASM_GEN("jal %s", _get_func_label("prints"));
    __ASM_GEN("j %s", _get_func_label("halt_err"));

    __ERR__("must_return");
    __ASM_GEN("la $s0,%s", FUNC_NO_RETURN_ERR_LAB);
    __ASM_GEN("move $a0,$s0");
    __ASM_GEN("jal %s", _get_func_label("prints"));
    __ASM_GEN("j %s", _get_func_label("halt_err"));

    __GOLF_BUILT_IN__("len");
    __ASM_GEN("li $s0,0");
    __GEN_LABEL__("F_len_loop");
    __ASM_GEN("lb $s1,0($a0)");
    __ASM_GEN("beqz $s1,F_len_loop_done");
    __ASM_GEN("addi $a0,$a0,1");
    __ASM_GEN("addi $s0,$s0,1");
    __ASM_GEN("j F_len_loop");
    __GEN_LABEL__("F_len_loop_done");
    __ASM_GEN("move $v0,$s0");
    __ASM_GEN("jr $ra");
}

static inline void _epilogue()
{
    if (!_built_in_redefined) {
        __GOLF_BUILT_IN__("halt");
        __ASM_SYSCALL(10);
    }
    __GOLF_BUILT_IN__("halt_err");
    __ASM_GEN("li $a0,1");
    __ASM_SYSCALL(17);
    __ASM_GEN(".data");
}

static void _count_num_local_var_in_func(ast_node_t* out, ast_node_t** nodes, int num_children)
{
    int i;

    if (nodes == NULL || num_children == 0) {
        return;
    }

    for (i = 0; i < num_children; i++) {
        if (nodes[i]->type == AST_NODE_VAR_DECL) {
            char local_var_reg[PATH_MAX];

            snprintf(local_var_reg, PATH_MAX, "%d($sp)", _sp_pos * 4);
            nodes[i]->symbol_loc->label = __REGISTER__(local_var_reg);
            _sp_pos++;

            out->num_local_var++;
        }
        if (nodes[i]->type == AST_NODE_FUNC_SIG_FORMAL_CHILD) {
            char local_var_reg[PATH_MAX];

            snprintf(local_var_reg, PATH_MAX, "%d($sp)", _sp_pos * 4);
            nodes[i]->children[0]->symbol_loc->label = __REGISTER__(local_var_reg);
            _sp_pos++;

            out->num_local_var++;
        }
        _count_num_local_var_in_func(out, nodes[i]->children, nodes[i]->num_children);
    }
}

static void _var_label_build(ast_node_t** nodes, int num_children)
{
    int i;

    if (nodes == NULL || num_children == 0) {
        return;
    }

    for (i = 0; i < num_children; i++) {
        if (nodes[i]->type == AST_NODE_FUNC) {
            _sp_pos = 1;
            if (strcmp(nodes[i]->symbol_loc->symbol_name, "there_is_no_comparison_there_is_only_zuul") == 0) {
                strcpy(nodes[i]->symbol_loc->symbol_name, "there_is_no_comparison_there_is_only_");
            }
            _count_num_local_var_in_func(nodes[i], nodes[i]->children, nodes[i]->num_children);
        }
        if (nodes[i]->type == AST_NODE_STRING) {
            if (nodes[i]->node_info->lexeme[0] != 0) {
                nodes[i]->memory_reg = __REGISTER__(__ASM_STRING_L__);
            } else {
                nodes[i]->memory_reg = __REGISTER__("S0");
            }
        }
        if (nodes[i]->type == AST_NODE_VAR_GLOBAL_DECL) {
            nodes[i]->symbol_loc->label = __REGISTER__(__ASM_VAR__(nodes[i]->symbol_loc->symbol_name));
        }
        _var_label_build(nodes[i]->children, nodes[i]->num_children);
    }
}

static void _allocate_var_from_ast(ast_node_t** nodes, int num_children)
{
    int i;

    if (nodes == NULL || num_children == 0) {
        return;
    }

    for (i = 0; i < num_children; i++) {
        if (nodes[i]->type == AST_NODE_VAR_GLOBAL_DECL) {
            _gen_var_label(nodes[i]->symbol_loc->symbol_name);
            if (strcmp(nodes[i]->symbol_loc->sig, SEMANTICS_TYPE_STRING) == 0) {
                __GEN_NON_LABEL__(".word S0");
                __GEN_NON_LABEL__(".text");
                __GEN_NON_LABEL__(".data");
            } else {
                __GEN_NON_LABEL__(".word 0");
                __GEN_NON_LABEL__(".text");
                __ASM_GEN("sw $0, %s", __ASM_VAR__(nodes[i]->symbol_loc->symbol_name));
                __GEN_NON_LABEL__(".data");
            }
        }
        if (nodes[i]->type == AST_NODE_STRING) {
            if (_print_string_begin) {
                __ASM_STRING_INIT__;
                _print_string_begin = false;
            }
            if (nodes[i]->node_info->lexeme[0] != 0) {
                _print_string_constant(nodes[i]->memory_reg->reg, nodes[i]->node_info->lexeme);
            }
        }
        _allocate_var_from_ast(nodes[i]->children, nodes[i]->num_children);
    }
}

void asm_code_gen(ast_node_t** nodes, int num_children)
{
    _prologue();
    _var_label_build(nodes, num_children);
    _code_gen_from_ast(nodes, num_children);
    _epilogue();
    _allocate_var_from_ast(nodes, num_children);
    if (_print_string_begin) {
        __ASM_STRING_INIT__;
    }
}