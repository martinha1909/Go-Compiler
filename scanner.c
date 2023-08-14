#include "include/scanner.h"
// #include "parser.tab.h"
#include "include/token.h"
#include "include/ast.h"
#include "include/semantics.h"
#include "include/asm_code_gen.h"

static int _err_line_num;
static token_type_t _err_tok;

#define FILE_EXISTS(f)  (access(f, F_OK) == 0 ? true: false)

static bool _file_is_empty(const char* filename)
{
    bool ret = false;
    FILE *fp;

    fp = fopen(filename, "r");

    if (fp) {
        fseek(fp, 0, SEEK_END);
        if (ftell(fp) == 0) {
            ret = true;
        }
        fclose(fp);
    } else {
        ret = true;
    }

    return ret;
}

int scanner_yylex()
{
    int ret = 0;
    token_t* tok = token_lexer_get_token();

    if (tok == NULL) {
        ret = TOKEN_TYPE_YYEOF;
        goto done;
    }
    
    /* ignore comments */
    while (tok->token_type == TOKEN_TYPE_COMMENT || 
          /* this case is where semi colon is automatically inserted, we want to keep the token if it was not automatically inserted */
          (tok->token_type == TOKEN_TYPE_SEMI_COLON)) {
        tok = token_lexer_get_token();
        if (tok == NULL) {
            ret = TOKEN_TYPE_YYEOF;
            goto done;
        }
    }

    // printf("Token type: %s\n", token_type_str(tok->token_type));
    // printf("Lex: %s\n", tok->lexeme);

    // if (tok->token_type == TOKEN_TYPE_OPEN_CURLY_BRACKET) {
    //     if (ast_blocks_num_children_size() > 0) {
    //         ast_block_num_children_increment();
    //     }
    //     ast_record_block_num_children();
    // } else if (tok->token_type == TOKEN_TYPE_CLOSE_CURLY_BRACKET) {
    //     ast_in_block_check();
    // }
    if (tok->token_type == TOKEN_TYPE_OPEN_ROUND_BRACKET) {
        /* if an ID is followed by a '(', we know that it's a function call */
        if (token_prev_at(1)->token_type == TOKEN_TYPE_ID ||
            token_prev_at(1)->token_type == TOKEN_TYPE_STRING) {
            /* ignore function declaration */
            if (token_prev_at(2)->token_type != TOKEN_TYPE_KEYWORD_FUNC) {
                if (ast_func_call_actuals_size() > 0) {
                    ast_func_call_actual_increment();
                }
                ast_record_func_call_actuals();
            }
        }
    } else if (tok->token_type == TOKEN_TYPE_CLOSE_ROUND_BRACKET) {
        ast_in_func_call_actual_check();
    }

    yylval.info = (ast_parse_info_t*)calloc(sizeof(ast_parse_info_t), 1);
    yylval.info->lexeme = (char*)calloc(sizeof(char), TOKEN_LEXEME_MAX_LEN);
    strcpy(yylval.info->lexeme, tok->lexeme);
    yylval.info->linenum = tok->linenum;

    _err_line_num = tok->linenum;
    _err_tok = tok->token_type;

    ret = tok->token_type;
done:
    return ret;
}

void scanner_yyerror(const char* err)
{
    // if (_err_tok == TOKEN_TYPE_NEWLINE) {
    //     fprintf(stderr, "error: unexpected newline at or near line %d\n", _err_line_num);
    // } else {
    //     fprintf(stderr, "error: syntax error on \"%s\" at or near line %d\n", token_type_str(_err_tok), _err_line_num);
    // }
    fprintf(stderr, "%s at line %d\n", err, _err_line_num);
}

int main(int argc, char** argv)
{
    int ret = EXIT_FAILURE;
    ast_start_t* ast_annotated = NULL;
    int ast_annotated_node_count = 0;

    if (argc < 2) {
        fprintf(stderr, "Too few arguments\n");
        goto done;
    }
    if (argc > 2) {
        fprintf(stderr, "Too many arguments\n");
        goto done;
    }
    if (!FILE_EXISTS(argv[1])) {
        fprintf(stderr, "nonexistent file\n");
        goto done;
    }
    if (_file_is_empty(argv[1])) {
        goto done;
    }

    if (token_lexer_init(argv[1]) == -1) {
        goto done;
    }
    if (token_lexer_get_tokens() == -1) {
        fprintf(stderr, "Failed to get tokens\n");
        goto done;
    }

    if (ast_init() == -1) {
        fprintf(stderr, "Failed to initialize ast\n");
        goto done;
    }

    if(yyparse() != 0)
    {
        goto done;
    }

    ast_annotated = ast_get();
    ast_annotated_node_count = ast_node_count_get();
    ast_prune(ast_annotated->nodes, ast_annotated_node_count);

    // ast_print_specific(ast_annotated, ast_annotated_node_count);

    semantics_init();

    semantics_global_decl_check(ast_annotated, ast_annotated_node_count);
    semantics_decl_check(ast_annotated, ast_annotated_node_count);
    semantics_type_check(ast_annotated, ast_annotated_node_count);
    semantics_miscellaneous_check(ast_annotated, ast_annotated_node_count);

    // ast_print_specific(ast_annotated, ast_annotated_node_count);
    asm_code_gen(ast_annotated->nodes, ast_annotated_node_count);

    ret = EXIT_SUCCESS;
done:
    token_lexer_deinit();
    ast_deinit();

    return ret;
}