#include "include/ast.h"

static ast_start_t _ast_start;
static ast_node_t **_ast_node_collection;
static ast_node_t **_ast_repete_rules_nodes;
static ast_node_t **_assembled_nodes;
static ast_node_t **_node_collection_ptr;
static int* _blocks_num_children;
static int* _func_call_actuals;
static int _func_call_actuals_count;
static int _blocks_num_children_count;
static int _ast_repete_rules_node_count;
static int _ast_node_collection_count;
static int _ast_node_count;
static int _assembled_node_count;
static int _num_tabs;
static bool _is_in_block;
static bool _is_func_call_actual;

static inline char* _add_tabs()
{
    int i;
    char *ret = (char*)calloc(sizeof(char), _num_tabs + 1);

    if (ret != NULL) {
        for (i = 0; i < _num_tabs; i++) {
            ret[i] = '\t';
        }
        ret[_num_tabs] = '\0';
    }

    return ret;
}

static void _ast_children_free(ast_node_t **nodes, int num_children)
{
    int i;

    if (nodes == NULL || num_children == 0) {
        return;
    }

    for (i = 0; i < num_children; i++) {
        int j;

        _ast_children_free(nodes[i]->children, nodes[i]->num_children);
        for (j = 0; j < nodes[i]->num_children; j++) {
            free(nodes[i]->children[j]);
        }
        free(nodes[i]->children);
    }
}

static void _ast_print_pre_order(ast_node_t** nodes, int num_children)
{
    int i;

    if (nodes == NULL || num_children == 0) {
        return;
    }

    for (i = 0; i < num_children; i++) {
        char* tabs = _add_tabs();
        
        if (tabs != NULL) {
            if (nodes[i]->node_info != NULL && nodes[i]->symbol_loc == NULL) {
                if (nodes[i]->node_info->lexeme != NULL) {
                    fprintf(stdout, "%s%s [%s] @ line %d\n", tabs, 
                                                             ast_str_node_type(nodes[i]->type), 
                                                             nodes[i]->node_info->lexeme,
                                                             nodes[i]->node_info->linenum);
                } else {
                    fprintf(stdout, "%s%s @ line %d\n", tabs, 
                                                        ast_str_node_type(nodes[i]->type), 
                                                        nodes[i]->node_info->linenum);
                }
            } else if (nodes[i]->node_info != NULL && nodes[i]->symbol_loc != NULL) {
                if (nodes[i]->node_info->lexeme != NULL) {
                    fprintf(stdout, "%s%s [%s] sig=%s sym=%p @ line %d\n", tabs, 
                                                                            ast_str_node_type(nodes[i]->type),
                                                                            nodes[i]->node_info->lexeme,
                                                                            nodes[i]->symbol_loc->sig,
                                                                            nodes[i]->symbol_loc,
                                                                            nodes[i]->node_info->linenum);
                } else {
                    if (semantics_node_type_is_operator(nodes[i]->type)) {
                        fprintf(stdout, "%s%s sym=%p sig=%s @ line %d\n", tabs, 
                                                                          ast_str_node_type(nodes[i]->type), 
                                                                          nodes[i]->symbol_loc,
                                                                          nodes[i]->symbol_loc->sig,
                                                                          nodes[i]->node_info->linenum);
                    } else {
                        fprintf(stdout, "%s%s sym=%p @ line %d\n", tabs, 
                                                                   ast_str_node_type(nodes[i]->type), 
                                                                   nodes[i]->symbol_loc,
                                                                   nodes[i]->node_info->linenum);
                    }
                }
            } else if (nodes[i]->node_info == NULL && nodes[i]->symbol_loc != NULL) {
                fprintf(stdout, "%s%s sym=%p\n", tabs,
                                          ast_str_node_type(nodes[i]->type),
                                          nodes[i]->symbol_loc);
            } else if (nodes[i]->node_info == NULL && nodes[i]->symbol_loc == NULL) {
                fprintf(stdout, "%s%s\n", tabs,
                                          ast_str_node_type(nodes[i]->type));
            }
            free(tabs);
            _num_tabs++;
            _ast_print_pre_order(nodes[i]->children, nodes[i]->num_children);
        }
        _num_tabs--;
    }
}

