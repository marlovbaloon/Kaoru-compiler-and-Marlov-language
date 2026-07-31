//mparser.c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "mtypes.h"

extern Token next_token(FILE *f); 

/* Forward declaration */
static bool parse_block(FILE *in, Token *current_tok);

static bool parse_block(FILE *in, Token *current_tok) {
    /* current_tok starts at TOKEN_LBRACE '{' */
    int depth = 1;

    while (depth > 0) {
        *current_tok = next_token(in);

        if (current_tok->type == TOKEN_EOF) {
            printf("[Kaoru Error]: Unterminated block (IDK your block!)\n");
            return false;
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
        *current_tok = next_token(in);
    }
    return true;
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

    printf("[Kaoru Compiler]: Permissions Validated. Hardware Hash Injected.\n");
} 


void free_ast(ASTNode *node) {
    if (node == NULL) {
        return;
    }
    free_ast(node->left);
    free_ast(node->right);
    free(node);
}

ASTNode* create_var_decl_node(char* name, ASTNode* expr) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_VAR_DECL;
    strcpy(node->var_name, name);
    node->left = expr; 
    node->right = NULL;
    return node;
}

ASTNode* create_int_node(int val) {
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_INT;
    node->val = val;
    node->left = node->right = NULL;
    return node;
} 

ASTNode* create_add_node(ASTNode* left, ASTNode* right){
    ASTNode *node = malloc(sizeof(ASTNode));
    node->type = NODE_ADD;
    node->left = left;
    node->right = right;
    return node;
}


ASTNode* parse(FILE *in) {
    Token tok = next_token(in); 

    // Case 1
    if (tok.type == TOKEN_AT_INT) {
        Token var_name = next_token(in); 
        
        // Syntax Check: @int is var name
        if (var_name.type != TOKEN_IDENTIFIER) {
            printf("[Kaoru Syntax Error]: Expected variable name after '@int', got '%s'\n", var_name.value);
            return NULL;
        }

        Token assign_op = next_token(in); 
        // Syntax Check:  '=' 
        if (strcmp(assign_op.value, "=") != 0 && assign_op.type != TOKEN_ASSIGN) {
            printf("[Kaoru Syntax Error]: Expected '=' after variable name '%s'\n", var_name.value);
            return NULL;
        }

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