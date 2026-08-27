// mtypes.h
#ifndef MTYPES_H
#define MTYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* Bit-flags for Gatekeeper permission control (Permission Bitmask) */
#define PERM_NONE       0x00
#define PERM_DISK_READ  (1 << 0)  /* 0x01 */
#define PERM_DISK_WRITE (1 << 1)  /* 0x02 */
#define PERM_NET_CONN   (1 << 2)  /* 0x04 */

typedef enum {
    TOKEN_EOF, TOKEN_AT_SYS, TOKEN_AT_IMPORT, TOKEN_AT_FUNC,
    TOKEN_AT_INT, TOKEN_AT_STR, TOKEN_AT_BOOL, TOKEN_AT_TBL,
    TOKEN_AT_PRINT, TOKEN_AT_DEBUG, TOKEN_IDENTIFIER, TOKEN_NUMBER,
    TOKEN_STRING_LIT, TOKEN_LBRACE, TOKEN_RBRACE, TOKEN_LPAREN,
    TOKEN_RPAREN, TOKEN_LBRACKET,TOKEN_LBRACKET ,TOKEN_SEMICOLON, TOKEN_ASSIGN, /* '=' */
    TOKEN_PLUS, TOKEN_MINUS, TOKEN_STAR, TOKEN_SLASH,
    TOKEN_AND, TOKEN_OR, TOKEN_NOT, TOKEN_IF, TOKEN_ELSE, TOKEN_WHILE, TOKEN_RETURN, TOKEN_FOR, TOKEN_COMMA, TOKEN_COLON,
    TOKEN_EQ,       /* == */
    TOKEN_NEQ,      /* != */
    TOKEN_LT,       /* < */
    TOKEN_GT,       /* > */
    TOKEN_LTE,      /* <= */
    TOKEN_GTE,      /* >= */
    TOKEN_AT_EXIT,
    TOKEN_AT_PANIC,
    TOKEN_FUNC,

    /* --- Extended Tokens for Self-Hosting Capabilities --- */
    TOKEN_AMP,      /* '&' Address-of / Bitwise AND */
    TOKEN_PIPE,     /* '|' Bitwise OR */
    TOKEN_CARET,    /* '^' Bitwise XOR */
    TOKEN_LSHIFT,   /* '<<' Bitwise Shift Left */
    TOKEN_RSHIFT,   /* '>>' Bitwise Shift Right */
    TOKEN_ARROW,    /* '->' Member Access Pointer */
    TOKEN_DOT,      /* '.' Member Access Direct */
    TOKEN_LBRACKET, /* '[' Array/Pointer Index */
    TOKEN_RBRACKET, /* ']' */
    TOKEN_AT_CHAR,  /* '@char' 8-bit Byte Type */
    TOKEN_AT_PTR    /* '@ptr' Generic Pointer Type */
} MTokenType;

typedef enum { 
    NODE_INT, NODE_ADD, NODE_VAR_DECL, 
    NODE_MUL, NODE_DIV, NODE_SUB,
    NODE_STR, NODE_BOOL, PRINT_NODE, NODE_IF,
    NODE_BLOCK,     /* AST_SCOPE_BLOCK */
    NODE_EQ,        /* == */
    NODE_NEQ,       /* != */
    NODE_LT,        /* < */
    NODE_GT,        /* > */
    NODE_LTE,       /* <= */
    NODE_GTE,       /* >= */
    NODE_VAR_REF, 
    NODE_WHILE,
    NODE_FOR,
    NODE_FUNC_DECL,
    NODE_FUNC_CALL,
    NODE_RETURN,
    NODE_EXIT,
    NODE_PANIC,
    NODE_BUILTIN_CALL,

    /* --- Extended AST Nodes for Self-Hosting Capabilities --- */
    NODE_CHAR,       /* 8-bit Byte Literal */
    NODE_DEREF,      /* Dereference (*ptr or ptr[index]) */
    NODE_ADDR_OF,    /* Address-of (&var) */
    NODE_MEMBER_REF, /* Struct Field Access (ptr->field or struct.field) */
    NODE_ASSIGN_PTR, /* Store via Pointer (*ptr = val) */
    NODE_BIT_AND,    /* Bitwise AND (&) */
    NODE_BIT_OR,     /* Bitwise OR (|) */
    NODE_BIT_XOR,    /* Bitwise XOR (^) */
    NODE_SHL,        /* Shift Left (<<) */
    NODE_SHR         /* Shift Right (>>) */
} ASTNodeType;     

/* Alias NodeType */
typedef ASTNodeType NodeType;

typedef enum {
    BUILTIN_OPEN,
    BUILTIN_READ,
    BUILTIN_WRITE,
    BUILTIN_CLOSE,
    BUILTIN_ALLOC,
    BUILTIN_FREE,
    BUILTIN_SIZEOF,
    BUILTIN_EXIT,
    BUILTIN_PANIC,

    /* --- Extended Memory Primitives --- */
    BUILTIN_LOAD8,   /* Load 1 Byte (movzx / ldrb) */
    BUILTIN_STORE8   /* Store 1 Byte (mov byte / strb) */
} BuiltinKind;

typedef struct ASTNode {
    ASTNodeType type;
    MTokenType data_type;          /* Variable data type */
    BuiltinKind builtin_kind;       /* System primitive kind */
    
    int val;
    char str_val[68];
    char var_name[32];
    
    /* Binary & Control Tree Links */
    struct ASTNode *left;
    struct ASTNode *right;
    struct ASTNode *cond;          /* condition for if/while */
    struct ASTNode *then_branch;   /* if true */
    struct ASTNode *else_branch;   /* if false */

    /* Pointer & Member Access Links */
    struct ASTNode *ptr;           /* Pointer expression to dereference */
    struct ASTNode *offset;        /* Array index or byte offset calculation */
    char member_name[32];          /* Struct field identifier */
    int member_offset;             /* Calculated memory byte offset for struct fields */
    bool is_byte_op;               /* Flag: true if 8-bit (char), false if 64-bit (int/ptr) */

    /* Loop Structure */
    struct ASTNode *for_init;
    struct ASTNode *for_cond;
    struct ASTNode *for_post;
    struct ASTNode *for_body;
    struct ASTNode *body;

    /* Block Statements */
    struct ASTNode **statements;    
    int stmt_count;                

    /* Function Declaration & Calls */
    char **params;
    int param_count;
    struct ASTNode *func_body;
    struct ASTNode **args;
    int arg_count;
} ASTNode;

typedef struct ASTArena {
    ASTNode *nodes;
    size_t capacity;
    size_t count;
} ASTArena;

typedef struct {
    MTokenType type;              
    char value[64];
    uint32_t line;
} Token;

typedef enum {
    SYM_FUNCTION,
    SYM_VARIABLE,
    SYM_STRUCT
} SymbolKind;

typedef struct Symbol {
    char name[64];
    SymbolKind kind;
    bool is_declared;  /* True if loaded from .mlov */
    bool is_defined;   /* True if body implemented in .ml */
    int stack_offset;  /* Local variable stack frame offset relative to RBP/FP */
    size_t data_size;  /* Data size: 1 byte (char), 8 bytes (int/ptr) */
    struct Symbol *next;
} Symbol;

typedef struct {
    Symbol *head;
} SymbolTable;

typedef struct {
    uint8_t permissions;  
    uint64_t hardware_hash; 
} SecurityContext;

/* Forward Declaration for permission system */
uint64_t get_hardware_signature(void);

#endif /* MTYPES_H */
