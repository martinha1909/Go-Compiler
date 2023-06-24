#include "include/glibc_util.h"

bool glibc_escape_recognized(const char tok)
{
    bool ret = false;

    if (tok == '\a' ||
        tok == '\b' ||
        tok == '\f' ||
        tok == '\n' ||
        tok == '\r' ||
        tok == '\t' ||
        tok == '\v' ||
        tok == '\'' ||
        tok == '\"' ||
        tok == '\?' ||
        tok == '\\' ||
        tok == '\0') {
        ret = true;
    }

    return ret;
}

bool glibc_bitwise_operator_recognized(const char tok)
{
    bool ret = false;

    if (tok == '&' ||
        tok == '|' ||
        tok == '^' ||
        tok == '~') {
        ret = true;
    }

    return ret;
}

char* glibc_escape_tok_str(const char tok)
{
    const char *ret = "undefined";
    switch (tok) {
        case '\a':
            ret = "\\a";
            break;
        case '\b':
            ret = "\\b";
            break;
        case '\f':
            ret = "\\f";
            break;
        case '\n':
            ret = "\\n";
            break;
        case '\r':
            ret = "\\r";
            break;
        case '\t':
            ret = "\\t";
            break;
        case '\v':
            ret = "\\v";
            break;
        case '\'':
            ret = "\\'";
            break;
        case '\"':
            ret = "\\\"";
            break;
        case '\?':
            ret = "\\?";
            break;
        case '\\':
            ret = "\\\\";
            break;
        case '\0':
            ret = "\\0";
            break;
    }
    return (char*) ret;
}

char* glibc_bitwise_operator_str(const char tok)
{
    const char *ret = "undefined";
    switch (tok) {
        case '&':
            ret = "Bitwise AND";
            break;
        case '|':
            ret = "Bitwise OR";
            break;
        case '^':
            ret = "Bitwise XOR";
            break;
        case '~':
            ret = "Bitwise complement";
            break;
    }
    return (char*)ret;
}