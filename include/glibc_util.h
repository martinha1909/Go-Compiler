#pragma once

#include <stdbool.h>
#include <stdlib.h>

/**
 * Determines if an escape string is recognized by glibc
 * 
 * @param tok escape token to be examined
 * 
 * @return true if the escape token is recognized by glibc, false otherwise
*/
bool glibc_escape_recognized(const char tok);
/**
 * Determines if a bitwise operator is recognized by glibc
 * 
 * @param tok bitwise operator to be examined
 * 
 * @return true if the bitwise operator is recognized by glibc, false otherwise
*/
bool glibc_bitwise_operator_recognized(const char tok);
/**
 * Gets a string representation of an escape character
 * 
 * @param tok escape token to retreive the string representation from
 * 
 * @return a string representation of a given escape token
*/
char* glibc_escape_tok_str(const char tok);
/**
 * Gets a string representation of a bitwise operator
 * 
 * @param tok bitwise operator to retreive the string representation from
 * 
 * @return a string representation of a given bitwise operator
*/
char* glibc_bitwise_operator_str(const char tok);