#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "mtypes.h"

extern Token next_token(FILE *f); 

/* Forward declarations */
static ASTNode* parse_statement(FILE *in, Token *current_tok);
static ASTNode* parse_statement_or_block(FILE *in, Token *current_tok);
static ASTNode* parse_expression(FILE *in, Token *current_tok);
static ASTNode* parse_builtin_call(FILE *in, Token *current_tok);
static ASTNode* parse_for_statement(FILE *in, Token *current_tok);
static ASTNode* parse_func_decl(FILE *in, Token *current_tok);

static ASTArena global_arena = {NULL, 0, 0};

void init_ast_arena(size_t initial_capacity) {
    global_arena.nodes = (ASTNode *)malloc(sizeof(ASTNode) * initial_capacity);
    global_arena.capacity = initial_capacity;
    global_arena.count = 0;
}

static BuiltinKind resolve_builtin_kind(const char *name) {
    if (strcmp(name, "open") == 0)   return BUILTIN_OPEN;
    if (strcmp(name, "read") == 0)   return BUILTIN_READ;
    if (strcmp(name, "write") == 0)  return BUILTIN_WRITE;
    if (strcmp(name, "close") == 0)  return BUILTIN_CLOSE;
    if (strcmp(name, "alloc") == 0)  return BUILTIN_ALLOC;
    if (strcmp(name, "free") == 0)   return BUILTIN_FREE;
    if (strcmp(name, "sizeof") == 0) return BUILTIN_SIZEOF;
    if (strcmp(name, "exit") == 0)   return BUILTIN_EXIT;
    if (strcmp(name, "panic") == 0)  return BUILTIN_PANIC;
    return (BuiltinKind)-1;
}

/* =========================================================================
 * AST Node Creation Helpers
 * ========================================================================= */
static ASTNode* create_ast_node(ASTNodeType type) {
    ASTNode *node = (ASTNode *)malloc(sizeof(ASTNode));
    if (!node) {
        printf("[Kaoru Memory Error]: Allocation failed for ASTNode!\n");
        exit(1);
    }
    memset(node, 0, sizeof(ASTNode));
    node->type = type;
    return node;
}

ASTNode* create_int_node(int val) {
    ASTNode *node = create_ast_node(NODE_INT);
    node->val = val;
    return node;
} 

ASTNode* create_add_node(ASTNode* left, ASTNode* right) {
    ASTNode *node = create_ast_node(NODE_ADD);
    node->left = left;
    node->right = right;
    return node;
}

ASTNode* create_sub_node(ASTNode* left, ASTNode* right) {
    ASTNode *node = create_ast_node(NODE_SUB);
    node->left = left;
    node->right = right;
    return node;
}

ASTNode* create_mul_node(ASTNode* left, ASTNode* right) {
    ASTNode *node = create_ast_node(NODE_MUL);
    node->left = left;
    node->right = right;
    return node;
}

ASTNode* create_div_node(ASTNode* left, ASTNode* right) {
    ASTNode *node = create_ast_node(NODE_DIV);
    node->left = left;
    node->right = right;
    return node;
}

ASTNode* create_var_decl_node(MTokenType data_type, const char* name, ASTNode* expr) {
    ASTNode *node = create_ast_node(NODE_VAR_DECL);
    node->data_type = data_type; 
    strncpy(node->var_name, name, sizeof(node->var_name) - 1);
    node->left = expr; 
    return node;
}

ASTNode* create_string_node(const char* str_val) {
    ASTNode *node = create_ast_node(NODE_STR);
    strncpy(node->str_val, str_val, sizeof(node->str_val) - 1);
    strncpy(node->var_name, str_val, sizeof(node->var_name) - 1);
    return node;
}

ASTNode* create_bool_node(bool val) {
    ASTNode *node = create_ast_node(NODE_BOOL);
    node->val = val ? 1 : 0; 
    return node;
}

ASTNode* create_print_node(ASTNode *expr) {
    ASTNode *node = create_ast_node(PRINT_NODE);
    node->left = expr;
    return node;
} 

ASTNode* create_if_node(ASTNode* cond, ASTNode* then_branch, ASTNode* else_branch) {
    ASTNode *node = create_ast_node(NODE_IF);
    node->cond = cond;
    node->then_branch = then_branch;
    node->else_branch = else_branch;
    return node;
}

ASTNode* create_while_node(ASTNode* cond, ASTNode* body) {
    ASTNode *node = create_ast_node(NODE_WHILE);
    node->cond = cond;          
    node->then_branch = body;   
    return node;
}

ASTNode* create_binary_node(ASTNodeType type, ASTNode* left, ASTNode* right) {
    ASTNode *node = create_ast_node(type);
    node->left = left;
    node->right = right;
    return node;
}