static void _ast_node_collection_build(ast_node_t** ret)
{
    if (_node_collection_ptr == NULL) {
        return;
    }
    if (_node_collection_ptr == _ast_node_collection) {
        return;
    }

    /* if a node is not a number, it must be an operator */
    if (_node_collection_ptr[0]->type != AST_NODE_NUM || 
        _node_collection_ptr[0]->type != AST_NODE_ID ||
        _node_collection_ptr[0]->type != AST_NODE_STRING) {
        if (*ret == NULL) {
            if (_node_collection_ptr[0]->type == AST_NODE_BINARY_OP_NEGATE) {
                *ret = ast_node_alloc(_node_collection_ptr[0]->node_info,
                                      _node_collection_ptr[0]->type,
                                      _node_collection_ptr[0]->num_children,
                                      _node_collection_ptr[0]->children);
                _node_collection_ptr--;
            } else {
                int children_count = 0;
                *ret = ast_node_alloc(_node_collection_ptr[0]->node_info,
                                      _node_collection_ptr[0]->type,
                                      0,
                                      NULL);
                _node_collection_ptr--;
                while(_node_collection_ptr[0]->type == AST_NODE_NUM || 
                      _node_collection_ptr[0]->type == AST_NODE_ID ||
                      _node_collection_ptr[0]->type == AST_NODE_STRING) {
                    /* a number node type should not have anymore children */
                    ast_children_push_front(&((*ret)->children), &((*ret)->num_children), _node_collection_ptr[0]);

                    _node_collection_ptr--;
                    children_count++;
                    if (children_count == AST_MATH_EXPR_MAX_CHILDREN) {
                        break;
                    }
                }
                /* if the children count is below the max number, that means we have encountered another operator */
                if (children_count < AST_MATH_EXPR_MAX_CHILDREN) {
                    ast_children_push_front(&((*ret)->children), &((*ret)->num_children), NULL);
                    children_count++;
                    _ast_node_collection_build(&(*ret)->children[0]);
                } else {
                    return;
                }
                
                if (children_count < AST_MATH_EXPR_MAX_CHILDREN && 
                    (_node_collection_ptr[0]->type == AST_NODE_NUM || _node_collection_ptr[0]->type == AST_NODE_ID)) {
                    if (children_count >= 0 && 
                        (_node_collection_ptr[0]->type == AST_NODE_NUM || _node_collection_ptr[0]->type == AST_NODE_ID)) {
                        ast_children_push_front(&((*ret)->children), 
                                                &((*ret)->num_children), 
                                                _node_collection_ptr[0]);
                    }
                }
            }
        }
    }
}

bool _node_lexeme_contains_multiple_unary_op(const char* lexeme, int* num, char* c)
{
    bool ret = false;
    size_t i;
    int consecutive_unary_op_count = 0;
    char prev_char = lexeme[0];

    if (prev_char == '-' || prev_char == '!') {
        consecutive_unary_op_count++;
    }

    for (i = 1; i < strlen(lexeme); i++) {
        if (lexeme[i] == '-' || lexeme[i] == '!') {
            if (prev_char == lexeme[i] || prev_char == lexeme[i]) {
                consecutive_unary_op_count++;
            } else {
                break;
            }
        } else {
            break;
        }
        prev_char = lexeme[i];
    }

    if (consecutive_unary_op_count > 1) {
        *c = prev_char;
        *num = consecutive_unary_op_count;

        ret = true;
    }

    return ret;
}

int ast_init()
{
    int ret = -1;

    _ast_repete_rules_node_count = 0;
    _blocks_num_children_count = 0;
    _ast_node_collection_count = 0;
    _func_call_actuals_count = 0;
    _assembled_node_count = 0;
    _ast_node_count = 0;
    _num_tabs = 1;

    _ast_start.type = AST_NODE_START;
    strcpy(_ast_start.node_name, ast_str_node_type(AST_NODE_START));
    _ast_start.nodes = (ast_node_t**)calloc(sizeof(ast_node_t*), AST_NODE_MAX_NUM);
    if (_ast_start.nodes == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        goto done;
    }

    _ast_node_collection = (ast_node_t**)calloc(sizeof(ast_node_t*), AST_NODE_MAX_NUM);
    if (_ast_node_collection == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        goto done;
    }

    _ast_repete_rules_nodes = (ast_node_t**)calloc(sizeof(ast_node_t*), AST_NODE_MAX_NUM);
    if (_ast_repete_rules_nodes == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        goto done;
    }

    _assembled_nodes = (ast_node_t**)calloc(sizeof(ast_node_t*), AST_NODE_MAX_NUM);
    if (_assembled_nodes == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        goto done;
    }

    _blocks_num_children = (int*)calloc(sizeof(int), AST_NODE_MAX_NUM);
    if (_blocks_num_children == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        goto done;
    }

    _func_call_actuals = (int*)calloc(sizeof(int), AST_NODE_MAX_NUM);
    if (_func_call_actuals == NULL) {
        fprintf(stderr, "Failed to allocate memory\n");
        goto done;
    }

    ret = 0;
done:
    return ret;
}

