#include "include/token.h"

#define MAX_NUM_ILLEGAL_TOKS            11

static FILE* _fp = NULL;
static char _ch;
static int _linenum = 1;
static int _num_tokens = 0;
static int _num_illegal_toks = 0;
static int _token_pos = 0;
static token_t _tokens[TOKEN_MAX_NUM];
static token_t *_token_ref = NULL;
static token_type_t _curr_tok_type;

static void _add_token(token_type_t token_type);

static void _token_ref_dealloc()
{
    if (_token_ref != NULL) {
        if (_token_ref->lexeme != NULL) {
            free(_token_ref->lexeme);
            _token_ref->lexeme = NULL;
        }
        free(_token_ref);
        _token_ref = NULL;
    }
}

static token_t* _token_ref_alloc()
{
    /* safe check to make sure we have freed the previous instance */
    if (_token_ref != NULL) {
        _token_ref_dealloc();
    }

    _token_ref = (token_t*) calloc(sizeof(token_t), 1);
    _token_ref->lexeme = (char*) calloc(sizeof(char), TOKEN_LEXEME_MAX_LEN);
    strcpy(_token_ref->lexeme, _tokens[_token_pos].lexeme);
    _token_ref->linenum = _tokens[_token_pos].linenum;
    _token_ref->token_type = _tokens[_token_pos].token_type;

    return _token_ref;
}

static token_t* _token_ref_alloc_at(int index)
{
    token_t *ret = NULL;

    ret = (token_t*) calloc(sizeof(token_t), 1);
    ret->lexeme = (char*) calloc(sizeof(char), TOKEN_LEXEME_MAX_LEN);
    strcpy(ret->lexeme, _tokens[index].lexeme);
    ret->linenum = _tokens[index].linenum;
    ret->token_type = _tokens[index].token_type;

    return ret;
}

/**
 * Checks to see if the word "NUL" is in any token, since this is an illegal word
 * 
 * @param index  index of the token list that needs to be checked
 * 
 * @return true if the token's lexeme contains NUL, false otherwise
*/
static bool _token_contains_nul(int index)
{
    bool ret = false;
    char lexeme_cpy[strlen(_tokens[index].lexeme)];

    strcpy(lexeme_cpy, _tokens[index].lexeme);

    if (strstr(lexeme_cpy, "NUL")) {
        ret = true;
    }

    return ret;
}

/**
 * Adds a semicolon token. 
 * 
 * @note Per Golf specs, this function should only be called once a new line character is detected
 *       and the ending token is one of the following:
 *          - An ID
 *          - An integer
 *          - A string
 *          - break or return keywords
 *          - ')' or '}'
 * 
 * @param line_num  line number of the line needed for semi colon insertion
*/
static void _add_semicolon_token(int line_num)
{
    /* empty lexeme */
    _tokens[_num_tokens].lexeme[0] = '\0';
    _tokens[_num_tokens].token_type = TOKEN_TYPE_SEMI_COLON;
    _tokens[_num_tokens].linenum = line_num;

    _num_tokens++;
}

/**
 * Adds a comment token, only to be called when the sequence "//" is detected
*/
static void _add_comment_token()
{
    int comment_index = 1;
    char comment[TOKEN_LEXEME_MAX_LEN];

    comment[0] = '/';
    comment[1] = '/';

    while (1) {
        _ch = (char)fgetc(_fp);

        if (_ch == '\n' || _ch == EOF) {
            if (_ch == '\n') {
                comment_index++;
                comment[comment_index] = _ch;
            }
            
            break;
        }

        comment_index++;
        comment[comment_index] = _ch;
    }

    strcpy(_tokens[_num_tokens].lexeme, comment);
    _tokens[_num_tokens].lexeme[comment_index + 1] = '\0';
    _tokens[_num_tokens].token_type = TOKEN_TYPE_COMMENT;
    _tokens[_num_tokens].linenum = _linenum;

    _curr_tok_type = TOKEN_TYPE_COMMENT;

    if (_ch == '\n') {
        _linenum++;
    }

    _num_tokens++;
}