ASTNode* create_block_node(ASTNode** stmts, int count) {
    ASTNode *node = create_ast_node(NODE_BLOCK);
    node->statements = stmts;
    node->stmt_count = count;
    return node;
}

ASTNode* create_func_decl_node(const char *name, char **params, int param_count, ASTNode *body) {
    ASTNode *node = create_ast_node(NODE_FUNC_DECL);
    strncpy(node->var_name, name, sizeof(node->var_name) - 1);
    node->params = params;
    node->param_count = param_count;
    node->func_body = body;
    return node;
}

ASTNode* create_func_call_node(const char *name, ASTNode **args, int arg_count) {
    ASTNode *node = create_ast_node(NODE_FUNC_CALL);
    strncpy(node->var_name, name, sizeof(node->var_name) - 1);
    node->args = args;
    node->arg_count = arg_count;
    return node;
}

ASTNode* create_return_node(ASTNode *expr) {
    ASTNode *node = create_ast_node(NODE_RETURN);
    node->left = expr;
    return node;
}

ASTNode* create_exit_node(ASTNode *expr) {
    ASTNode *node = create_ast_node(NODE_EXIT);
    node->left = expr;
    return node;
}

ASTNode* create_panic_node(ASTNode *expr) {
    ASTNode *node = create_ast_node(NODE_PANIC);
    node->left = expr;
    return node;
}

ASTNode* create_builtin_node(BuiltinKind kind, ASTNode **args, int arg_count) {
    ASTNode *node = create_ast_node(NODE_BUILTIN_CALL);
    node->builtin_kind = kind;
    node->args = args;
    node->arg_count = arg_count;
    return node;
}

