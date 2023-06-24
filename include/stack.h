#pragma once

#include "semantics.h"

void stack_init();
int stack_top_index();
void stack_push(semantics_scope_t* new_scope);
semantics_scope_t* stack_pop();
semantics_scope_t* stack_peek();
semantics_scope_t* stack_peek_at(int index);
void stack_reset_to_file_scope();