static void _add_newline_token()
{
    const char* lex = "\\n";

    strcpy(_tokens[_num_tokens].lexeme, lex);
    _tokens[_num_tokens].linenum = _linenum;
    _tokens[_num_tokens].token_type = TOKEN_TYPE_NEWLINE;

    _curr_tok_type = TOKEN_TYPE_NEWLINE;
    _num_tokens++;
}

/**
 * Adds a comment token, only to be called when a number is detected, 
 * this ends when the next character read is a non-number character
 * 
 * @param num_c     the first number detected when reading in from input
*/
static void _add_number_token(const char num_c)
{
    int num_idx = 0;
    char number_str[TOKEN_LEXEME_MAX_LEN];
    bool semi_colon_added = false;

    number_str[0] = num_c;

    while (1) {
        _ch = (char)fgetc(_fp);

        /* if it is not a character, our number sequence ends */
        if (!isdigit(_ch)) {
            break;
        }
        num_idx++;
        number_str[num_idx] = _ch;
    }

    number_str[num_idx + 1] = '\0';

    strcpy(_tokens[_num_tokens].lexeme, number_str);
    _tokens[_num_tokens].linenum = _linenum;
    _tokens[_num_tokens].token_type = TOKEN_TYPE_NUM;

    _curr_tok_type = TOKEN_TYPE_NUM;

    if (_ch == '\n') {
        _num_tokens++;
        _add_semicolon_token(_linenum);
        _add_newline_token();
        /* set to none to make sure we don't add again */
        _curr_tok_type = TOKEN_TYPE_NONE;
        semi_colon_added = true;
        _linenum++;
    } else {
        fseek(_fp, -1, SEEK_CUR);
    }

    if (!semi_colon_added) {
        _num_tokens++;
    }
}

/**
 * Adds a string token, only to be called when the character '"' is encountered
 * ends when another '"' is encountered
*/
static void _add_string_token()
{
    int idx = 0;
    bool string_error = false;
    char string_error_msg[TOKEN_LEXEME_MAX_LEN];

    while(1) {
        _ch = (char)fgetc(_fp);

        if (_ch == '"') {
            break;
        } else if (_ch == '\n') {
            strcpy(string_error_msg, "string contains newline");
            string_error = true;
            break;
        } else if (_ch == EOF) {
            strcpy(string_error_msg, "string terminated by EOF");
            string_error = true;
            break;
        }

        /* escape sequence encountered */
        if (_ch == '\\') {
            /* we want to get the next character to see if the escape character is legal or not */
            _ch = (char)fgetc(_fp);

            switch (_ch) {
                case 'b':
                    _tokens[_num_tokens].lexeme[idx] = '\\';
                    idx++;
                    _tokens[_num_tokens].lexeme[idx] = 'b';
                    break;
                case 'f':
                    _tokens[_num_tokens].lexeme[idx] = '\\';
                    idx++;
                    _tokens[_num_tokens].lexeme[idx] = 'f';
                    break;
                case 'n':
                    _tokens[_num_tokens].lexeme[idx] = '\\';
                    idx++;
                    _tokens[_num_tokens].lexeme[idx] = 'n';
                    break;
                case 'r':
                    _tokens[_num_tokens].lexeme[idx] = '\\';
                    idx++;
                    _tokens[_num_tokens].lexeme[idx] = 'r';
                    break;
                case 't':
                    _tokens[_num_tokens].lexeme[idx] = '\\';
                    idx++;
                    _tokens[_num_tokens].lexeme[idx] = 't';
                    break;
                case '\\':
                    _tokens[_num_tokens].lexeme[idx] = '\\';
                    idx++;
                    _tokens[_num_tokens].lexeme[idx] = '\\';
                    break;
                case '"':
                    _tokens[_num_tokens].lexeme[idx] = '\\';
                    idx++;
                    _tokens[_num_tokens].lexeme[idx] = '"';
                    break;
                case EOF:
                    /* in the case of EOF encountered, that means our string is not terminated by another '"', which is illegal */
                    strcpy(string_error_msg, "string terminated by EOF");
                    string_error = true;
                    break;
                case '\n':
                    /* in the case of '\n' encountered, that means our string is not terminated by another '"', which is illegal */
                    strcpy(string_error_msg, "string contains newline");
                    string_error = true;
                    break;
                default:
                    /* any other escape characters are illegal */
                    string_error = true;
                    snprintf(string_error_msg, TOKEN_LEXEME_MAX_LEN, "bad string escape '%c'", _ch);
                    break;
            }
            if (string_error) {
                break;
            } else {
                idx++;
                continue;
            }
        }

        _tokens[_num_tokens].lexeme[idx] = _ch;
        idx++;
    }

    _tokens[_num_tokens].lexeme[idx] = '\0';
    _tokens[_num_tokens].linenum = _linenum;
    if (string_error) {
        strcpy(_tokens[_num_tokens].lexeme, string_error_msg);
        _tokens[_num_tokens].token_type = TOKEN_TYPE_ILLEGAL_STRING;
        _curr_tok_type = TOKEN_TYPE_ILLEGAL_STRING;
    } else {
        _tokens[_num_tokens].token_type = TOKEN_TYPE_STRING;
        _curr_tok_type = TOKEN_TYPE_STRING;
    }

    _num_tokens++;
}

