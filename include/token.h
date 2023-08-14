#pragma once

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "glibc_util.h"
#include "../parser.tab.h"

#define TOKEN_LEXEME_MAX_LEN    2048
#define TOKEN_MAX_NUM           1024

typedef enum yytokentype token_type_t;

typedef struct token {
    token_type_t token_type;
    char *lexeme;
    int linenum;
} token_t;

/**
 * Returns the string representation of a token type
 * 
 * @param type  token type to retrieve the string representation
 * 
 * @return string representation of a given token type
*/
char* token_type_str(token_type_t type);
/**
 * Initializes the global variable like file pointer, memory allocation, etc.
 * 
 * @param filename filename to be opened and read input from
 * 
 * @return 0 on success, -1 on error
*/
int token_lexer_init(const char* filename);
/**
 * Deallocate memory and close input file
 * 
 * @return 0 on success, -1 on error
*/
void token_lexer_deinit();
/**
 * Gets all tokens from a previously initialized input stream file
 * 
 * @note token_lexer_init must be called before this function
 * 
 * @return 0 on success, -1 on error
*/
int token_lexer_get_tokens();
token_t* token_lexer_get_token();
token_t* token_prev_at(int rewind);
/**
 * Prints all the token retrieved from an initialized input stream file
 * 
 * @note token_lexer_init must be called before this function
 * 
 * @return 0 on success, -1 on error
*/
void token_lexer_print_tokens();
