// mparser.c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "mtypes.h"

extern Token next_token(FILE *f); 

/* Forward declaration for parse_primary to use parse_expression inside parentheses */
static ASTNode* parse_expression(FILE *in, Token *current_tok);

/* =========================================================================
 * AST Node Creation Helpers
 * ========================================================================= */
ASTNode* create_int_node(int val) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) {
        printf("[Kaoru Memory Error]: Allocation failed for ASTNode!\n Try to find the root of the problem.\n");
        exit(1);
    }
    node->type = NODE_INT;
    node->val = val;
    node->var_name[0] = '\0';
    node->left = NULL;
    node->right = NULL;
    return node;
} 

ASTNode* create_add_node(ASTNode* left, ASTNode* right) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) {
        printf("[Kaoru Memory Error]: Allocation failed for ASTNode!\n Try to find the root of the problem.\n");
        exit(1);
    }
    node->type = NODE_ADD;
    node->val = 0;
    node->var_name[0] = '\0';
    node->left = left;
    node->right = right;
    return node;
}

ASTNode* create_sub_node(ASTNode* left, ASTNode* right) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) {
        printf("[Kaoru Memory Error]: Allocation failed for ASTNode!\n Try to find the root of the problem.\n");
        exit(1);
    }
    node->type = NODE_SUB;
    node->val = 0;
    node->var_name[0] = '\0';
    node->left = left;
    node->right = right;
    return node;
}

ASTNode* create_mul_node(ASTNode* left, ASTNode* right) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) {
        printf("[Kaoru Memory Error]: Allocation failed for ASTNode!\n Try to find the root of the problem.\n");
        exit(1);
    }
    node->type = NODE_MUL;
    node->val = 0;
    node->var_name[0] = '\0';
    node->left = left;
    node->right = right;
    return node;
}

ASTNode* create_div_node(ASTNode* left, ASTNode* right) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) {
        printf("[Kaoru Memory Error]: Allocation failed for ASTNode!\n Try to find the root of the problem.\n");
        exit(1);
    }
    node->type = NODE_DIV;
    node->val = 0;
    node->var_name[0] = '\0';
    node->left = left;
    node->right = right;
    return node;
}

ASTNode* create_var_decl_node(const char* name, ASTNode* expr) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) {
        printf("[Kaoru Memory Error]: Allocation failed for ASTNode!\n Try to find the root of the problem.\n");
        exit(1);
    }
    node->type = NODE_VAR_DECL;
    node->val = 0;
    strncpy(node->var_name, name, sizeof(node->var_name) - 1);
    node->var_name[sizeof(node->var_name) - 1] = '\0';
    node->left = expr; 
    node->right = NULL;
    return node;
}

ASTNode* create_string_node(const char* str_val) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) exit(1);
    node->type = NODE_STR;
    node->val = 0;
    strncpy(node->var_name, str_val, sizeof(node->var_name) - 1);
    node->left = NULL;
    node->right = NULL;
    return node;
}

ASTNode* create_bool_node(bool val) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) {
        printf("[Kaoru Memory Error]: Allocation failed for ASTNode!\n Try to find the root of the problem.\n"); 
        exit(1);
    }
    node->type = NODE_BOOL;
    node->val = val ? 1 : 0; /* true is 1, false is 0 */
    node->var_name[0] = '\0';
    node->left = NULL;
    node->right = NULL;
    return node;
}
ASTNode* create_print_node(ASTNode *expr) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node){
        printf("[Kaoru Memory Error]: Allocation failed for ASTNode!\n where my printer?.\n");
        exit(1);
    }
    node->type = PRINT_NODE;
    node->left = expr;
    node->right = NULL;
    return node;
}
void free_ast(ASTNode *node) {
    if (node->type == PRINT_NODE) {
        free_ast(node->left); 
        free(node);
        return;
    }
    if (node == NULL) return;
    free_ast(node->left);
    free_ast(node->right);
    free(node);
}

/* =========================================================================
 * Security & Block Parsing Functions
 * ========================================================================= */
static bool parse_block(FILE *in, Token *current_tok) {
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

    *current_tok = next_token(in);

    if (current_tok->type == TOKEN_SEMICOLON) {
        *current_tok = next_token(in);
    }
    return true;
}