/**
 * Adds an identifier token
*/
static void _add_id_token()
{
    char lex[TOKEN_LEXEME_MAX_LEN];
    int idx = 0;
    bool semi_colon_added = false;

    lex[idx] = _ch;

    while (1) {
        _ch = (char)fgetc(_fp);

        if (_ch == '\r') {
            continue;
        }

        if (_ch == '\n' || 
            _ch == EOF || 
            _ch == ' ' || 
            _ch == '"' || 
            _ch == '(' || 
            _ch == ')' ||
            _ch == '-' ||
            _ch == '+' ||
            _ch == ',' ||
            _ch == ';' ||
            _ch == '\t') {
            break;
        }

        idx++;
        lex[idx] = _ch;
    }
    lex[idx + 1] = '\0';

    strcpy(_tokens[_num_tokens].lexeme, lex);
    _tokens[_num_tokens].linenum = _linenum;
    _tokens[_num_tokens].token_type = TOKEN_TYPE_ID;

    _curr_tok_type = TOKEN_TYPE_ID;

    if (_ch == '\n') {
        _num_tokens++;
        _add_semicolon_token(_linenum);
        _add_newline_token();
        /* set to none to make sure we don't add again */
        _curr_tok_type = TOKEN_TYPE_NONE;
        semi_colon_added = true;
        _linenum++;
    } else if (_ch == '"' || 
               _ch == '(' || 
               _ch == ',' || 
               _ch == ')' ||
               _ch == ';') {
        fseek(_fp, -1, SEEK_CUR);
    }

    if (!semi_colon_added) {
        _num_tokens++;
    }

    if (_ch == '+') {
        _add_token(TOKEN_TYPE_ADD);
    } else if (_ch == '-') {
        _add_token(TOKEN_TYPE_SUB);
    }
}

/**
 * Adds an illegal token
*/
static void _add_illegal_token(const char illegal_tok)
{
    _tokens[_num_tokens].lexeme[0] = illegal_tok;
    _tokens[_num_tokens].lexeme[1] = '\0';
    _tokens[_num_tokens].linenum = _linenum;
    _tokens[_num_tokens].token_type = TOKEN_TYPE_ILLEGAL;

    _num_tokens++;
}

/**
 * Adds any token type. A matching token type string will also be populated
 * 
 * @param  token_type   token type to be added
*/
static void _add_token(token_type_t token_type)
{
    const char* token_str = token_type_str(token_type);

    strcpy(_tokens[_num_tokens].lexeme, token_str);
    _tokens[_num_tokens].linenum = _linenum;
    _tokens[_num_tokens].token_type = token_type;

    _curr_tok_type = token_type;

    _num_tokens++;
}

