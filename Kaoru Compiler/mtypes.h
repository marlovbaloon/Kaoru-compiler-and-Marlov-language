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
    TOKEN_AND, TOKEN_OR, TOKEN_NOT, TOKEN_IF, TOKEN_ELSE, TOKEN_WHILE, TOKEN_RETURN
} TokenType;

typedef enum  { NODE_INT, NODE_ADD,NODE_VAR_DECl} NodeType;

typedef struct ASTNode{
    NodeType type;
    int val;
    char var_name[32];
    struct ASTNode *left;
    struct ASTNode *right;
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