void parse_program(FILE *in, SecurityContext *sec_ctx) {
    Token tok = next_token(in);

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

/* =========================================================================
 * Expression & Statement Recursive Descent Parser
 * ========================================================================= */

/* Primary Expression: Handles Numbers, Strings, Booleans, Identifiers, and Grouping Parentheses */
static ASTNode* parse_primary(FILE *in, Token *current_tok) {
    /* 1. Numbers */
    if (current_tok->type == TOKEN_NUMBER) {
        ASTNode *node = create_int_node(atoi(current_tok->value));
        *current_tok = next_token(in); // Consume number
        return node;
    }
    
    /* 2. String Literals */
    if (current_tok->type == TOKEN_STRING_LIT) {
        ASTNode *node = create_string_node(current_tok->value);
        *current_tok = next_token(in); // Consume string
        return node;
    }

    /* 3. Identifiers & Booleans (true / false) */
    if (current_tok->type == TOKEN_IDENTIFIER) {
        if (strcmp(current_tok->value, "true") == 0) {
            ASTNode *node = create_bool_node(true);
            *current_tok = next_token(in);
            return node;
        } else if (strcmp(current_tok->value, "false") == 0) {
            ASTNode *node = create_bool_node(false);
            *current_tok = next_token(in);
            return node;
        }
        
        ASTNode *node = create_string_node(current_tok->value);
        *current_tok = next_token(in); // Consume identifier
        return node;
    }

    /* 4. Parenthesized Expressions: ( 10 + 20 ) */
    if (current_tok->type == TOKEN_LPAREN) {
        *current_tok = next_token(in); // Consume '('
        
        ASTNode *node = parse_expression(in, current_tok);
        if (!node) return NULL;

        if (current_tok->type == TOKEN_RPAREN) {
            *current_tok = next_token(in); // Consume ')'
            return node;
        } else {
            printf("[Kaoru Syntax Error Line %u]: Expected ')' after expression, got '%s'\n", 
                   current_tok->line, current_tok->value);
            free_ast(node);
            return NULL;
        }
    }

    printf("[Kaoru Syntax Error Line %u]: Expected number, string, identifier, bool, or '(', got '%s'\n", 
           current_tok->line, current_tok->value);
    return NULL;
}

/* Multiplicative Expression: Handles '*' and '/' (Higher Precedence) */
static ASTNode* parse_multiplicative(FILE *in, Token *current_tok) {
    ASTNode *left = parse_primary(in, current_tok);
    if (!left) return NULL;

    while (current_tok->type == TOKEN_STAR || current_tok->type == TOKEN_SLASH) {
        MTokenType op = current_tok->type;
        *current_tok = next_token(in); // Consume '*' or '/'

        ASTNode *right = parse_primary(in, current_tok);
        if (!right) {
            free_ast(left);
            return NULL;
        }

        if (op == TOKEN_STAR) {
            left = create_mul_node(left, right);
        } else if (op == TOKEN_SLASH) {
            left = create_div_node(left, right);
        }
    }

    return left;
}

/* Additive Expression: Handles '+' and '-' (Lower Precedence) */
static ASTNode* parse_expression(FILE *in, Token *current_tok) {
    ASTNode *left = parse_multiplicative(in, current_tok);
    if (!left) return NULL;

    while (current_tok->type == TOKEN_PLUS || current_tok->type == TOKEN_MINUS) {
        MTokenType op = current_tok->type;
        *current_tok = next_token(in); // Consume '+' or '-'

        ASTNode *right = parse_multiplicative(in, current_tok);
        if (!right) {
            free_ast(left);
            return NULL;
        }

        if (op == TOKEN_PLUS) {
            left = create_add_node(left, right);
        } else if (op == TOKEN_MINUS) {
            left = create_sub_node(left, right);
        }
    }

    return left;
}

/* Statement Parser: Handles variable declarations and expressions */
ASTNode* parse_statement(FILE *in, Token *current_tok) {
    if (current_tok->type == TOKEN_AT_PRINT) {
        *current_tok = next_token(in); 
        ASTNode *expr = parse_expression(in, current_tok);
        if (current_tok->type == TOKEN_SEMICOLON) {
            *current_tok = next_token(in);
        } else {
            printf("[Kaoru Syntax Error Line %u]: Expected ';' after @print statement\n", current_tok->line);
        }
        
        return create_print_node(expr);
    }
    /* Case 1: Variable Declaration (@int, @str, or @bool) */
    if (current_tok->type == TOKEN_AT_INT || 
        current_tok->type == TOKEN_AT_STR || 
        current_tok->type == TOKEN_AT_BOOL) {
        
        MTokenType decl_type = current_tok->type;
        *current_tok = next_token(in); // Consume type token (@int / @str / @bool)
        
        if (current_tok->type != TOKEN_IDENTIFIER) {
            printf("[Kaoru Syntax Error Line %u]: Expected variable name after type, got '%s'\n", 
                   current_tok->line, current_tok->value);
            return NULL;
        }

        char var_name[32];
        strncpy(var_name, current_tok->value, sizeof(var_name) - 1);
        var_name[sizeof(var_name) - 1] = '\0';

        *current_tok = next_token(in); // Consume identifier
        if (current_tok->type != TOKEN_ASSIGN) {
            printf("[Kaoru Syntax Error Line %u]: Expected '=' after variable name '%s'\n", 
                   current_tok->line, var_name);
            return NULL;
        }

        *current_tok = next_token(in); // Consume '='

        ASTNode *expr = parse_expression(in, current_tok);
        if (!expr) return NULL;

        if (current_tok->type == TOKEN_SEMICOLON) {
            *current_tok = next_token(in); // Consume ';'
        } else {
            printf("[Kaoru Syntax Error Line %u]: Expected ';' at end of declaration\n", current_tok->line);
            free_ast(expr);
            return NULL;
        }

        return create_var_decl_node(var_name, expr);
    }

    /* Case 2: Standalone Expression (e.g. (10 + 20) * 3;) */
    ASTNode *expr = parse_expression(in, current_tok);
    if (expr && current_tok->type == TOKEN_SEMICOLON) {
        *current_tok = next_token(in); // Consume ';'
    }
    
    return expr;
}



/* Wrapper entry-point parser function */
ASTNode* parse(FILE *in) {
    Token tok = next_token(in);
    if (tok.type == TOKEN_EOF) return NULL;
    return parse_statement(in, &tok);
}