/**
 * determines if a character is a part of a whitespace
 * 
 * @param c character to be determined
 * 
 * @return true if a character is a whitespace, false otherwise
*/
static bool _token_is_whitespace(const char c)
{
    bool ret = false;

    if (c == '\n' ||
        c == '\r' ||
        c == '\t') {
        ret = true;
    }

    return ret;
}

/**
 * determines if a token is an operator or a punctuation
 * 
 * @note    for multi-character operators (for example, &&), this function will still be called 
 *          to retrieve the rest of the operator. If the operator doesn't match after retrieving,
 *          we simply rewind back the read characters from the file and add that to a list of 
 *          illegal tokens
 * 
 * @return true if a character is an operator or punctuation, false otherwise
*/
static bool _token_is_operator_or_punc()
{
    bool ret = false;
    switch(_ch) {
        case '+':
            _add_token(TOKEN_TYPE_ADD);
            ret = true;
            break;
        case '-':
            _add_token(TOKEN_TYPE_SUB);
            ret = true;
            break;
        case '*':
            _add_token(TOKEN_TYPE_MULT);
            ret = true;
            break;
        case '/':
            _ch = (char)fgetc(_fp);
            if (_ch != '/') {
                _add_token(TOKEN_TYPE_DIV);
                fseek(_fp, -1, SEEK_CUR);
            } else {
                /* comment block encountered, retrieve the rest of comment block */
                _add_comment_token();
            }
            ret = true;
            break;
        case '%':
            _add_token(TOKEN_TYPE_PERCENTAGE);
            ret = true;
            break;
        case '&':
            _ch = (char)fgetc(_fp);
            /* operator '&&' */
            if (_ch == '&') {
                _add_token(TOKEN_TYPE_AND_CMP);
            } else {
                _add_illegal_token('&');
                fseek(_fp, -1, SEEK_CUR);
            }
            ret = true;
            break;
        case '|':
            _ch = (char)fgetc(_fp);
            /* operator '||' */
            if (_ch == '|') {
                _add_token(TOKEN_TYPE_OR_CMP);
            } else {
                _add_illegal_token('|');
                fseek(_fp, -1, SEEK_CUR);
            }
            ret = true;
            break;
        case '>':
            _ch = (char)fgetc(_fp);
            if (_ch != '=') {
                _add_token(TOKEN_TYPE_GT);
                fseek(_fp, -1, SEEK_CUR);
            } else {
                /* operator '>=' */
                _add_token(TOKEN_TYPE_GE);
            }
            ret = true;
            break;
        case '<':
            _ch = (char)fgetc(_fp);
            if (_ch != '=') {
                _add_token(TOKEN_TYPE_LT);
                fseek(_fp, -1, SEEK_CUR);
            } else {
                /* operator '<=' */
                _add_token(TOKEN_TYPE_LE);
            }
            ret = true;
            break;
        case '=':
            _ch = (char)fgetc(_fp);
            if (_ch != '=') {
                _add_token(TOKEN_TYPE_ASSIGNMENT);
                fseek(_fp, -1, SEEK_CUR);
            } else {
                /* operator '==' */
                _add_token(TOKEN_TYPE_EQ_CMP);
            }
            ret = true;
            break;
        case '!':
            _ch = (char)fgetc(_fp);
            if (_ch != '=') {
                _add_token(TOKEN_TYPE_NOT);
                fseek(_fp, -1, SEEK_CUR);
            } else {
                /* operator '!=' */
                _add_token(TOKEN_TYPE_NOT_EQ_CMP);
            }
            ret = true;
            break;
        case '(':
            _add_token(TOKEN_TYPE_OPEN_ROUND_BRACKET);
            ret = true;
            break;
        case '{':
            _add_token(TOKEN_TYPE_OPEN_CURLY_BRACKET);
            ret = true;
            break;
        case ',':
            _add_token(TOKEN_TYPE_COMMA);
            ret = true;
            break;
        case ')':
            _add_token(TOKEN_TYPE_CLOSE_ROUND_BRACKET);
            ret = true;
            break;
        case '}':
            _add_token(TOKEN_TYPE_CLOSE_CURLY_BRACKET);
            ret = true;
            break;
        case ';':
            _add_token(TOKEN_TYPE_SEMI_COLON);
            ret = true;
            break;
        default:
            ret = false;
            break;
    }

    return ret;
}

