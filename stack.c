#include "include/stack.h"

static semantics_scope_t* _stack[SEMANTICS_SCOPE_MAX_NUM];
static int                _top;

void stack_init()
{
    int i;

    for (i = 0; i < SEMANTICS_SCOPE_MAX_NUM; i++) {
        _stack[i] = NULL;
    }

    _top = -1;
}

int stack_top_index()
{
    return _top;
}

semantics_scope_t* stack_pop()
{
    semantics_scope_t* ret = NULL;

    if (_top > -1) {
        ret = _stack[_top];
        _top--;    
    }

    return ret;
}

void stack_push(semantics_scope_t* new_scope)
{
    if (_top < SEMANTICS_SCOPE_MAX_NUM) {
        _top++;
        _stack[_top] = new_scope;
    }
}

semantics_scope_t* stack_peek()
{
    semantics_scope_t* ret = NULL;

    if (_top > -1) {
        ret = _stack[_top];   
    }

    return ret;
}

semantics_scope_t* stack_peek_at(int index)
{
    semantics_scope_t* ret = NULL;

    if (index > -1) {
        ret = _stack[index];   
    }

    return ret;
}

void stack_reset_to_file_scope()
{
    if (_top > -1) {
        semantics_scope_t* curr_scope = _stack[_top];

        while (strcmp(curr_scope->scope_name, "File") != 0 &&
               strcmp(curr_scope->scope_name, "Universal") != 0) {
            curr_scope = stack_pop();
        }
    }
}