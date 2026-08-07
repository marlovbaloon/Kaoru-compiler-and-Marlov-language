//mtypes.h
#ifndef MTYPES_H
#define MTYPES_H

#include <stdint.h>
#include <stdbool.h>

/* Bit-flags for Gatekeeper permission control (Permission Bitmask). */
#define PERM_NONE       0x00
#define PERM_DISK_READ  (1 << 0)  /* 0x01 */
#define PERM_DISK_WRITE (1 << 1)  /* 0x02 */
#define PERM_NET_CONN   (1 << 2)  /* 0x04 */

typedef enum {
    TOKEN_EOF, TOKEN_AT_SYS, TOKEN_AT_IMPORT, TOKEN_AT_FUNC,
    TOKEN_AT_INT, TOKEN_AT_STR, TOKEN_AT_BOOL, TOKEN_AT_TBL,
    TOKEN_AT_PRINT, TOKEN_AT_DEBUG, TOKEN_IDENTIFIER, TOKEN_NUMBER,
    TOKEN_STRING_LIT, TOKEN_LBRACE, TOKEN_RBRACE, TOKEN_LPAREN,
    TOKEN_RPAREN, TOKEN_SEMICOLON, TOKEN_ASSIGN, TOKEN_EQUAL,
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH,
    TOKEN_AND, TOKEN_OR, TOKEN_NOT, TOKEN_IF, TOKEN_ELSE, TOKEN_WHILE, TOKEN_RETURN,
    TOKEN_EQ,       /* == */
    TOKEN_NEQ,      /* != */
    TOKEN_LT,       /* < */
    TOKEN_GT,       /* > */
    TOKEN_LTE,      /* <= */
    TOKEN_GTE,      /* >= */
} MTokenType;

typedef enum  { 
    NODE_INT, NODE_ADD,NODE_VAR_DECL, 
    NODE_MUL, NODE_DIV, NODE_SUB,
    NODE_STR, NODE_BOOL,PRINT_NODE, NODE_IF,
    NODE_BLOCK,
    NODE_EQ,       /* == */
    NODE_NEQ,      /* != */
    NODE_LT,       /* < */
    NODE_GT,       /* > */
    NODE_LTE,      /* <= */
    NODE_GTE       /* >= */

}NodeType;

typedef struct ASTNode{
    NodeType type;
    int val;
    char str_val[67];
    char var_name[32];
    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *cond; //conditon if
    struct ASTNode *then_branch; //if true
    struct ASTNode *else_branch; //if false (else, else if)
    struct ASTNode **statement; // node block
    int stm_count; 
} ASTNode;

typedef struct {
    TokenType type;
    char value[64];
    uint32_t line;
} Token;

typedef struct {
    uint8_t permissions; /* Bitmask for permission require from program */
    uint64_t hardware_hash; /* Hash made from Motherboard/CPU */
} SecurityContext;

#endif