// mparser.c
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "mtypes.h"

extern Token next_token(FILE *f); 

/* Forward declaration */
static ASTNode* parse_expression(FILE *in, Token *current_tok);
static ASTNode* parse_statement(FILE *in, Token *current_tok);

/* =========================================================================
 * AST Node Creation Helpers
 * ========================================================================= */
ASTNode* create_int_node(int val) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) {
        printf("[Kaoru Memory Error]: Allocation failed for ASTNode!\n");
        exit(1);
    }
    memset(node, 0, sizeof(ASTNode));
    node->type = NODE_INT;
    node->val = val;
    return node;
} 

ASTNode* create_add_node(ASTNode* left, ASTNode* right) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) exit(1);
    memset(node, 0, sizeof(ASTNode));
    node->type = NODE_ADD;
    node->left = left;
    node->right = right;
    return node;
}

ASTNode* create_sub_node(ASTNode* left, ASTNode* right) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) exit(1);
    memset(node, 0, sizeof(ASTNode));
    node->type = NODE_SUB;
    node->left = left;
    node->right = right;
    return node;
}

ASTNode* create_mul_node(ASTNode* left, ASTNode* right) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) exit(1);
    memset(node, 0, sizeof(ASTNode));
    node->type = NODE_MUL;
    node->left = left;
    node->right = right;
    return node;
}

ASTNode* create_div_node(ASTNode* left, ASTNode* right) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) exit(1);
    memset(node, 0, sizeof(ASTNode));
    node->type = NODE_DIV;
    node->left = left;
    node->right = right;
    return node;
}

ASTNode* create_var_decl_node(const char* name, ASTNode* expr) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) exit(1);
    memset(node, 0, sizeof(ASTNode));
    node->type = NODE_VAR_DECL;
    strncpy(node->var_name, name, sizeof(node->var_name) - 1);
    node->left = expr; 
    return node;
}

ASTNode* create_string_node(const char* str_val) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) exit(1);
    memset(node, 0, sizeof(ASTNode));
    node->type = NODE_STR;
    strncpy(node->var_name, str_val, sizeof(node->var_name) - 1);
    return node;
}

ASTNode* create_bool_node(bool val) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) exit(1);
    memset(node, 0, sizeof(ASTNode));
    node->type = NODE_BOOL;
    node->val = val ? 1 : 0; 
    return node;
}

ASTNode* create_print_node(ASTNode *expr) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) exit(1);
    memset(node, 0, sizeof(ASTNode));
    node->type = PRINT_NODE;
    node->left = expr;
    return node;
} 

ASTNode* create_if_node(ASTNode* cond, ASTNode* then_branch, ASTNode* else_branch) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) exit(1);
    memset(node, 0, sizeof(ASTNode));
    node->type = NODE_IF;
    node->cond = cond;
    node->then_branch = then_branch;
    node->else_branch = else_branch;
    return node;
}

ASTNode* create_binary_node(ASTNodeType type, ASTNode* left, ASTNode* right) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) exit(1);
    memset(node, 0, sizeof(ASTNode));
    node->type = type;
    node->left = left;
    node->right = right;
    return node;
}

/* Equivalent to AST_SCOPE_BLOCK in paper-1.md */
ASTNode* create_block_node(ASTNode** stmts, int count) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) exit(1);
    memset(node, 0, sizeof(ASTNode));
    node->type = NODE_BLOCK;
    node->statements = stmts;
    node->stmt_count = count;
    return node;
}

void free_ast(ASTNode *node) {
    if (!node) return;
    
    if (node->type == NODE_IF) {
        free_ast(node->cond);
        free_ast(node->then_branch);
        free_ast(node->else_branch);
        free(node);
        return;
    }
    
    if (node->type == NODE_BLOCK) {
        for (int i = 0; i < node->stmt_count; i++) {
            free_ast(node->statements[i]);
        }
        free(node->statements); 
        free(node);
        return;
    }
    
    if (node->type == PRINT_NODE) {
        free_ast(node->left); 
        free(node);
        return;
    }
    
    free_ast(node->left);
    free_ast(node->right);
    free(node);
}

/* =========================================================================
 * Security Context
 * ========================================================================= */
void parse_program(FILE *in, SecurityContext *sec_ctx) {
    long original_pos = ftell(in); // remember address file
    rewind(in); // return to file

    Token tok = next_token(in);
    while (tok.type != TOKEN_EOF) {
        if (tok.type == TOKEN_AT_SYS) {
            if (strcmp(tok.value, "@sys.disk.read") == 0) {
                sec_ctx->permissions |= PERM_DISK_READ;
            }
        }
        tok = next_token(in);
    }

    sec_ctx->hardware_hash = get_hardware_signature();
    printf("[Kaoru Compiler]: Security context initialized. Owner Hash: 0x%LX\n", (unsigned long long)sec_ctx->hardware_hash);   

    fseek(in, original_pos, SEEK_SET); //  return address before start Parse AST
}

