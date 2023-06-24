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

// typedef enum token_type_e {
    // TOKEN_TYPE_NONE = 0,
    // TOKEN_TYPE_ILLEGAL,
    // TOKEN_TYPE_ILLEGAL_STRING,
    // TOKEN_TYPE_ID,
    // TOKEN_TYPE_STRING,
    // TOKEN_TYPE_NUM,
    // TOKEN_TYPE_KEYWORD_BREAK,
    // TOKEN_TYPE_KEYWORD_ELSE,
    // TOKEN_TYPE_KEYWORD_FOR,
    // TOKEN_TYPE_KEYWORD_FUNC,
    // TOKEN_TYPE_KEYWORD_IF,
    // TOKEN_TYPE_KEYWORD_RETURN,
    // TOKEN_TYPE_KEYWORD_VAR,
    // TOKEN_TYPE_ADD = 43,
    // TOKEN_TYPE_SUB = 45,
    // TOKEN_TYPE_MULT = 42,
    // TOKEN_TYPE_DIV = 47,
    // TOKEN_TYPE_PERCENTAGE = 37,
    // TOKEN_TYPE_AND_CMP,
    // TOKEN_TYPE_OR_CMP,
    // TOKEN_TYPE_EQ_CMP,
    // TOKEN_TYPE_GT,
    // TOKEN_TYPE_LT,
    // TOKEN_TYPE_ASSIGNMENT,
    // TOKEN_TYPE_NOT,
    // TOKEN_TYPE_NOT_EQ_CMP,
    // TOKEN_TYPE_LE,
    // TOKEN_TYPE_GE,
    // TOKEN_TYPE_OPEN_ROUND_BRACKET = 40,
    // TOKEN_TYPE_OPEN_CURLY_BRACKET = 23,
    // TOKEN_TYPE_COMMA,
    // TOKEN_TYPE_CLOSE_ROUND_BRACKET = 41,
    // TOKEN_TYPE_CLOSE_CURLY_BRACKET = 25,
    // TOKEN_TYPE_SEMI_COLON,
    // TOKEN_TYPE_COMMENT
// } token_type_t;

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