/**
 * Adds a keyword or an identifier token
 * 
 * @note we begin this function by examining the first letter of a given correct keyword.
 *       Then we estimate how long that keyword is and attempt to read the next remaining characters to a buffer.
 *       Once all the characters are read, the following cases could happen:
 *          - If the read buffer matches with the keyword, we add a keyword token
 *          - If the read buffer does not match with the keyword:
 *              - if the next character is a whitespace, new line (i.e '\n'), or EOF, we add 
 *                the content of a buffer as an identifier token
 *              - if the next character is not a whitespace, we keep reading til we encounter one 
 *                and add the content of the buffer as an indentifier token
 * 
 * @param   keyword_len             estimated length of the keyword to read from the input stream
 * @param   correct_keyword         keyword to compare the buffer to
 * @param   correct_keywork_type    if the correct keyword is matched, we use this to populate the token
*/
static void _add_keyword_or_id(int keyword_len,
                               const char* correct_keyword, 
                               token_type_t correct_keywork_type)
{
    char keyword[keyword_len + 1];
    int i = 1;
    int rewind_num = -keyword_len;

    /* peek at the next character to see if it's a white space */
    _ch = fgetc(_fp);
    /* easiest case: the next character is a white space then it is definitely an id */
    if (_ch == ' ' || _ch == '\n') {
        fseek(_fp, -1, SEEK_CUR);
        _ch = correct_keyword[0];
        _add_id_token();
    } else {
        fseek(_fp, -1, SEEK_CUR);
        keyword[0] = correct_keyword[0];
        while (i < keyword_len) {
            _ch = (char)fgetc(_fp);
            keyword[i] = _ch;
            i++;
        }
        keyword[i] = '\0';
        if (strcmp(keyword, correct_keyword) == 0) {
            _ch = (char)fgetc(_fp);
            if (_ch == ' ' || _ch == '\n' || _ch == EOF) {
                _add_token(correct_keywork_type);
                if (_ch == '\n') {
                    if (correct_keywork_type == TOKEN_TYPE_KEYWORD_BREAK ||
                        correct_keywork_type == TOKEN_TYPE_KEYWORD_RETURN) {
                        _add_semicolon_token(_linenum);
                        /* set to none to make sure we don't add again */
                        _curr_tok_type = TOKEN_TYPE_NONE;
                    }
                    _linenum++;
                }
            } else {
                /* need to set back to the first letter of the incorrect keyword to populate the identifier */
                fseek(_fp, rewind_num - 1, SEEK_CUR);
                _ch = (char)fgetc(_fp);
                _add_id_token();
            }
        } else {
            fseek(_fp, rewind_num, SEEK_CUR);
            _ch = (char)fgetc(_fp);
            _add_id_token();
        }
    }
}

