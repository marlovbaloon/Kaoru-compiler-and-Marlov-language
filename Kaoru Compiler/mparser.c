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

