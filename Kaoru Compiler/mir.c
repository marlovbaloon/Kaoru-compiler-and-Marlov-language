//  mir.c
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

static void new_temp(IRProgram *prog, char *out_temp) {
    snprintf(out_temp, 32, "t%d", prog->temp_count++);
}

static void new_label(IRProgram *prog, char *out_label) {
    snprintf(out_label, 32, "L%d", prog->label_count++);
}

/* คำนวณ SizeOfScope(B) = Align8( Sum(sizeof(v)) ) ตามสมการที่ 3 */
static size_t calculate_scope_size(ASTNode *block_node) {
    size_t raw_size = 0;
    for (int i = 0; i < block_node->stmt_count; i++) {
        ASTNode *stmt = block_node->statements[i];
        if (stmt && stmt->type == NODE_VAR_DECL) {
            raw_size += 4; /* บน 32-bit ARM Cortex-M Primitive data มีขนาด 4 ไบต์ */
        }
    }
    return align8(raw_size); /* อนุรักษ์การจัดเรียง 8 ไบต์ (Lemma 1) */
}

/* Recursive AST to IR Lowering */
static char* lower_ast(ASTNode *node, IRProgram *prog, int *current_offset) {
    if (!node) return NULL;

    static char ret_target[32];
    memset(ret_target, 0, sizeof(ret_target));

    switch (node->type) {
        case NODE_INT: {
            new_temp(prog, ret_target);
            IRInstruction *inst = create_ir_inst(IR_ASSIGN);
            strncpy(inst->target, ret_target, sizeof(inst->target) - 1);
            inst->imm_val = node->val;
            append_inst(prog, inst);
            return ret_target;
        }

        case NODE_ADD:
        case NODE_SUB:
        case NODE_MUL:
        case NODE_DIV: {
            char *left = lower_ast(node->left, prog, current_offset);
            char left_sym[32];
            strncpy(left_sym, left ? left : "", sizeof(left_sym) - 1);

            char *right = lower_ast(node->right, prog, current_offset);
            char right_sym[32];
            strncpy(right_sym, right ? right : "", sizeof(right_sym) - 1);

            new_temp(prog, ret_target);
            IROpcode op = (node->type == NODE_ADD) ? IR_ADD :
                          (node->type == NODE_SUB) ? IR_SUB :
                          (node->type == NODE_MUL) ? IR_MUL : IR_DIV;

            IRInstruction *inst = create_ir_inst(op);
            strncpy(inst->target, ret_target, sizeof(inst->target) - 1);
            strncpy(inst->arg1, left_sym, sizeof(inst->arg1) - 1);
            strncpy(inst->arg2, right_sym, sizeof(inst->arg2) - 1);
            append_inst(prog, inst);
            return ret_target;
        }

        case NODE_BLOCK: {
            size_t scope_size = calculate_scope_size(node);
            int local_offset_tracker = 0; // ออฟเซตเริ่มต้นภายในเฟรม [sp0 - N, sp0)

            // 1. [ENTER] Scope Rule (สมการที่ 4)
            IRInstruction *enter = create_ir_inst(IR_ENTER_SCOPE);
            enter->imm_val = (int)scope_size;
            append_inst(prog, enter);

            // 2. [EXEC] Scope Body Inner AST Induction
            for (int i = 0; i < node->stmt_count; i++) {
                lower_ast(node->statements[i], prog, &local_offset_tracker);
            }

            // 3. [EXIT] Scope Rule (ทฤษฎีบทที่ 1)
            IRInstruction *exit_inst = create_ir_inst(IR_EXIT_SCOPE);
            exit_inst->imm_val = (int)scope_size;
            append_inst(prog, exit_inst);
            return NULL;
        }

        case NODE_VAR_DECL: {
            char *val_sym = lower_ast(node->left, prog, current_offset);

            IRInstruction *inst = create_ir_inst(IR_VAR_DECL);
            strncpy(inst->target, node->var_name, sizeof(inst->target) - 1);
            if (val_sym) strncpy(inst->arg1, val_sym, sizeof(inst->arg1) - 1);
            
            /* กำหนดตำแหน่ง Stack Offset ตาม Lemma 2 */
            if (current_offset) {
                inst->stack_offset = *current_offset;
                *current_offset += 4; // ถัดไปอีก 4 ไบต์
            }
            append_inst(prog, inst);
            return NULL;
        }

        case NODE_IF: {
            char else_label[32], end_label[32];
            new_label(prog, else_label);
            new_label(prog, end_label);

            char *cond_sym = lower_ast(node->cond, prog, current_offset);

            IRInstruction *jif = create_ir_inst(IR_JUMP_IF_FALSE);
            if (cond_sym) strncpy(jif->arg1, cond_sym, sizeof(jif->arg1) - 1);
            strncpy(jif->target, node->else_branch ? else_label : end_label, sizeof(jif->target) - 1);
            append_inst(prog, jif);

            lower_ast(node->then_branch, prog, current_offset);

            if (node->else_branch) {
                IRInstruction *jmp = create_ir_inst(IR_JUMP);
                strncpy(jmp->target, end_label, sizeof(jmp->target) - 1);
                append_inst(prog, jmp);

                IRInstruction *lbl_else = create_ir_inst(IR_LABEL);
                strncpy(lbl_else->target, else_label, sizeof(lbl_else->target) - 1);
                append_inst(prog, lbl_else);

                lower_ast(node->else_branch, prog, current_offset);
            }

            IRInstruction *lbl_end = create_ir_inst(IR_LABEL);
            strncpy(lbl_end->target, end_label, sizeof(lbl_end->target) - 1);
            append_inst(prog, lbl_end);
            return NULL;
        }

        case PRINT_NODE: {
            char *val_sym = lower_ast(node->left, prog, current_offset);
            IRInstruction *inst = create_ir_inst(IR_PRINT);
            if (val_sym) strncpy(inst->arg1, val_sym, sizeof(inst->arg1) - 1);
            append_inst(prog, inst);
            return NULL;
        }

        default:
            return NULL;
    }
}

IRProgram* generate_ir(ASTNode *root) {
    IRProgram *prog = (IRProgram *)calloc(1, sizeof(IRProgram));
    if (!prog) exit(1);
    int root_offset = 0;
    lower_ast(root, prog, &root_offset);
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