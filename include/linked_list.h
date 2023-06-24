#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include "semantics.h"

#define ARRAY_LENGTH(arr)    (sizeof(arr)/sizeof(arr[0]))

typedef struct node {
   semantics_stab_record_t data;
   struct node *next;
} node_t;

semantics_stab_record_t* linked_list_append(node_t** head, semantics_stab_record_t* data);
void linked_list_append_at(node_t** curr, semantics_stab_record_t* data);
bool linked_list_symbol_found(node_t* head, const char* symbol_name, semantics_stab_record_t** found_data);
void linked_list_destroy(node_t** head);
void linked_list_print(node_t* head);
void linked_list_node_print(node_t* node);