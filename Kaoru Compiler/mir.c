// mir.c - Lowering AST to MIR (Medium-level Intermediate Representation)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mir.h"

/* Formula 2: Hardware 8-Byte Alignment Operator */
size_t align8(size_t x) {
    return (x + 7) & ~((size_t)7);
}

/* Formula 3: Frame Allocation Metric Function */
size_t size_of_scope(ASTNode *block) {
    if (!block || block->type != NODE_BLOCK) {
        return 0;
    }
    size_t total_size = 0;
    for (size_t i = 0; i < block->child_count; i++) {
        ASTNode *child = block->children[i];
        if (child->type == NODE_VAR_DECL) {
            total_size += (child->var_size > 0) ? (size_t)child->var_size : 8;
        }
    }
    return align8(total_size);
}

static IRInstruction *create_ir_inst(IROpcode op) {
    IRInstruction *inst = (IRInstruction *)calloc(1, sizeof(IRInstruction));
    if (!inst) {
        fprintf(stderr, "Error: Memory allocation failed for IRInstruction\n");
        exit(EXIT_FAILURE);
    }
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

static void new_temp(IRProgram *prog, char *out_buf, size_t buf_size) {
    snprintf(out_buf, buf_size, "t%d", prog->temp_count++);
}

static void new_label(IRProgram *prog, const char *prefix, char *out_buf, size_t buf_size) {
    snprintf(out_buf, buf_size, "%s_%d", prefix, prog->label_count++);
}

/* Operational Semantics State Transition Lowering */
void lower_ast_node(ASTNode *node, IRProgram *prog, ScopeContext *ctx, int32_t *current_offset, char *out_target, size_t target_size) {
    if (!node) return;

    switch (node->type) {

    /* Formula 4 & Theorem 1: Scope Block Handling with Stack Invariant */
    case NODE_BLOCK: {
        int32_t scope_size = (int32_t)size_of_scope(node);
        int32_t saved_offset = *current_offset;

        /* [E-ENTER] Rule: Allocate frame */
        IRInstruction *enter = create_ir_inst(IR_ENTER_SCOPE);
        enter->stack_offset = scope_size;
        append_inst(prog, enter);

        /* [E-EXEC] Rule: Process statements sequentially inside scope bound */
        for (size_t i = 0; i < node->child_count; i++) {
            lower_ast_node(node->children[i], prog, ctx, current_offset, NULL, 0);
        }

        /* [E-EXIT] Rule: Restore stack pointer to preserve invariant sp_exit = sp0 */
        IRInstruction *exit_inst = create_ir_inst(IR_EXIT_SCOPE);
        exit_inst->stack_offset = scope_size;
        append_inst(prog, exit_inst);

        *current_offset = saved_offset;
        break;
    }

    /* Lemma 2: Scope Memory Isolation Bound Variable Allocation */
    case NODE_VAR_DECL: {
        int v_size = (node->var_size > 0) ? node->var_size : 8;
        *current_offset += v_size;

        IRInstruction *decl = create_ir_inst(IR_VAR_DECL);
        strncpy(decl->target, node->name, sizeof(decl->target) - 1);
        decl->stack_offset = *current_offset;
        append_inst(prog, decl);
        break;
    }

    case NODE_VAR_REF: {
        if (out_target && target_size > 0) {
            strncpy(out_target, node->name, target_size - 1);
        }
        break;
    }

    case NODE_LITERAL: {
        if (out_target && target_size > 0) {
            new_temp(prog, out_target, target_size);
            IRInstruction *assign = create_ir_inst(IR_ASSIGN);
            strncpy(assign->target, out_target, sizeof(assign->target) - 1);
            assign->imm_val = node->int_val;
            append_inst(prog, assign);
        }
        break;
    }

    case NODE_ASSIGN: {
        char val_target[32] = {0};
        lower_ast_node(node->val, prog, ctx, current_offset, val_target, sizeof(val_target));

        if (node->target->type == NODE_DEREF) {
            /* Pointer dereference store: *ptr = val */
            char ptr_target[32] = {0};
            lower_ast_node(node->target->val, prog, ctx, current_offset, ptr_target, sizeof(ptr_target));

            IRInstruction *store = create_ir_inst(IR_STORE_PTR);
            strncpy(store->target, ptr_target, sizeof(store->target) - 1);
            strncpy(store->arg1, val_target, sizeof(store->arg1) - 1);
            append_inst(prog, store);
        } else {
            /* Standard assignment: var = val */
            IRInstruction *assign = create_ir_inst(IR_ASSIGN);
            strncpy(assign->target, node->target->name, sizeof(assign->target) - 1);
            strncpy(assign->arg1, val_target, sizeof(assign->arg1) - 1);
            append_inst(prog, assign);
        }
        break;
    }

    case NODE_BINOP: {
        char left_target[32] = {0};
        char right_target[32] = {0};
        lower_ast_node(node->left, prog, ctx, current_offset, left_target, sizeof(left_target));
        lower_ast_node(node->right, prog, ctx, current_offset, right_target, sizeof(right_target));

        if (out_target && target_size > 0) {
            new_temp(prog, out_target, target_size);
            IROpcode op = IR_ADD;
            if (strcmp(node->op, "-") == 0) op = IR_SUB;
            else if (strcmp(node->op, "*") == 0) op = IR_MUL;
            else if (strcmp(node->op, "/") == 0) op = IR_DIV;

            IRInstruction *bin = create_ir_inst(op);
            strncpy(bin->target, out_target, sizeof(bin->target) - 1);
            strncpy(bin->arg1, left_target, sizeof(bin->arg1) - 1);
            strncpy(bin->arg2, right_target, sizeof(right_target) - 1);
            append_inst(prog, bin);
        }
        break;
    }

    /* Small-step Deterministic Control Flow */
    case NODE_IF: {
        char cond_target[32] = {0};
        char label_else[32] = {0};
        char label_end[32] = {0};

        new_label(prog, "L_else", label_else, sizeof(label_else));
        new_label(prog, "L_end", label_end, sizeof(label_end));

        lower_ast_node(node->cond, prog, ctx, current_offset, cond_target, sizeof(cond_target));

        IRInstruction *br = create_ir_inst(IR_JUMP_IF_FALSE);
        strncpy(br->arg1, cond_target, sizeof(br->arg1) - 1);
        strncpy(br->target, node->else_block ? label_else : label_end, sizeof(br->target) - 1);
        append_inst(prog, br);

        lower_ast_node(node->then_block, prog, ctx, current_offset, NULL, 0);

        if (node->else_block) {
            IRInstruction *jmp = create_ir_inst(IR_JUMP);
            strncpy(jmp->target, label_end, sizeof(jmp->target) - 1);
            append_inst(prog, jmp);

            IRInstruction *lbl_else = create_ir_inst(IR_LABEL);
            strncpy(lbl_else->target, label_else, sizeof(lbl_else->target) - 1);
            append_inst(prog, lbl_else);

            lower_ast_node(node->else_block, prog, ctx, current_offset, NULL, 0);
        }

        IRInstruction *lbl_end = create_ir_inst(IR_LABEL);
        strncpy(lbl_end->target, label_end, sizeof(lbl_end->target) - 1);
        append_inst(prog, lbl_end);
        break;
    }

    case NODE_WHILE: {
        char label_start[32] = {0};
        char label_end[32] = {0};
        char cond_target[32] = {0};

        new_label(prog, "L_loop_start", label_start, sizeof(label_start));
        new_label(prog, "L_loop_end", label_end, sizeof(label_end));

        IRInstruction *lbl_start = create_ir_inst(IR_LABEL);
        strncpy(lbl_start->target, label_start, sizeof(lbl_start->target) - 1);
        append_inst(prog, lbl_start);

        lower_ast_node(node->cond, prog, ctx, current_offset, cond_target, sizeof(cond_target));

        IRInstruction *br = create_ir_inst(IR_JUMP_IF_FALSE);
        strncpy(br->arg1, cond_target, sizeof(br->arg1) - 1);
        strncpy(br->target, label_end, sizeof(br->target) - 1);
        append_inst(prog, br);

        ScopeContext loop_ctx;
        strncpy(loop_ctx.break_label, label_end, sizeof(loop_ctx.break_label) - 1);
        loop_ctx.parent = ctx;

        lower_ast_node(node->body, prog, &loop_ctx, current_offset, NULL, 0);

        IRInstruction *jmp = create_ir_inst(IR_JUMP);
        strncpy(jmp->target, label_start, sizeof(jmp->target) - 1);
        append_inst(prog, jmp);

        IRInstruction *lbl_end = create_ir_inst(IR_LABEL);
        strncpy(lbl_end->target, label_end, sizeof(lbl_end->target) - 1);
        append_inst(prog, lbl_end);
        break;
    }

    case NODE_BREAK: {
        if (ctx && ctx->break_label[0] != '\0') {
            IRInstruction *jmp = create_ir_inst(IR_JUMP);
            strncpy(jmp->target, ctx->break_label, sizeof(jmp->target) - 1);
            append_inst(prog, jmp);
        }
        break;
    }

    case NODE_RETURN: {
        char ret_target[32] = {0};
        if (node->val) {
            lower_ast_node(node->val, prog, ctx, current_offset, ret_target, sizeof(ret_target));
        }
        IRInstruction *ret = create_ir_inst(IR_RETURN);
        strncpy(ret->arg1, ret_target, sizeof(ret->arg1) - 1);
        append_inst(prog, ret);
        break;
    }

    /* Self-Hosting Primitives: Pointer Operations & Calls */
    case NODE_ADDR_OF: {
        if (out_target && target_size > 0) {
            new_temp(prog, out_target, target_size);
            IRInstruction *addr = create_ir_inst(IR_ADDR_OF);
            strncpy(addr->target, out_target, sizeof(addr->target) - 1);
            strncpy(addr->arg1, node->val->name, sizeof(addr->arg1) - 1);
            append_inst(prog, addr);
        }
        break;
    }

    case NODE_DEREF: {
        char ptr_target[32] = {0};
        lower_ast_node(node->val, prog, ctx, current_offset, ptr_target, sizeof(ptr_target));

        if (out_target && target_size > 0) {
            new_temp(prog, out_target, target_size);
            IRInstruction *load = create_ir_inst(IR_LOAD_PTR);
            strncpy(load->target, out_target, sizeof(load->target) - 1);
            strncpy(load->arg1, ptr_target, sizeof(load->arg1) - 1);
            append_inst(prog, load);
        }
        break;
    }

    case NODE_CALL: {
        for (size_t i = 0; i < node->arg_count; i++) {
            char arg_target[32] = {0};
            lower_ast_node(node->args[i], prog, ctx, current_offset, arg_target, sizeof(arg_target));
            
            IRInstruction *param = create_ir_inst(IR_PARAM);
            strncpy(param->arg1, arg_target, sizeof(param->arg1) - 1);
            append_inst(prog, param);
        }

        if (out_target && target_size > 0) {
            new_temp(prog, out_target, target_size);
        }

        IRInstruction *call = create_ir_inst(IR_CALL);
        if (out_target && target_size > 0) {
            strncpy(call->target, out_target, sizeof(call->target) - 1);
        }
        strncpy(call->arg1, node->name, sizeof(call->arg1) - 1);
        call->imm_val = (int32_t)node->arg_count;
        append_inst(prog, call);
        break;
    }
    }
}