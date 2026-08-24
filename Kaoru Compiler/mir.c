// mir.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mir.h"

static IRInstruction* create_ir_inst(IROpcode op) {
    IRInstruction *inst = (IRInstruction *)calloc(1, sizeof(IRInstruction));
    if (!inst) exit(1);
    inst->op = op;
    return inst;
}

static void append_inst(IRProgram *prog, IRInstruction *inst) {
    if (!prog->head) {
        prog->head = inst;
        prog->tail = inst;
    } else {
        prog->tail->next = inst;
        prog->tail = inst;
    }
}

/* Calculate SizeOfScope(B) = Align8( Sum(sizeof(v)) ) */
static size_t calculate_scope_size(ASTNode *block_node) {
    size_t raw_size = 0;
    for (int i = 0; i < block_node->stmt_count; i++) {
        ASTNode *stmt = block_node->statements[i];
        if (stmt && stmt->type == NODE_VAR_DECL) {
            raw_size += 4; /* สมมติให้ Primitive Data Type (Int/Bool) มีขนาด 4 Bytes */
        }
    }
    return align8(raw_size); /* การันตี Lemma 1: Align8 Preserved */
}

/* Recursive AST to IR Lowering */
static void lower_ast(ASTNode *node, IRProgram *prog) {
    if (!node) return;

    switch (node->type) {
        case NODE_BLOCK: {
            size_t scope_size = calculate_scope_size(node);
            
            // 1. [ENTER] Scope Rule
            IRInstruction *enter = create_ir_inst(IR_ENTER_SCOPE);
            enter->imm_val = (int)scope_size;
            append_inst(prog, enter);

            // 2. [EXEC] Scope Body Inner AST Induction
            for (int i = 0; i < node->stmt_count; i++) {
                lower_ast(node->statements[i], prog);
            }

            // 3. [EXIT] Scope Rule (Theorem 1 Preservation)
            IRInstruction *exit_inst = create_ir_inst(IR_EXIT_SCOPE);
            exit_inst->imm_val = (int)scope_size;
            append_inst(prog, exit_inst);
            break;
        }

        case NODE_VAR_DECL: {
            lower_ast(node->left, prog);
            IRInstruction *inst = create_ir_inst(IR_VAR_DECL);
            strncpy(inst->target, node->var_name, sizeof(inst->target) - 1);
            append_inst(prog, inst);
            break;
        }

        case PRINT_NODE: {
            lower_ast(node->left, prog);
            IRInstruction *inst = create_ir_inst(IR_PRINT);
            append_inst(prog, inst);
            break;
        }

        default:
            break;
    }
}

IRProgram* generate_ir(ASTNode *root) {
    IRProgram *prog = (IRProgram *)calloc(1, sizeof(IRProgram));
    if (!prog) exit(1);
    lower_ast(root, prog);
    return prog;
}

void free_ir(IRProgram *ir) {
    if (!ir) return;
    IRInstruction *curr = ir->head;
    while (curr) {
        IRInstruction *next = curr->next;
        free(curr);
        curr = next;
    }
    free(ir);
}