/**
 * Determines if the current character pointed to the input stream is a beginning of a keyword
 * if the character is not a beginning of a keyword, then we examine to see if it is an identifier
 * 
 * @note    An identifier will NOT be created if it starts with the following:
 *              - a number
 *              - an escape character
 *              - a bitwise operator
 *              - a non-ascii character
 *              - a special character like '#'
 * 
 * @return true if the character is the beginning of a keyword or an identifier
*/
static bool _token_is_keyword_or_id()
{
    bool ret = false;

    switch (_ch) {
        case 'b':
            _add_keyword_or_id(strlen("break"), "break", TOKEN_TYPE_KEYWORD_BREAK);
            ret = true;
            break;
        case 'e':
            _add_keyword_or_id(strlen("else"), "else", TOKEN_TYPE_KEYWORD_ELSE);
            ret = true;
            break;
        case 'f':
            _ch = (char)fgetc(_fp);
            if (_ch == 'o') {
                /* rewind 1 here since _add_keyword_or_id will need to read this character again */
                fseek(_fp, -1, SEEK_CUR);
                _add_keyword_or_id(strlen("for"), "for", TOKEN_TYPE_KEYWORD_FOR);
            } else if (_ch == 'u') {
                /* rewind 1 here since _add_keyword_or_id will need to read this character again */
                fseek(_fp, -1, SEEK_CUR);
                _add_keyword_or_id(strlen("func"), "func", TOKEN_TYPE_KEYWORD_FUNC);
            } else {
                /* rewind 2 here since _add_keyword_or_id will need to read these 2 characters again to form a complete identifier */
                fseek(_fp, -2, SEEK_CUR);
                _ch = (char)fgetc(_fp);
                _add_id_token();
            }
            ret = true;
            break;
        case 'i':
            _add_keyword_or_id(strlen("if"), "if", TOKEN_TYPE_KEYWORD_IF);
            ret = true;
            break;
        case 'r':
            _add_keyword_or_id(strlen("return"), "return", TOKEN_TYPE_KEYWORD_RETURN);
            ret = true;
            break;
        case 'v':
            _add_keyword_or_id(strlen("var"), "var", TOKEN_TYPE_KEYWORD_VAR);
            ret = true;
            break;
        default:
            /* a punctuation should not be the case here since it is already handled */
            if (!isdigit(_ch) && 
                !glibc_escape_recognized(_ch) && 
                !glibc_bitwise_operator_recognized(_ch) &&
                (_ch >= 0 && _ch <= 127) &&
                _ch != '#') {
                _add_id_token();
                ret = true;
            }
            break;
    }

    return ret;
}

int token_lexer_init(const char* filename)
{
    int err = -1;
    int i;

    _fp = fopen(filename, "r");
    if (_fp == NULL) {
        fprintf(stderr, "Failed to open file %s, %s", filename, strerror(errno));
        goto done;
    }

    for (i = 0; i < TOKEN_MAX_NUM; i++) {
        _tokens[i].lexeme = (char*)calloc(TOKEN_LEXEME_MAX_LEN, sizeof(char));
        if (_tokens[i].lexeme == NULL) {
            fprintf(stderr, "failed to allocate memory\n");
            goto done;
        }
    }

    err = 0;
done:
    return err;
}

char* token_type_str(token_type_t type)
{
    char* ret;
    switch (type) {
        case TOKEN_TYPE_ILLEGAL:
            ret = "illegal character";
            break;
        case TOKEN_TYPE_ILLEGAL_STRING:
            ret = "illegal string";
            break;
        case TOKEN_TYPE_ID:
            ret = "id";
            break;
        case TOKEN_TYPE_STRING:
            ret = "string";
            break;
        case TOKEN_TYPE_KEYWORD_BREAK:
            ret = "break";
            break;
        case TOKEN_TYPE_KEYWORD_ELSE:
            ret = "else";
            break;
        case TOKEN_TYPE_KEYWORD_FOR:
            ret = "for";
            break;
        case TOKEN_TYPE_KEYWORD_FUNC:
            ret = "func";
            break;
        case TOKEN_TYPE_KEYWORD_IF:
            ret = "if";
            break;
        case TOKEN_TYPE_KEYWORD_RETURN:
            ret = "return";
            break;
        case TOKEN_TYPE_KEYWORD_VAR:
            ret = "var";
            break;
        case TOKEN_TYPE_NUM:
            ret = "int";
            break;
        case TOKEN_TYPE_ADD:
            ret = "+";
            break;
        case TOKEN_TYPE_SUB:
            ret = "-";
            break;
        case TOKEN_TYPE_MULT:
            ret = "*";
            break;
        case TOKEN_TYPE_DIV:
            ret = "/";
            break;
        case TOKEN_TYPE_PERCENTAGE:
            ret = "%";
            break;
        case TOKEN_TYPE_AND_CMP:
            ret = "&&";
            break;
        case TOKEN_TYPE_OR_CMP:
            ret = "||";
            break;
        case TOKEN_TYPE_EQ_CMP:
            ret = "==";
            break;
        case TOKEN_TYPE_GT:
            ret = ">";
            break;
        case TOKEN_TYPE_LT:
            ret = "<";
            break;
        case TOKEN_TYPE_ASSIGNMENT:
            ret = "=";
            break;
        case TOKEN_TYPE_NOT:
            ret = "!";
            break;
        case TOKEN_TYPE_NOT_EQ_CMP:
            ret = "!=";
            break;
        case TOKEN_TYPE_LE:
            ret = "<=";
            break;
        case TOKEN_TYPE_GE:
            ret = ">=";
            break;
        case TOKEN_TYPE_OPEN_ROUND_BRACKET:
            ret = "(";
            break;
        case TOKEN_TYPE_OPEN_CURLY_BRACKET:
            ret = "{";
            break;
        case TOKEN_TYPE_COMMA:
            ret = ",";
            break;
        case TOKEN_TYPE_CLOSE_ROUND_BRACKET:
            ret = ")";
            break;
        case TOKEN_TYPE_CLOSE_CURLY_BRACKET:
            ret = "}";
            break;
        case TOKEN_TYPE_SEMI_COLON:
            ret = ";";
            break;
        case TOKEN_TYPE_COMMENT:
            ret = "//";
            break;
        case TOKEN_TYPE_NONE:
            ret = "none";
            break;
        case TOKEN_TYPE_NEWLINE:
            ret = "\\n";
            break;
        default:
            ret = "Unexpected token type";
            break;
    }

    return ret;
}