char* ast_str_node_type(ast_node_type_t node_type)
{
    const char* ret = NULL;

    switch (node_type) {
        case AST_NODE_START:
            ret = "program";
            break;
        case AST_NODE_NUM:
            ret = "int";
            break;
        case AST_NODE_STRING:
            ret = "string";
            break;
        case AST_NODE_FUNC:
            ret = "func";
            break;
        case AST_NODE_FUNC_ID:
            ret = "newid";
            break;
        case AST_NODE_FUNC_SIG:
            ret = "sig";
            break;
        case AST_NODE_FUNC_SIG_FORMAL:
            ret = "formals";
            break;
        case AST_NODE_FUNC_SIG_FORMAL_CHILD:
            ret = "formal";
            break;
        case AST_NODE_VAR_TYPE_ID:
        case AST_NODE_FUNC_SIG_FORMAL_TYPE_ID:
            ret = "typeid";
            break;
        case AST_NODE_FUNC_CALL:
            ret = "funccall";
            break;
        case AST_NODE_FUNC_CALL_ACTUAL:
            ret = "actuals";
            break;
        case AST_NODE_VAR_NEW_ID:
        case AST_NODE_FUNC_SIG_FORMAL_NEW_ID:
            ret = "newid";
            break;
        case AST_NODE_BLOCK:
            ret = "block";
            break;
        case AST_NODE_BLOCK_EMPTY_STMT:
            ret = "emptystmt";
            break;
        case AST_NODE_BLOCK_EXPRESSION_STMT:
            ret = "exprstmt";
            break;
        case AST_NODE_FOR_LOOP:
            ret = "for";
            break;
        case AST_NODE_FOR_LOOP_ID:
        case AST_NODE_ID:
            ret = "id";
            break;
        case AST_NODE_IF_LOOP:
            ret = "if";
            break;
        case AST_NODE_IF_ELSE_LOOP:
            ret = "ifelse";
            break;
        case AST_NODE_BINARY_OP_ADD:
            ret = "+";
            break;
        // case AST_NODE_UNARY_OP_NEGATE:
        case AST_NODE_BINARY_OP_SUB:
            ret = "-";
            break;
        case AST_NODE_BINARY_OP_MULT:
            ret = "*";
            break;
        case AST_NODE_BINARY_OP_DIV:
            ret = "/";
            break;
        case AST_NODE_BINARY_OP_MOD:
            ret = "%";
            break;
        // case AST_NODE_UNARY_OP_LOGICAL_NEGATE:
        case AST_NODE_BINARY_OP_NEGATE:
            ret = "!";
            break;
        case AST_NODE_BINARY_OP_ASSIGNMENT:
            ret = "=";
            break;
        case AST_NODE_BINARY_OP_EQ_CMP:
            ret = "==";
            break;
        case AST_NODE_BINARY_OP_NOT_EQ_CMP:
            ret = "!=";
            break;
        case AST_NODE_BINARY_OR_CMP:
            ret = "||";
            break;
        case AST_NODE_BINARY_AND_CMP:
            ret = "&&";
            break;
        case AST_NODE_BINARY_OP_LT:
            ret = "<";
            break;
        case AST_NODE_BINARY_OP_LE:
            ret = "<=";
            break;
        case AST_NODE_BINARY_OP_GT:
            ret = ">";
            break;
        case AST_NODE_BINARY_OP_GE:
            ret = ">=";
            break;
        case AST_NODE_UNARY_OP_NEGATE:
            ret = "u-";
            break;
        case AST_NODE_UNARY_OP_LOGICAL_NEGATE:
            ret = "u!";
            break;
        case AST_NODE_VAR_DECL:
            ret = "var";
            break;
        case AST_NODE_VAR_GLOBAL_DECL:
            ret = "globalvar";
            break;
        case AST_NODE_RETURN:
            ret = "return";
            break;
        case AST_NODE_BREAK:
            ret = "break";
            break;
        default:
            ret = "Undefined node type";
            break;
    }

    return (char*)ret;
}

