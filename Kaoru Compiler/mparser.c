//mparser.c
#include <stdio.h>
#include <stdlib.h>
#include "mtypes.h"

extern Token next_token(FILE *f); 

/* Parse isolated block statement {}; or standard control block {} */
static void parse_block(FILE *in, Token *current_tok);
static void parse_block(FILE *in, Token *current_tok) {
    /* current_tok starts at TOKEN_LBRACE '{' */
    int depth = 1;

    while (depth > 0) {
        *current_tok = next_token(in);

        if (current_tok->type == TOKEN_EOF) {
            /* Error: Unterminated block */
            exit(1);
        }

        if (current_tok->type == TOKEN_LBRACE) {
            depth++;
        } else if (current_tok->type == TOKEN_RBRACE) {
            depth--;
        }
    }

    /* Check token right after closing brace '}' */
    *current_tok = next_token(in);

    if (current_tok->type == TOKEN_SEMICOLON) {
        /*
         * Pattern: { }; 
         * Isolated Statement Scope: Reset stack pointer immediately (Zero-GC)
         */
        *current_tok = next_token(in);
    } else {
        /*
         * Pattern: {} 
         * Expression / Control Flow Block: Pass return value up
         */
    }
}

void parse_program(FILE *in, SecurityContext *sec_ctx) {
    Token tok = next_token(in);

    /* Force @sys permission check at the file header */
    while (tok.type == TOKEN_AT_SYS) {
        if (strcmp(tok.value, "@sys.disk.read") == 0) {
            sec_ctx->permissions |= PERM_DISK_READ;
        }
        tok = next_token(in);
    }

    if (!(sec_ctx->permissions & PERM_DISK_READ)) {
        printf("[Kaoru Error]: Security Violation! Missing @sys.disk.read at file header.\n");
        exit(1);
    }

    /* check function Parse and Scope { }; */
    printf("[Kaoru Compiler]: Permissions Validated. Hardware Hash Injected.\n");
} 

ASTNode* parse(FILE *in) {
    Token tok = next_token(in);
    // Case 1: 
    if (tok.type == TOKEN_AT_INT) {
        Token var_name = next_token(in); 
        Token assign_op = next_token(in); 
        Token val_tok = next_token(in);   
        ASTNode *val_node = create_int_node(atoi(val_tok.value));
        return create_var_decl_node(var_name.value, val_node);
    }
    // Case 2
    ASTNode *left = create_int_node(atoi(tok.value));
    Token op = next_token(in); 
    if (op.type == TOKEN_EOF || op.type == TOKEN_SEMICOLON) {
        return left; 
    }
    Token t2 = next_token(in); 
    ASTNode *right = create_int_node(atoi(t2.value));
    return create_add_node(left, right);
}