/* =========================================================================
 * Expression & Statement Recursive Descent Parser
 * ========================================================================= */

static ASTNode* parse_primary(FILE *in, Token *current_tok) {
    if (current_tok->type == TOKEN_NUMBER) {
        ASTNode *node = create_int_node(atoi(current_tok->value));
        *current_tok = next_token(in); 
        return node;
    }
    
    if (current_tok->type == TOKEN_STRING_LIT) {
        ASTNode *node = create_string_node(current_tok->value);
        *current_tok = next_token(in); 
        return node;
    }

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
        *current_tok = next_token(in); 
        return node;
    }

    if (current_tok->type == TOKEN_LPAREN) {
        *current_tok = next_token(in); 
        ASTNode *node = parse_expression(in, current_tok);
        if (!node) return NULL;

        if (current_tok->type == TOKEN_RPAREN) {
            *current_tok = next_token(in); 
            return node;
        } else {
            printf("[Kaoru Syntax Error Line %u]: Expected ')'\n", current_tok->line);
            free_ast(node);
            return NULL;
        }
    }

    printf("[Kaoru Syntax Error Line %u]: Unexpected token '%s'\n", current_tok->line, current_tok->value);
    return NULL;
}

static ASTNode* parse_multiplicative(FILE *in, Token *current_tok) {
    ASTNode *left = parse_primary(in, current_tok);
    if (!left) return NULL;

    while (current_tok->type == TOKEN_STAR || current_tok->type == TOKEN_SLASH) {
        MTokenType op = current_tok->type;
        *current_tok = next_token(in); 

        ASTNode *right = parse_primary(in, current_tok);
        if (!right) {
            free_ast(left);
            return NULL;
        }

        if (op == TOKEN_STAR) {
            left = create_mul_node(left, right);
        } else {
            left = create_div_node(left, right);
        }
    }
    return left;
}

static ASTNode* parse_expression(FILE *in, Token *current_tok) {
    ASTNode *left = parse_multiplicative(in, current_tok);
    if (!left) return NULL;

    while (current_tok->type == TOKEN_PLUS || current_tok->type == TOKEN_MINUS) {
        MTokenType op = current_tok->type;
        *current_tok = next_token(in); 

        ASTNode *right = parse_multiplicative(in, current_tok);
        if (!right) {
            free_ast(left);
            return NULL;
        }

        if (op == TOKEN_PLUS) {
            left = create_add_node(left, right);
        } else {
            left = create_sub_node(left, right);
        }
    }
    return left;
}

static ASTNode* parse_relational(FILE *in, Token *current_tok) {
    ASTNode *left = parse_expression(in, current_tok);
    if (!left) return NULL;

    while (current_tok->type == TOKEN_EQ || current_tok->type == TOKEN_NEQ || 
           current_tok->type == TOKEN_LT || current_tok->type == TOKEN_GT ||
           current_tok->type == TOKEN_LTE || current_tok->type == TOKEN_GTE) {
            
            MTokenType op = current_tok->type; 
            *current_tok = next_token(in); 
            
            ASTNode *right = parse_expression(in, current_tok);
            if (!right) {
                free_ast(left);
                return NULL;
            }
            
            ASTNodeType node_type;
            switch (op) {
                case TOKEN_EQ: node_type = NODE_EQ; break;
                case TOKEN_NEQ: node_type = NODE_NEQ; break;
                case TOKEN_LT: node_type = NODE_LT; break;
                case TOKEN_GT: node_type = NODE_GT; break;
                case TOKEN_LTE: node_type = NODE_LTE; break;
                case TOKEN_GTE: node_type = NODE_GTE; break;
                default: node_type = NODE_EQ; break;
            }
            left = create_binary_node(node_type, left, right);
        }
        return left;
}

/* Aligned with paper-1.md Empirical Scope Syntax: { ... }; */
static ASTNode* parse_statement_or_block(FILE *in, Token *current_tok) {
    if (current_tok->type == TOKEN_LBRACE) {
        *current_tok = next_token(in); // Consume '{'
        ASTNode **stmts = NULL;
        int capacity = 0;
        int count = 0;

        while (current_tok->type != TOKEN_RBRACE && current_tok->type != TOKEN_EOF) {
            ASTNode *stmt = parse_statement(in, current_tok);
            if (stmt) {
                if (count >= capacity) {
                    capacity = (capacity == 0) ? 4 : capacity * 2;
                    ASTNode **new_stmts = realloc(stmts, sizeof(ASTNode*) * capacity);
                    if (!new_stmts) { 
                        free(stmts); exit(1); 
                    }
                    stmts = new_stmts;
                }
                stmts[count++] = stmt;
            }
        }
        
        if (current_tok->type == TOKEN_RBRACE) {
            *current_tok = next_token(in); // Consume '}'
            
            // Allow optional semicolon to support "{ @int x = 10; };" syntax from the paper
            if (current_tok->type == TOKEN_SEMICOLON) {
                *current_tok = next_token(in);
            }
        } else {
            printf("[Kaoru Syntax Error Line %u]: Expected '}' at end of block\n", current_tok->line);
        }
        return create_block_node(stmts, count);
    }
    return parse_statement(in, current_tok);
}