void ast_prune(ast_node_t** nodes, int num_children)
{
    int i;

    if (nodes == NULL || num_children == 0) {
        return;
    }

    for (i = 0; i < num_children; i++) {
        if (semantics_node_type_is_operator(nodes[i]->type)) {
            int j;

            for (j = 0; j < nodes[i]->num_children; j++) {
                if (nodes[i]->children[j]->type == AST_NODE_BLOCK_EXPRESSION_STMT) {
                    ast_node_t* tmp = nodes[i]->children[j];
                    if (tmp->num_children > 0) {
                        nodes[i]->children[j] = tmp->children[0];
                    }
                    free(tmp);
                }
            }
            continue;
        }
        ast_prune(nodes[i]->children, nodes[i]->num_children);
    }
}

void ast_record_block_num_children()
{
    if (_blocks_num_children != NULL) {
        _is_in_block = true;
        _blocks_num_children_count++;
        _blocks_num_children[_blocks_num_children_count - 1] = 0;
    }
}

void ast_record_func_call_actuals()
{
    if (_func_call_actuals != NULL) {
        _is_func_call_actual = true;
        _func_call_actuals_count++;
        _func_call_actuals[_func_call_actuals_count - 1] = 0;
    }
}

void ast_block_num_children_increment()
{
    if (_blocks_num_children != NULL) {
        if (_is_in_block) {
            _blocks_num_children[_blocks_num_children_count - 1]++;
        }
    }
}

void ast_func_call_actual_increment()
{
    if (_func_call_actuals != NULL) {
        if (_is_func_call_actual) {
        _func_call_actuals[_func_call_actuals_count - 1]++;
        }
    }
}

void ast_node_collection_append(ast_node_t* node)
{
    if (_ast_node_collection != NULL) {
        _ast_node_collection[_ast_node_collection_count] = node;
        _ast_node_collection_count++;
    } else {
        _ast_node_collection[_ast_node_collection_count] = NULL;
        _ast_node_collection_count++;
    }
}

void ast_repete_rules_nodes_append(ast_node_t* node)
{
    if (_ast_repete_rules_nodes != NULL) {
        _ast_repete_rules_nodes[_ast_repete_rules_node_count] = node;
        _ast_repete_rules_node_count++;
    } else {
        _ast_repete_rules_nodes[_ast_repete_rules_node_count] = NULL;
        _ast_repete_rules_node_count++;
    }
}

void ast_assembled_nodes_append(ast_node_t* node)
{
    if (_assembled_nodes != NULL) {
        _assembled_nodes[_assembled_node_count] = node;
        _assembled_node_count++;
    } else {
        _assembled_nodes[_assembled_node_count] = NULL;
        _assembled_node_count++;
    }
}

void ast_add_node(ast_node_t* node)
{
    if (_ast_start.nodes != NULL) {
        _ast_start.nodes[_ast_node_count] = node;
        _ast_node_count++;
    }
}

ast_node_t* ast_node_alloc(ast_parse_info_t* node_info,
                           ast_node_type_t node_type,
                           int num_children, 
                           ast_node_t** children)
{
    ast_node_t* ret = NULL;
    ast_parse_info_t *parse_info_alloc = (ast_parse_info_t*)calloc(sizeof(ast_parse_info_t), 1);
    
    ret = (ast_node_t*)calloc(sizeof(ast_node_t), 1);
    if (ret == NULL) {
        fprintf(stderr, "Failed to allocate memory");
        goto done;
    }

    if (parse_info_alloc != NULL) {
        if (node_info != NULL) {
            parse_info_alloc->lexeme = node_info->lexeme;
            parse_info_alloc->linenum = node_info->linenum;
        } else {
            free(parse_info_alloc);
            parse_info_alloc = NULL;
        }
    }

    ret->node_info = parse_info_alloc;
    ret->type = node_type;
    ret->num_children = num_children;
    ret->children = children;
done:
    return ret;
}

ast_parse_info_t* ast_node_info_alloc(char* lexeme, 
                                      const int linenum, 
                                      const int token_count)
{
    ast_parse_info_t* ret = NULL;

    ret = (ast_parse_info_t*) calloc(sizeof(ast_parse_info_t), 1);
    if (ret == NULL) {
        goto done;
    }

    ret->lexeme = lexeme;
    ret->linenum = linenum;
    ret->token_count = token_count;
done:
    return ret;
}

ast_node_t* ast_node_alloc_from_collection()
{
    ast_node_t* ret = NULL;

    _node_collection_ptr = &_ast_node_collection[_ast_node_collection_count - 1];
    _ast_node_collection_build(&ret);

    return ret;
}