int token_lexer_get_tokens()
{
    int err = -1;

    if (_fp == NULL) {
        goto done;
    }

    while (1) {
        /* other functions might have encountered EOF */
        if (_ch == EOF) {
            if (_curr_tok_type == TOKEN_TYPE_CLOSE_ROUND_BRACKET ||
                _curr_tok_type == TOKEN_TYPE_CLOSE_CURLY_BRACKET ||
                _curr_tok_type == TOKEN_TYPE_KEYWORD_BREAK  ||
                _curr_tok_type == TOKEN_TYPE_KEYWORD_RETURN ||
                _curr_tok_type == TOKEN_TYPE_ID ||
                _curr_tok_type == TOKEN_TYPE_NUM ||
                _curr_tok_type == TOKEN_TYPE_STRING) {
                _add_semicolon_token(_linenum);
                _curr_tok_type = TOKEN_TYPE_NONE;
            }
            break;
        }

        _ch = (char)fgetc(_fp);

        if (_ch == EOF) {
            if (_curr_tok_type == TOKEN_TYPE_CLOSE_ROUND_BRACKET ||
                _curr_tok_type == TOKEN_TYPE_CLOSE_CURLY_BRACKET ||
                _curr_tok_type == TOKEN_TYPE_KEYWORD_BREAK ||
                _curr_tok_type == TOKEN_TYPE_KEYWORD_RETURN ||
                _curr_tok_type == TOKEN_TYPE_ID ||
                _curr_tok_type == TOKEN_TYPE_NUM ||
                _curr_tok_type == TOKEN_TYPE_STRING) {
                _add_semicolon_token(_linenum);
                _curr_tok_type = TOKEN_TYPE_NONE;
            }
            break;
        }

        if (_ch == '\n') {
            if (_curr_tok_type == TOKEN_TYPE_CLOSE_ROUND_BRACKET ||
                _curr_tok_type == TOKEN_TYPE_CLOSE_CURLY_BRACKET ||
                _curr_tok_type == TOKEN_TYPE_STRING) {
                _add_semicolon_token(_linenum);
                _curr_tok_type = TOKEN_TYPE_NONE;
            }
            _add_newline_token();
            _linenum++;
            continue;
        } else if (_ch == ' ' || _ch == '\0') {
            continue;
        } else if (_ch == '"') {
            _add_string_token();
            continue;
        } else if (_token_is_whitespace(_ch)) {
            continue;
        } else if (_token_is_operator_or_punc()) {
            continue;
        } else if (_token_is_keyword_or_id()) {
            continue;  
        } else if (isdigit(_ch)) {
            _add_number_token(_ch);
            continue;
        } else {
            if (_num_illegal_toks < MAX_NUM_ILLEGAL_TOKS) {
                _add_illegal_token(_ch);
            }
            _num_illegal_toks++;
            continue;
        }
    }

    err = 0;
done:
    return err;
}

