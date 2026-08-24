// mir.h
#ifndef MIR_H
#define MIR_H

#include <stdint.h>
#include <stddef.h>
#include "mtypes.h"

/* IR Instruction Opcodes */
typedef enum {
    IR_ENTER_SCOPE, /* Align8 Frame Allocation [ENTER] */
    IR_EXIT_SCOPE,  /* Scope Stack Cleanup [EXIT] */
    IR_VAR_DECL,    /* Local Variable Local Offset Assign */
    IR_ASSIGN,      /* Value Store */
    IR_ADD, IR_SUB, IR_MUL, IR_DIV,
    IR_PRINT,
    IR_JUMP, IR_JUMP_IF_FALSE,
    IR_LABEL
} IROpcode;

typedef struct IRInstruction {
    IROpcode op;
    char target[32];   /* Variable name or Temporary register */
    char arg1[32];     /* Operand 1 */
    char arg2[32];     /* Operand 2 */
    int imm_val;       /* Immediate value or Scope Frame Size N */
    int stack_offset;  /* Addr(v) within [sp0 - N, sp0) */
    struct IRInstruction *next;
} IRInstruction;

typedef struct {
    IRInstruction *head;
    IRInstruction *tail;
    int temp_count;
    int label_count;
} IRProgram;

/* Mathematical Helper Function (Align8 Operator Definition) */
static inline size_t align8(size_t x) {
    return (x + 7) & ~((size_t)7); /* Align8(x) = (x + 7) AND NOT 7 */
}

IRProgram* generate_ir(ASTNode *root);
void free_ir(IRProgram *ir);
void print_ir(IRProgram *ir);

#endif /* MIR_H */