ast_node_t* ast_node_alloc_from_assembled_nodes()
{
    ast_node_t* ret = NULL;
    /* first node is always a block */
    int i = _assembled_node_count - 2;
    int children_populated = 0;
    int num_children = _assembled_nodes[_assembled_node_count - 1]->num_children;
    int num_removed = 0;

    ret = ast_node_alloc(NULL,
                         AST_NODE_BLOCK,
                         0,
                         NULL);
    num_removed++;

    if (num_children == 0) {
        ast_node_t *empty_block_node = ast_node_alloc(NULL,
                                                      AST_NODE_BLOCK_EMPTY_STMT,
                                                      0,
                                                      NULL);
        if (empty_block_node != NULL) {
            ast_children_append(&(ret->children), &(ret->num_children), empty_block_node);
        }
    } else {

        while (i >= 0) {
            if (children_populated == num_children) {
                break;
            }
            if (_assembled_nodes[i]->type != AST_NODE_BLOCK) {
                ast_children_push_front(&(ret->children),
                                        &(ret->num_children),
                                        _assembled_nodes[i]);
                num_removed++;
            } else {
                int child_block_node_num_children = _assembled_nodes[i]->num_children;
                ast_node_t* child_block_node = ast_node_alloc(NULL,
                                                            AST_NODE_BLOCK,
                                                            0,
                                                            NULL);
                num_removed++;
                i--;
                /* >0 instead of >=0 here since we decremented i by 1 */
                while (child_block_node_num_children > 0) {
                    ast_children_push_front(&(child_block_node->children),
                                            &(child_block_node->num_children),
                                            _assembled_nodes[i]);
                    i--;
                    child_block_node_num_children--;
                    num_removed++;
                }
                
                ast_children_push_front(&(ret->children),
                                        &(ret->num_children),
                                        child_block_node);

                continue;
            }
            children_populated++;
            i--;
        }
    }

    for (i = 0; i < num_removed; i++) {
        (void)ast_assembled_nodes_pop();
    }
    return ret;
}

ast_node_t* ast_repete_rules_nodes_alloc_single(int index)
{
    ast_node_t* ret = NULL;

    if (_ast_repete_rules_node_count == 0) {
        goto done;
    }

    if (index >= _ast_repete_rules_node_count) {
        goto done;
    }

    ret = ast_node_alloc(_ast_repete_rules_nodes[index]->node_info,
                         _ast_repete_rules_nodes[index]->type,
                         0,
                         NULL);
done:
    return ret;
}

void ast_node_separate_multiple_unary_op_if_any(ast_node_t* node)
{
    size_t i;
    int num_consecutive_unary_op = 0;
    char unary_op = '\0';

    if (node == NULL) {
        return;
    }

    if (node->type == AST_NODE_BINARY_OP_NEGATE) {
        return;
    }

    if (node->node_info->lexeme != NULL) {
        if (_node_lexeme_contains_multiple_unary_op(node->node_info->lexeme, 
                                                    &num_consecutive_unary_op,
                                                    &unary_op)) {
            ast_node_t* tmp = node;
            ast_node_t* new_node = NULL;
            ast_parse_info_t* new_info = (ast_parse_info_t*) calloc(sizeof(ast_parse_info_t), 1);
            int new_lexeme_index = 1;
            const char* lexeme_cpy = node->node_info->lexeme;
            char *new_lexeme = (char*)calloc(sizeof(char), strlen(lexeme_cpy) - num_consecutive_unary_op + 2);
            
            i = 1;
            node->node_info->lexeme = NULL;
            if (unary_op == '-') {
                node->type = AST_NODE_UNARY_OP_NEGATE;
            } else if (unary_op == '!') {
                node->type = AST_NODE_UNARY_OP_LOGICAL_NEGATE;
            }

            if (node->type == AST_NODE_NUM) {
                /* num_consecutive_unary_op - 1 here since we need to leave 1 unary op out for the number token */
                while (i < num_consecutive_unary_op - 1) {
                    ast_node_t* unary_op_node = ast_node_alloc(node->node_info,
                                                            node->type,
                                                            0,
                                                            NULL);
                    ast_children_append(&tmp->children, &tmp->num_children, unary_op_node);
                    tmp = tmp->children[0];
                    i++;
                }
                for (i = 0; i < strlen(lexeme_cpy); i++) {
                    if (lexeme_cpy[i] == unary_op) {
                        continue;
                    }
                    new_lexeme[0] = unary_op;
                    new_lexeme[new_lexeme_index] = lexeme_cpy[i];
                    new_lexeme_index++;
                }
                new_info->lexeme = new_lexeme;
                new_info->linenum = node->node_info->linenum;
                new_node = ast_node_alloc(new_info,
                                        AST_NODE_NUM,
                                        0,
                                        NULL);
            } else {
                while (i < num_consecutive_unary_op) {
                    ast_node_t* unary_op_node = ast_node_alloc(node->node_info,
                                                            node->type,
                                                            0,
                                                            NULL);
                    ast_children_append(&tmp->children, &tmp->num_children, unary_op_node);
                    tmp = tmp->children[0];
                    i++;
                }
                for (i = 0; i < strlen(lexeme_cpy); i++) {
                    if (lexeme_cpy[i] == unary_op) {
                        continue;
                    }
                    new_lexeme[0] = lexeme_cpy[i];
                }
                new_info->lexeme = new_lexeme;
                new_info->linenum = node->node_info->linenum;
                new_node = ast_node_alloc(new_info,
                                          AST_NODE_ID,
                                          0,
                                          NULL);
            }
            ast_children_append(&tmp->children, &tmp->num_children, new_node);
            return;
        }
    }

    for (i = 0; i < node->num_children; i++) {
        ast_node_separate_multiple_unary_op_if_any(node->children[i]);
    }
}

