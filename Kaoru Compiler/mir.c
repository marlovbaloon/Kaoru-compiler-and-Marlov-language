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

static void new_temp(IRProgram *prog, char *out_temp, size_t max_len) {
    snprintf(out_temp, max_len, "t%d", prog->temp_count++);
}

static void new_label(IRProgram *prog, char *out_label, size_t max_len) {
    snprintf(out_label, max_len, "L%d", prog->label_count++);
}

/* Recursive traversal to accurately aggregate raw variable declarations size */
static size_t calculate_scope_size(ASTNode *node) {
    if (!node) return 0;
    
    size_t raw_size = 0;
    if (node->type == NODE_VAR_DECL) {
        raw_size += 4; /* Standard 32-bit primitive storage size */
    } else if (node->type == NODE_BLOCK) {
        for (int i = 0; i < node->stmt_count; i++) {
            raw_size += calculate_scope_size(node->statements[i]);
        }
    } else if (node->type == NODE_IF) {
        raw_size += calculate_scope_size(node->then_branch);
        if (node->else_branch) {
            raw_size += calculate_scope_size(node->else_branch);
        }
    }
    return raw_size;
}

/* 
 * Safe Recursive AST Lowering
 * Requires out_target to be passed down to prevent static memory corruption
 */
static void lower_ast_rec(ASTNode *node, IRProgram *prog, int *current_offset, char *out_target, size_t out_size) {
    if (!node) {
        if (out_target && out_size > 0) out_target[0] = '\0';
        return;
    }

    switch (node->type) {
        case NODE_INT: {
            new_temp(prog, out_target, out_size);
            IRInstruction *inst = create_ir_inst(IR_ASSIGN);
            strncpy(inst->target, out_target, sizeof(inst->target) - 1);
            inst->imm_val = node->val;
            append_inst(prog, inst);
            break;
        }

        case NODE_ADD:
        case NODE_SUB:
        case NODE_MUL:
        case NODE_DIV: {
            char left_sym[32] = {0};
            char right_sym[32] = {0};

            lower_ast_rec(node->left, prog, current_offset, left_sym, sizeof(left_sym));
            lower_ast_rec(node->right, prog, current_offset, right_sym, sizeof(right_sym));

            new_temp(prog, out_target, out_size);
            IROpcode op = (node->type == NODE_ADD) ? IR_ADD :
                          (node->type == NODE_SUB) ? IR_SUB :
                          (node->type == NODE_MUL) ? IR_MUL : IR_DIV;

            IRInstruction *inst = create_ir_inst(op);
            strncpy(inst->target, out_target, sizeof(inst->target) - 1);
            strncpy(inst->arg1, left_sym, sizeof(inst->arg1) - 1);
            strncpy(inst->arg2, right_sym, sizeof(inst->arg2) - 1);
            append_inst(prog, inst);
            break;
        }

        case NODE_BLOCK: {
            size_t raw_size = calculate_scope_size(node);
            size_t aligned_scope_size = align8(raw_size);
            int local_offset_tracker = 0;

            IRInstruction *enter = create_ir_inst(IR_ENTER_SCOPE);
            enter->imm_val = (int)aligned_scope_size;
            append_inst(prog, enter);

            for (int i = 0; i < node->stmt_count; i++) {
                lower_ast_rec(node->statements[i], prog, &local_offset_tracker, NULL, 0);
            }

            IRInstruction *exit_inst = create_ir_inst(IR_EXIT_SCOPE);
            exit_inst->imm_val = (int)aligned_scope_size;
            append_inst(prog, exit_inst);

            if (out_target && out_size > 0) out_target[0] = '\0';
            break;
        }

        case NODE_VAR_DECL: {
            char val_sym[32] = {0};
            lower_ast_rec(node->left, prog, current_offset, val_sym, sizeof(val_sym));

            IRInstruction *inst = create_ir_inst(IR_VAR_DECL);
            strncpy(inst->target, node->var_name, sizeof(inst->target) - 1);
            strncpy(inst->arg1, val_sym, sizeof(inst->arg1) - 1);

            if (current_offset) {
                inst->stack_offset = *current_offset;
                *current_offset += 4;
            }
            append_inst(prog, inst);
            
            if (out_target && out_size > 0) out_target[0] = '\0';
            break;
        }

        case NODE_IF: {
            char else_label[32], end_label[32], cond_sym[32] = {0};
            new_label(prog, else_label, sizeof(else_label));
            new_label(prog, end_label, sizeof(end_label));

            lower_ast_rec(node->cond, prog, current_offset, cond_sym, sizeof(cond_sym));

            IRInstruction *jif = create_ir_inst(IR_JUMP_IF_FALSE);
            strncpy(jif->arg1, cond_sym, sizeof(jif->arg1) - 1);
            strncpy(jif->target, node->else_branch ? else_label : end_label, sizeof(jif->target) - 1);
            append_inst(prog, jif);

            lower_ast_rec(node->then_branch, prog, current_offset, NULL, 0);

            if (node->else_branch) {
                IRInstruction *jmp = create_ir_inst(IR_JUMP);
                strncpy(jmp->target, end_label, sizeof(jmp->target) - 1);
                append_inst(prog, jmp);

                IRInstruction *lbl_else = create_ir_inst(IR_LABEL);
                strncpy(lbl_else->target, else_label, sizeof(lbl_else->target) - 1);
                append_inst(prog, lbl_else);

                lower_ast_rec(node->else_branch, prog, current_offset, NULL, 0);
            }

            IRInstruction *lbl_end = create_ir_inst(IR_LABEL);
            strncpy(lbl_end->target, end_label, sizeof(lbl_end->target) - 1);
            append_inst(prog, lbl_end);

            if (out_target && out_size > 0) out_target[0] = '\0';
            break;
        }

        case PRINT_NODE: {
            char val_sym[32] = {0};
            lower_ast_rec(node->left, prog, current_offset, val_sym, sizeof(val_sym));
            
            IRInstruction *inst = create_ir_inst(IR_PRINT);
            strncpy(inst->arg1, val_sym, sizeof(inst->arg1) - 1);
            append_inst(prog, inst);

            if (out_target && out_size > 0) out_target[0] = '\0';
            break;
        }

        default:
            if (out_target && out_size > 0) out_target[0] = '\0';
            break;
    }
}

IRProgram* generate_ir(ASTNode *root) {
    IRProgram *prog = (IRProgram *)calloc(1, sizeof(IRProgram));
    if (!prog) exit(1);
    
    int root_offset = 0;
    char root_target[32] = {0};
    lower_ast_rec(root, prog, &root_offset, root_target, sizeof(root_target));
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