static ASTNode* parse_if_statement(FILE *in, Token *current_tok) {
    *current_tok = next_token(in); 

    if (current_tok->type != TOKEN_LPAREN) {
        printf("[Kaoru Syntax Error Line %u]: Expected '('\n", current_tok->line);
        return NULL;
    }
    *current_tok = next_token(in); 

    ASTNode *cond = parse_relational(in, current_tok);
    if (!cond) return NULL;
    
    if (current_tok->type != TOKEN_RPAREN) {
        printf("[Kaoru Syntax Error Line %u]: Expected ')'\n", current_tok->line);
        free_ast(cond);
        return NULL;
    }
    *current_tok = next_token(in); 

    ASTNode *then_branch = parse_statement_or_block(in, current_tok);
    if (!then_branch) {
        free_ast(cond);
        return NULL;
    }

    ASTNode *else_branch = NULL;
    if (current_tok->type == TOKEN_ELSE) {
        *current_tok = next_token(in); 
        if (current_tok->type == TOKEN_IF) {
            else_branch = parse_if_statement(in, current_tok);
        } else {
            else_branch = parse_statement_or_block(in, current_tok);
        }
    }

    return create_if_node(cond, then_branch, else_branch);
}

/* Consolidated Main Statement Router */
static ASTNode* parse_statement(FILE *in, Token *current_tok) {
    if (current_tok->type == TOKEN_IF) {
        return parse_if_statement(in, current_tok);
    }

    if (current_tok->type == TOKEN_AT_PRINT) {
        *current_tok = next_token(in); 
        ASTNode *expr = parse_expression(in, current_tok);
        if (current_tok->type == TOKEN_SEMICOLON) {
            *current_tok = next_token(in);
        } else {
            printf("[Kaoru Syntax Error Line %u]: Expected ';' after @print\n", current_tok->line);
        }
        return create_print_node(expr);
    }

    if (current_tok->type == TOKEN_AT_INT || 
        current_tok->type == TOKEN_AT_STR || 
        current_tok->type == TOKEN_AT_BOOL) {
        
        *current_tok = next_token(in); 
        
        if (current_tok->type != TOKEN_IDENTIFIER) {
            printf("[Kaoru Syntax Error Line %u]: Expected variable name\n", current_tok->line);
            return NULL;
        }

        char var_name[32];
        strncpy(var_name, current_tok->value, sizeof(var_name) - 1);
        var_name[sizeof(var_name) - 1] = '\0';

        *current_tok = next_token(in); 
        if (current_tok->type != TOKEN_ASSIGN) {
            printf("[Kaoru Syntax Error Line %u]: Expected '=' after '%s'\n", current_tok->line, var_name);
            return NULL;
        }

        *current_tok = next_token(in); 

        ASTNode *expr = parse_expression(in, current_tok);
        if (!expr) return NULL;

        if (current_tok->type == TOKEN_SEMICOLON) {
            *current_tok = next_token(in); 
        } else {
            printf("[Kaoru Syntax Error Line %u]: Expected ';' at end of declaration\n", current_tok->line);
            free_ast(expr);
            return NULL;
        }

        return create_var_decl_node(var_name, expr);
    }

    ASTNode *expr = parse_relational(in, current_tok);
    if (expr && current_tok->type == TOKEN_SEMICOLON) {
        *current_tok = next_token(in); 
    }
    
    return expr;
}

/* Wrapper entry-point parser function */
ASTNode* parse(FILE *in) {
    Token tok = next_token(in);
    if (tok.type == TOKEN_EOF) return NULL;

    ASTNode **stmts = NULL;
    int capacity = 0;
    int count = 0;

    while (tok.type != TOKEN_EOF) {
        ASTNode *stmt = parse_statement(in, &tok);
        if (stmt) {
            if (count >= capacity) {
                capacity = (capacity == 0) ? 4 : capacity * 2;
                ASTNode **new_stmts = realloc(stmts, sizeof(ASTNode*) * capacity);
                if (!new_stmts) { free(stmts); exit(1); }
                stmts = new_stmts;
            }
            stmts[count++] = stmt;
        }
    }
    return create_block_node(stmts, count); // return Root Block 
}