void ast_func_formals_children_alloc(ast_node_t*** children_node, int* num_children)
{
    int i;

    for (i = 0; i < _ast_repete_rules_node_count - 1; i += 2) {
        ast_node_t* func_formal = ast_node_alloc(NULL,
                                                 AST_NODE_FUNC_SIG_FORMAL_CHILD,
                                                 0,
                                                 NULL);

        if (func_formal != NULL) {
            ast_children_append(&(func_formal->children),
                                &(func_formal->num_children),
                                _ast_repete_rules_nodes[i]);
            ast_children_append(&(func_formal->children),
                                &(func_formal->num_children),
                                _ast_repete_rules_nodes[i + 1]);
            ast_children_append(children_node,
                                num_children,
                                func_formal);
        }
    }
}

ast_node_t* ast_func_call_node_alloc()
{
    int num_removed = 0;
    int i;
    ast_node_t* ret = NULL;
    ast_node_t* func_actuals = ast_node_alloc(NULL,
                                              AST_NODE_FUNC_CALL_ACTUAL,
                                              0,
                                              NULL);

    if (_ast_repete_rules_node_count > 0) {
        i = _ast_repete_rules_node_count - 2;
        int num_actual_appended = 0;

        ret = ast_node_alloc(NULL,
                             AST_NODE_FUNC_CALL,
                             0,
                             NULL);
        
        if (_ast_repete_rules_nodes[_ast_repete_rules_node_count - 1]->type == AST_NODE_FUNC_ID) {
            _ast_repete_rules_nodes[_ast_repete_rules_node_count - 1]->type = AST_NODE_ID;
            ast_children_append(&(ret->children), 
                                &(ret->num_children),
                                _ast_repete_rules_nodes[_ast_repete_rules_node_count - 1]);
            num_removed++;
        } else if (_ast_repete_rules_nodes[_ast_repete_rules_node_count - 1]->type == AST_NODE_STRING) {
            ast_children_append(&(ret->children), 
                                &(ret->num_children),
                                _ast_repete_rules_nodes[_ast_repete_rules_node_count - 1]);
            num_removed++;
        }

        while (i >= 0) {
            if (num_actual_appended == _func_call_actuals[_func_call_actuals_count - 1]) {
                break;
            }
            ast_children_append(&(func_actuals->children),
                                &(func_actuals->num_children),
                                _ast_repete_rules_nodes[i]);
            i--;
            num_actual_appended++;
            num_removed++;
        }

        ast_children_append(&(ret->children), &(ret->num_children), func_actuals);
    } else {
        ret = ast_node_alloc(NULL,
                             AST_NODE_FUNC_CALL,
                             0,
                             NULL);
        
        if (ret != NULL) {
            if (_ast_repete_rules_nodes[0] != NULL) {
                _ast_repete_rules_nodes[0]->type = AST_NODE_ID;
                ast_children_append(&ret->children, &ret->num_children, _ast_repete_rules_nodes[0]);
                num_removed++;
            }
            if (func_actuals != NULL) {
                ast_children_append(&ret->children, &ret->num_children, func_actuals);
            }
        }
    }

    for (i = 0; i < num_removed; i++) {
        (void)ast_repete_rules_nodes_pop();
    }

    ast_func_call_actuals_pop();
    
    return ret;
}