void free_ast(ASTNode *node) {
    if (!node) return;
    
    if (node->type == NODE_BLOCK) {
        for (int i = 0; i < node->stmt_count; i++) {
            free_ast(node->statements[i]);
        }
        free(node->statements); 
        free(node);
        return;
    }

    if (node->type == NODE_FUNC_DECL) {
        for (int i = 0; i < node->param_count; i++) {
            free(node->params[i]);
        }
        free(node->params);
        free_ast(node->func_body);
        free(node);
        return;
    }

    if (node->type == NODE_FUNC_CALL || node->type == NODE_BUILTIN_CALL) {
        for (int i = 0; i < node->arg_count; i++) {
            free_ast(node->args[i]);
        }
        free(node->args);
        free(node);
        return;
    }

    if (node->type == NODE_FOR) {
        free_ast(node->for_init);
        free_ast(node->for_cond);
        free_ast(node->for_post);
        free_ast(node->for_body);
        free(node);
        return;
    }

    if (node->type == NODE_IF) {
        free_ast(node->cond);
        free_ast(node->then_branch);
        free_ast(node->else_branch);
        free(node);
        return;
    }

    if (node->type == NODE_WHILE) {
        free_ast(node->cond);
        free_ast(node->then_branch);
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
    long original_pos = ftell(in);
    rewind(in);

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
    printf("[Kaoru Compiler]: Security context initialized. Owner Hash: 0x%LLX\n", (unsigned long long)sec_ctx->hardware_hash);   

    fseek(in, original_pos, SEEK_SET);
}

/* =========================================================================
 * Expression & Statement Recursive Descent Parser
 * ========================================================================= */

static ASTNode* parse_builtin_call(FILE *in, Token *current_tok) {
    const char *name = current_tok->value;
    if (name[0] == '@') name++;

    BuiltinKind kind = resolve_builtin_kind(name);
    if ((int)kind == -1) {
        printf("[Kaoru Syntax Error Line %u]: Unknown builtin directive '@%s'\n", current_tok->line, name);
        return NULL;
    }

    *current_tok = next_token(in); // Consumes `@name`

    if (current_tok->type != TOKEN_LPAREN) {
        printf("[Kaoru Syntax Error Line %u]: Expected '(' after '@%s'\n", current_tok->line, name);
        return NULL;
    }
    *current_tok = next_token(in); // Consumes '('

    ASTNode **args = NULL;
    int arg_count = 0;
    int capacity = 0;

    if (current_tok->type != TOKEN_RPAREN) {
        while (1) {
            ASTNode *arg = parse_expression(in, current_tok);
            if (!arg) {
                for (int i = 0; i < arg_count; i++) free_ast(args[i]);
                free(args);
                return NULL;
            }

            if (arg_count >= capacity) {
                capacity = (capacity == 0) ? 4 : capacity * 2;
                ASTNode **new_args = realloc(args, sizeof(ASTNode*) * capacity);
                if (!new_args) { free(args); exit(1); }
                args = new_args;
            }
            args[arg_count++] = arg;

            if (current_tok->type == TOKEN_COMMA) {
                *current_tok = next_token(in); // Consumes ','
            } else {
                break;
            }
        }
    }

    if (current_tok->type != TOKEN_RPAREN) {
        printf("[Kaoru Syntax Error Line %u]: Expected ')' after builtin arguments\n", current_tok->line);
        for (int i = 0; i < arg_count; i++) free_ast(args[i]);
        free(args);
        return NULL;
    }
    *current_tok = next_token(in); // Consumes ')'

    return create_builtin_node(kind, args, arg_count);
}

static ASTNode* parse_primary(FILE *in, Token *current_tok) {
    // Check for '@' System Builtins used inside expressions
    if (current_tok->value[0] == '@') {
        const char *name = current_tok->value + 1;
        if ((int)resolve_builtin_kind(name) != -1) {
            return parse_builtin_call(in, current_tok);
        }
    }

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

        char name[32];
        strncpy(name, current_tok->value, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        *current_tok = next_token(in);

        // Function call detection inside expressions
        if (current_tok->type == TOKEN_LPAREN) {
            *current_tok = next_token(in); // consume '('
            ASTNode **args = NULL;
            int arg_count = 0, capacity = 0;

            if (current_tok->type != TOKEN_RPAREN) {
                while (1) {
                    ASTNode *arg = parse_expression(in, current_tok);
                    if (!arg) return NULL;

                    if (arg_count >= capacity) {
                        capacity = (capacity == 0) ? 4 : capacity * 2;
                        args = realloc(args, sizeof(ASTNode*) * capacity);
                    }
                    args[arg_count++] = arg;

                    if (current_tok->type == TOKEN_COMMA) {
                        *current_tok = next_token(in);
                    } else {
                        break;
                    }
                }
            }
            if (current_tok->type == TOKEN_RPAREN) {
                *current_tok = next_token(in); // consume ')'
            } else {
                printf("[Kaoru Syntax Error Line %u]: Expected ')' after function call arguments\n", current_tok->line);
                return NULL;
            }
            return create_func_call_node(name, args, arg_count);
        }

        return create_string_node(name);
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

static ASTNode* parse_statement_or_block(FILE *in, Token *current_tok) {
    if (current_tok->type == TOKEN_LBRACE) {
        *current_tok = next_token(in); 
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
            *current_tok = next_token(in); 
            
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

static ASTNode* parse_while_statement(FILE *in, Token *current_tok) {
    *current_tok = next_token(in); // Consume 'while'

    if (current_tok->type != TOKEN_LPAREN) {
        printf("[Kaoru Syntax Error Line %u]: Expected '(' after 'while'\n", current_tok->line);
        return NULL;
    }
    *current_tok = next_token(in); // Consume '('

    ASTNode *cond = parse_relational(in, current_tok);
    if (!cond) return NULL;
    
    if (current_tok->type != TOKEN_RPAREN) {
        printf("[Kaoru Syntax Error Line %u]: Expected ')' after while condition\n", current_tok->line);
        free_ast(cond);
        return NULL;
    }
    *current_tok = next_token(in); // Consume ')'

    ASTNode *body = parse_statement_or_block(in, current_tok);
    if (!body) {
        free_ast(cond);
        return NULL;
    }

    return create_while_node(cond, body);
}

static ASTNode* parse_for_statement(FILE *in, Token *current_tok) {
    *current_tok = next_token(in); 
    if (current_tok->type == TOKEN_LPAREN) {
        *current_tok = next_token(in);
    } else {
        printf("[Kaoru Syntax Error Line %u]: Expected '(' after 'for'\n", current_tok->line);
        return NULL;
    }

    ASTNode *node = create_ast_node(NODE_FOR);

    if (current_tok->type != TOKEN_SEMICOLON) {
        node->for_init = parse_statement(in, current_tok);
    } else {
        *current_tok = next_token(in); 
    }

    if (current_tok->type != TOKEN_SEMICOLON) {
        node->for_cond = parse_expression(in, current_tok);
    }
    if (current_tok->type == TOKEN_SEMICOLON) {
        *current_tok = next_token(in); 
    } else {
        printf("[Kaoru Syntax Error Line %u]: Expected ';' after for-condition\n", current_tok->line);
        free_ast(node);
        return NULL;
    }

    if (current_tok->type != TOKEN_RPAREN) {
        node->for_post = parse_expression(in, current_tok);
    }
    if (current_tok->type == TOKEN_RPAREN) {
        *current_tok = next_token(in); 
    } else {
        printf("[Kaoru Syntax Error Line %u]: Expected ')' after for-step\n", current_tok->line);
        free_ast(node);
        return NULL;
    }

    node->for_body = parse_statement_or_block(in, current_tok);
    return node;
}

static ASTNode* parse_func_decl(FILE *in, Token *current_tok) {
    *current_tok = next_token(in); // consume '@func' or 'TOKEN_FUNC'

    if (current_tok->type != TOKEN_IDENTIFIER) {
        printf("[Kaoru Syntax Error Line %u]: Expected function name\n", current_tok->line);
        return NULL;
    }

    char func_name[32];
    strncpy(func_name, current_tok->value, sizeof(func_name) - 1);
    func_name[sizeof(func_name) - 1] = '\0';
    *current_tok = next_token(in);

    if (current_tok->type != TOKEN_LPAREN) {
        printf("[Kaoru Syntax Error Line %u]: Expected '(' after function name\n", current_tok->line);
        return NULL;
    }
    *current_tok = next_token(in);

    char **params = NULL;
    int param_count = 0, capacity = 0;

    if (current_tok->type != TOKEN_RPAREN) {
        while (1) {
            if (current_tok->type != TOKEN_IDENTIFIER) {
                printf("[Kaoru Syntax Error Line %u]: Expected parameter name\n", current_tok->line);
                return NULL;
            }
            if (param_count >= capacity) {
                capacity = (capacity == 0) ? 4 : capacity * 2;
                params = realloc(params, sizeof(char*) * capacity);
            }
            params[param_count] = strdup(current_tok->value);
            param_count++;
            *current_tok = next_token(in);

            if (current_tok->type == TOKEN_COMMA) {
                *current_tok = next_token(in);
            } else {
                break;
            }
        }
    }

    if (current_tok->type == TOKEN_RPAREN) {
        *current_tok = next_token(in);
    } else {
        printf("[Kaoru Syntax Error Line %u]: Expected ')' after parameters\n", current_tok->line);
        return NULL;
    }

    ASTNode *body = parse_statement_or_block(in, current_tok);
    return create_func_decl_node(func_name, params, param_count, body);
}

/* Consolidated Main Statement Router */
static ASTNode* parse_statement(FILE *in, Token *current_tok) {
    // 1. Standalone Scope Block `{ ... }`
    if (current_tok->type == TOKEN_LBRACE) {
        return parse_statement_or_block(in, current_tok);
    }

    // 2. Control Flow Directives
    if (current_tok->type == TOKEN_IF) {
        return parse_if_statement(in, current_tok);
    }
    if (current_tok->type == TOKEN_WHILE) {
        return parse_while_statement(in, current_tok);
    }
    if (current_tok->type == TOKEN_FOR) {
        return parse_for_statement(in, current_tok);
    }

    // 3. Function Declarations & Return
    if (current_tok->type == TOKEN_AT_FUNC || current_tok->type == TOKEN_FUNC || strcmp(current_tok->value, "@func") == 0) {
        return parse_func_decl(in, current_tok);
    }
    if (current_tok->type == TOKEN_RETURN || strcmp(current_tok->value, "return") == 0) {
        *current_tok = next_token(in);
        ASTNode *expr = parse_expression(in, current_tok);
        if (current_tok->type == TOKEN_SEMICOLON) {
            *current_tok = next_token(in);
        }
        return create_return_node(expr);
    }

    // 4. Print Directive
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

    // 5. Variable Declarations (@int, @str, @bool)
    if (current_tok->type == TOKEN_AT_INT || 
        current_tok->type == TOKEN_AT_STR || 
        current_tok->type == TOKEN_AT_BOOL) {
        
        MTokenType var_type = current_tok->type;
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

        return create_var_decl_node(var_type, var_name, expr);
    }

    // 6. System Primitives Directives (@write, @close, @free, etc.)
    if (current_tok->value[0] == '@') {
        const char *name = current_tok->value + 1;
        if ((int)resolve_builtin_kind(name) != -1) {
            ASTNode *builtin = parse_builtin_call(in, current_tok);
            if (current_tok->type == TOKEN_SEMICOLON) {
                *current_tok = next_token(in);
            }
            return builtin;
        }
    }

    if (current_tok->type == TOKEN_AT_EXIT) {
        *current_tok = next_token(in);
        ASTNode *expr = parse_expression(in, current_tok);
        if (current_tok->type == TOKEN_SEMICOLON) *current_tok = next_token(in);
        return create_exit_node(expr);
    }
    if (current_tok->type == TOKEN_AT_PANIC) {
        *current_tok = next_token(in);
        ASTNode *expr = parse_expression(in, current_tok);
        if (current_tok->type == TOKEN_SEMICOLON) *current_tok = next_token(in);
        return create_panic_node(expr);
    }
    if (current_tok->type == TOKEN_AT_SYS) {
        *current_tok = next_token(in);
        if (current_tok->type == TOKEN_SEMICOLON) *current_tok = next_token(in);
        return NULL;
    }

    // 7. Expression Statement
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
        } else { 
            tok = next_token(in); 
        }
    }
    return create_block_node(stmts, count);
}