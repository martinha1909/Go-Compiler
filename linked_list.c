#include "include/linked_list.h"

static node_t* _node_alloc_new(semantics_stab_record_t *data)
{
    node_t* ret = (node_t*)calloc(sizeof(node_t), 1);

    // printf("Printing data:\n");
    // semantics_record_print(data);

    if (ret != NULL) {
        ret->data.symbol_name = strdup(data->symbol_name);
        ret->data.sig         = strdup(data->sig);
        ret->data.rv_sig      = strdup(data->rv_sig);
        ret->data.is_const    = data->is_const;
        ret->data.is_type     = data->is_type;
    }

    // linked_list_node_print(ret);

    return ret;
}

static void _node_dealloc(semantics_stab_record_t* data)
{
    if (data != NULL) {
        if (data->symbol_name != NULL) {
            free(data->symbol_name);
        }
        if (data->sig != NULL) {
            free(data->sig);
        }
        if (data->rv_sig != NULL) {
            free(data->rv_sig);
        }
        free(data);
    }
}

semantics_stab_record_t* linked_list_append(node_t** head, semantics_stab_record_t* data)
{
    semantics_stab_record_t* ret = NULL;
    if (*head != NULL) {
        node_t *new_node = _node_alloc_new(data);
        node_t *current_node = *head;

        while (current_node->next != NULL) {
            current_node = current_node->next;
        }

        current_node->next = new_node;
        ret = &(current_node->next->data);
    } else {
        *head = _node_alloc_new(data);
        (*head)->next = NULL;
        ret = &((*head)->data);
    }

    return ret;
}

void linked_list_append_at(node_t** curr, semantics_stab_record_t* data)
{
    /* only append if the next node is NULL to make sure we don't lose existing data */
    if (*curr != NULL && (*curr)->next == NULL) {
        (*curr)->next = _node_alloc_new(data);
    } else if (*curr == NULL) {
        *curr = _node_alloc_new(data);
        (*curr)->next = NULL;
    }
}

bool linked_list_symbol_found(node_t* head, const char* symbol_name, semantics_stab_record_t** found_data)
{
    bool ret = false;
    node_t* curr_node = head;

    while (curr_node != NULL) {
        if (strcmp(curr_node->data.symbol_name, symbol_name) == 0) {
            *found_data = &(curr_node->data);
            ret = true;
            break;
        }

        curr_node = curr_node->next;
    }

    return ret;
}

void linked_list_destroy(node_t** head)
{
    node_t* curr_node = *head;

    while (curr_node != NULL) {
        _node_dealloc(&(curr_node->data));
        curr_node = curr_node->next;
    }

    *head = NULL;
}

void linked_list_print(node_t* head)
{
    node_t* curr_node = head;

    while (curr_node != NULL) {
        semantics_record_print(&curr_node->data);
        curr_node = curr_node->next;
    }
}

void linked_list_node_print(node_t* node)
{
    if (node != NULL) {
        semantics_record_print(&(node->data));
    }
}