ast_node_t* ast_node_collection_pop()
{
    ast_node_t* ret = NULL;

    if (_ast_node_collection_count > 0) {
        ret = _ast_node_collection[_ast_node_collection_count - 1];
        _ast_node_collection[_ast_node_collection_count - 1] = NULL;
        _ast_node_collection_count--;
    }

    return ret;
}

ast_node_t* ast_assembled_nodes_pop()
{
    ast_node_t* ret = NULL;

    if (_assembled_node_count > 0) {
        ret = _assembled_nodes[_assembled_node_count - 1];
        _assembled_nodes[_assembled_node_count - 1] = NULL;
        _assembled_node_count--;
    }

    return ret;
}

ast_node_t* ast_repete_rules_nodes_pop()
{
    ast_node_t* ret = NULL;

    if (_ast_repete_rules_node_count > 0) {
        ret = _ast_repete_rules_nodes[_ast_repete_rules_node_count - 1];
        _ast_repete_rules_nodes[_ast_repete_rules_node_count - 1] = NULL;
        _ast_repete_rules_node_count--;
    }

    return ret;
}

int ast_blocks_num_children_pop()
{
    int ret = 0;

    if (_blocks_num_children_count > 0) {
        ret = _blocks_num_children[_blocks_num_children_count - 1];
        _blocks_num_children[_blocks_num_children_count - 1] = 0;
        _blocks_num_children_count--;
    }

    return ret;
}

int ast_func_call_actuals_pop()
{
    int ret = 0;

    if (_func_call_actuals_count > 0) {
        ret = _func_call_actuals[_func_call_actuals_count - 1];
        _func_call_actuals[_func_call_actuals_count - 1] = 0;
        _func_call_actuals_count--;
    }

    return ret;
}

void ast_in_block_check()
{
    if (_blocks_num_children_count == 0) {
        _is_in_block = false;
    }
}

void ast_in_func_call_actual_check()
{
    if (_func_call_actuals_count == 0) {
        _is_func_call_actual = false;
    }
}

int ast_blocks_num_children_size()
{
    return _blocks_num_children_count;
}

int ast_func_call_actuals_size()
{
    return _func_call_actuals_count;
}

int ast_repete_rules_nodes_size()
{
    return _ast_repete_rules_node_count;
}

int ast_node_collection_size()
{
    return _ast_node_collection_count;
}

bool ast_repete_rules_nodes_empty()
{
    bool ret = false;

    if (_ast_repete_rules_node_count == 0) {
        ret = true;
    }

    return ret;
}

bool ast_assembled_nodes_empty()
{
    bool ret = false;

    if (_assembled_node_count == 0) {
        ret = true;
    }

    return ret;
}

ast_node_t** ast_children_alloc(ast_parse_info_t** children_info,
                                ast_node_type_t* children_types,
                                int num_children_alloc)
{
    int i;
    ast_node_t** ret = (ast_node_t**)calloc(sizeof(ast_node_t*), num_children_alloc);

    if (ret == NULL) {
        fprintf(stderr, "Failed to allocate memory for children\n");
        goto done;
    }

    for (i = 0; i < num_children_alloc; i++) {
        ret[i] = ast_node_alloc(children_info[i],
                                children_types[i],
                                0,
                                NULL);
        if (ret[i] == NULL) {
            fprintf(stderr, "Failed to allocate memory for children\n");
            if (i > 0) {
                free(ret[i-1]);
            }
            free(ret);
            goto done;
        }
    }

done:
    return ret;
}

void ast_children_push_front(ast_node_t*** children, 
                             int* current_children_size, 
                             ast_node_t* item)
{
    ast_node_t** ast_node_arr = NULL;
    if (children != NULL && *current_children_size > 0) {
        ast_node_arr = (ast_node_t**) realloc(*children, sizeof(ast_node_t*) * (*current_children_size + 1));
        if (ast_node_arr != NULL) {
            int i;

            *current_children_size = *current_children_size + 1;
            for (i = *current_children_size - 1; i >0 ; i--) {
                ast_node_arr[i] = ast_node_arr[i-1];
            }
            ast_node_arr[0] = item;
        }
    } else {
        ast_node_arr = (ast_node_t**) calloc(sizeof(ast_node_t*), 1);
        if (ast_node_arr != NULL) {
            ast_node_arr[0] = item;
            *current_children_size = 1;
        }
    }

    *children = ast_node_arr;
}