token_t* token_lexer_get_token()
{
    token_t *ret = NULL;

    if (_token_ref != NULL) {
        _token_ref_dealloc();
    }

    if (_token_pos >= _num_tokens) {
        goto done;
    }

    if (_token_pos < 0) {
        goto done;
    }

    /* if memory allocation fails, ret should be NULL here as well */
    ret = _token_ref_alloc();
    _token_pos++;

done:
    return ret;
}

token_t* token_prev_at(int rewind)
{
    return _token_ref_alloc_at(_token_pos - rewind - 1);
}

void token_lexer_print_tokens()
{
    int current_illegal_tok_num = 0;
    if (_num_tokens > 0) {
        int i;

        for (i = 0; i < _num_tokens; i++) {
            if (_tokens[i].token_type == TOKEN_TYPE_NONE) {
                break;
            }

            if (_token_contains_nul(i)) {
                fprintf(stdout, "Warning: skipping NUL character at line %d\n", _tokens[i].linenum);
            } else {
                if (_tokens[i].token_type == TOKEN_TYPE_COMMENT) {
                    continue;
                } else if (_tokens[i].token_type == TOKEN_TYPE_ILLEGAL) {
                    if (current_illegal_tok_num < MAX_NUM_ILLEGAL_TOKS) {
                        if (!isascii((int)_tokens[i].lexeme[0])) {
                            fprintf(stderr, "Warning: skipping non-ASCII input character at line %d\n", _tokens[i].linenum);
                        } else {
                            char* illegal_tok;
                            
                            if (glibc_escape_recognized(_tokens[i].lexeme[0])) {
                                illegal_tok = glibc_escape_tok_str(_tokens[i].lexeme[0]);
                                if (strcmp(illegal_tok, "undefined") == 0) {
                                    illegal_tok = &_tokens[i].lexeme[0];
                                }
                                fprintf(stderr, "Warning: skipping unknown character '%s' at line %d\n", illegal_tok,
                                                                                                        _tokens[i].linenum);
                            }  else if (glibc_bitwise_operator_recognized(_tokens[i].lexeme[0])) {
                                illegal_tok = glibc_bitwise_operator_str(_tokens[i].lexeme[0]);
                                if (strcmp(illegal_tok, "undefined") == 0) {
                                    illegal_tok = &_tokens[i].lexeme[0];
                                }
                                fprintf(stderr, "error: %s not supported in Golf at line %d\n", illegal_tok,
                                                                                          _tokens[i].linenum);
                                break;
                            } else {
                                illegal_tok = &_tokens[i].lexeme[0];
                                fprintf(stderr, "Warning: skipping unknown character '%s' at line %d\n", illegal_tok,
                                                                                                        _tokens[i].linenum);
                            }
                        }

                        if (current_illegal_tok_num == MAX_NUM_ILLEGAL_TOKS - 1) {
                            fprintf(stderr, "error: too many warnings at line %d\n", _tokens[i].linenum);
                            break;
                        }
                    }
                    current_illegal_tok_num++;
                } else if (_tokens[i].token_type == TOKEN_TYPE_ILLEGAL_STRING) {
                    fprintf(stderr, "error: %s at line %d\n", _tokens[i].lexeme, _tokens[i].linenum);
                    break;
                } else {
                    fprintf(stdout, "%s\t[%s] @ line %d\n", token_type_str(_tokens[i].token_type),
                                                            _tokens[i].lexeme,
                                                            _tokens[i].linenum);
                }
            }
        }
    }
}

void token_lexer_deinit()
{
    int i;

    if (_fp != NULL) {
        fclose(_fp);
    }

    for (i = 0; i < TOKEN_MAX_NUM; i++) {
        if (_tokens[i].lexeme != NULL) {
            free(_tokens[i].lexeme);
        }
    }
}