void ast_children_append(ast_node_t*** children, int* current_children_size, ast_node_t* item)
{
    ast_node_t** ast_node_arr = NULL;
    if (children != NULL && *current_children_size > 0) {
        ast_node_arr = (ast_node_t**) realloc(*children, sizeof(ast_node_t*) * (*current_children_size + 1));
        if (ast_node_arr != NULL) {
            ast_node_arr[*current_children_size] = item;
            *current_children_size = *current_children_size + 1;
        }
    } else {
        ast_node_arr = (ast_node_t**) calloc(sizeof(ast_node_t*), 1);
        if (ast_node_arr != NULL) {
            ast_node_arr[0] = item;
            *current_children_size = 1;
        }
    }

    *children = ast_node_arr;
}

ast_start_t* ast_get()
{
    return &_ast_start;
}

int ast_node_count_get()
{
    return _ast_node_count;
}

void ast_print()
{
    fprintf(stdout, "%s\n", _ast_start.node_name);
    _num_tabs = 1;
    _ast_print_pre_order(_ast_start.nodes, _ast_node_count);
    fprintf(stdout, "\n");
}

void ast_print_specific(ast_start_t* ast_start, int node_count)
{
    if (ast_start->symbol_loc != NULL) {
        fprintf(stdout, "%s sym=%p\n", ast_start->node_name, ast_start->symbol_loc);
    } else {
        fprintf(stdout, "%s\n", ast_start->node_name);
    }
    _num_tabs = 1;
    _ast_print_pre_order(ast_start->nodes, node_count);
    fprintf(stdout, "\n");
}

void ast_node_collection_print()
{
    int i;

    for (i = 0; i < _ast_node_collection_count; i++) {
        if (_ast_node_collection[i] != NULL) {
            ast_node_print(_ast_node_collection[i]);
            fprintf(stdout, "\n");
        }
    }
}

void ast_repete_rules_nodes_print()
{
    int i;
    int idx = _func_call_actuals_count - 1;

    for (i = 0; i < _ast_repete_rules_node_count; i++) {
        if (_ast_repete_rules_nodes[i] != NULL) {
            if (_ast_repete_rules_nodes[i]->type == AST_NODE_FUNC_ID) {
                printf("Num children: %d\n", _func_call_actuals[idx]);
                idx--;
            }
            ast_node_print(_ast_repete_rules_nodes[i]);
            fprintf(stdout, "\n");
        }
    }
}

void ast_assembled_nodes_print()
{
    int i;

    for (i = 0; i < _assembled_node_count; i++) {
        if (_assembled_nodes[i] != NULL) {
            if (_assembled_nodes[i]->type == AST_NODE_BLOCK) {
                printf("Node num children: %d\n", _assembled_nodes[i]->num_children);
            }
            ast_node_print(_assembled_nodes[i]);
            fprintf(stdout, "\n");
        }
    }
}

void ast_node_print(ast_node_t* node)
{
    ast_node_t** tmp = (ast_node_t**) calloc(sizeof(ast_node_t*), 1);

    tmp[0] = node;
    _num_tabs = 1;
    _ast_print_pre_order(tmp, 1);

    free(tmp);
}

void ast_node_collection_reset()
{
    int i;

    for (i = 0; i < _ast_node_collection_count; i++) {
        _ast_node_collection[i] = NULL;
    }
    _ast_node_collection_count = 0;
    _node_collection_ptr = NULL;
}

void ast_repete_rules_nodes_reset()
{
    int i;

    for (i = 0; i < _ast_repete_rules_node_count; i++) {
        _ast_repete_rules_nodes[i] = NULL;
    }
    _ast_repete_rules_node_count = 0;
}

void ast_assembled_nodes_reset()
{
    int i;

    for (i = 0; i < _assembled_node_count; i++) {
        _assembled_nodes[i] = NULL;
    }
    _assembled_node_count = 0;
}

void ast_blocks_num_children_reset()
{
    int i;

    for (i = 0; i < _blocks_num_children_count; i++) {
        _blocks_num_children[i] = 0;
    }
    _blocks_num_children_count = 0;
}

void ast_deinit()
{
    if (_ast_start.nodes != NULL) {
        _ast_children_free(_ast_start.nodes, _ast_node_count);
        free(_ast_start.nodes);
    }
    if (_ast_node_collection != NULL) {
        free(_ast_node_collection);
    }
    if (_ast_repete_rules_nodes != NULL) {
        free(_ast_repete_rules_nodes);
    }
}