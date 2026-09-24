// mir.c - Lowering AST to Medium-Level Intermediate Representation (MIR)
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "mir.h"
#include "mtypes.h"

/* Forward declaration for ScopeContext structure */
typedef struct ScopeContext ScopeContext;
struct ScopeContext {
    char break_label[32];
    char continue_label[32];
    ScopeContext *parent;
};

/* Formula 2: Hardware 8-Byte Alignment Operator */
size_t align8(size_t x) {
    return (x + 7) & ~((size_t)7);
}

/* Formula 3: Frame Allocation Metric Function */
size_t size_of_scope(ASTNode *block) {
    if (!block) return 0;
    
    size_t total_size = 0;

    /* Inspect primary statements array from parser */
    if (block->statements && block->stmt_count > 0) {
        for (int i = 0; i < block->stmt_count; i++) {
            ASTNode *child = block->statements[i];
            if (child && child->type == NODE_VAR_DECL) {
                size_t v_size = (child->val > 0) ? (size_t)child->val : 8;
                total_size += v_size;
            }
        }
    }

    return align8(total_size);
}

static IRInstruction *create_ir_inst(IROpcode op) {
    IRInstruction *inst = (IRInstruction *)calloc(1, sizeof(IRInstruction));
    if (!inst) {
        fprintf(stderr, "[Kaoru Fatal Error]: Memory allocation failed for IRInstruction\n");
        exit(EXIT_FAILURE);
    }
    inst->op = op;
    return inst;
}

static void append_inst(IRProgram *prog, IRInstruction *inst) {
    if (!prog || !inst) return;
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

/* Forward Declaration for Recursive Lowering */
void lower_ast_node(ASTNode *node, IRProgram *prog, ScopeContext *ctx, int32_t *current_offset, char *out_target, size_t target_size);

/* Operational Semantics State Transition Lowering */
void lower_ast_node(ASTNode *node, IRProgram *prog, ScopeContext *ctx, int32_t *current_offset, char *out_target, size_t target_size) {
    if (!node || !prog) return;

    switch (node->type) {

    /* Formula 4 & Theorem 1: Scope Block Handling with Stack Invariant */
    case NODE_BLOCK: {
        int32_t scope_size = (int32_t)size_of_scope(node);
        int32_t saved_offset = *current_offset;

        /* [E-ENTER] Rule: Allocate frame explicitly with Align8 sizing */
        IRInstruction *enter = create_ir_inst(IR_ENTER_SCOPE);
        enter->stack_offset = scope_size;
        enter->imm_val = scope_size;
        append_inst(prog, enter);

        /* [E-EXEC] Rule: Process statements sequentially inside scope bound */
        if (node->statements && node->stmt_count > 0) {
            for (int i = 0; i < node->stmt_count; i++) {
                lower_ast_node(node->statements[i], prog, ctx, current_offset, NULL, 0);
            }
        }

        /* [E-EXIT] Rule: Restore stack pointer to preserve sp_exit = sp0 invariant */
        IRInstruction *exit_inst = create_ir_inst(IR_EXIT_SCOPE);
        exit_inst->stack_offset = scope_size;
        exit_inst->imm_val = scope_size;
        append_inst(prog, exit_inst);

        /* Reclaim local stack offset space for sibling scopes */
        *current_offset = saved_offset;
        break;
    }

    /* Lemma 2: Variable Declaration and Memory Offset Assignment */
    case NODE_VAR_DECL: {
        int v_size = (node->val > 0) ? node->val : (node->is_byte_op ? 1 : 8);
        *current_offset += v_size;

        IRInstruction *decl = create_ir_inst(IR_VAR_DECL);
        strncpy(decl->target, node->var_name, sizeof(decl->target) - 1);
        decl->stack_offset = *current_offset;
        decl->imm_val = v_size;
        append_inst(prog, decl);

        /* Handle Initializer Assignment if Present */
        if (node->left) {
            char val_target[32] = {0};
            lower_ast_node(node->left, prog, ctx, current_offset, val_target, sizeof(val_target));

            IRInstruction *assign = create_ir_inst(IR_ASSIGN);
            strncpy(assign->target, node->var_name, sizeof(assign->target) - 1);
            strncpy(assign->arg1, val_target, sizeof(assign->arg1) - 1);
            append_inst(prog, assign);
        }
        break;
    }

    case NODE_INT:
    case NODE_BOOL: {
        if (out_target && target_size > 0) {
            new_temp(prog, out_target, target_size);
            IRInstruction *assign = create_ir_inst(IR_ASSIGN);
            strncpy(assign->target, out_target, sizeof(assign->target) - 1);
            assign->imm_val = node->val;
            snprintf(assign->arg1, sizeof(assign->arg1), "%d", node->val);
            append_inst(prog, assign);
        }
        break;
    }

    case NODE_STR: {
        if (out_target && target_size > 0) {
            new_temp(prog, out_target, target_size);
            IRInstruction *assign = create_ir_inst(IR_ASSIGN);
            strncpy(assign->target, out_target, sizeof(assign->target) - 1);
            strncpy(assign->arg1, node->str_val, sizeof(assign->arg1) - 1);
            append_inst(prog, assign);
        }
        break;
    }

    case NODE_VAR_REF: {
        if (out_target && target_size > 0) {
            strncpy(out_target, node->var_name, target_size - 1);
        }
        break;
    }

    case NODE_ADD:
    case NODE_SUB:
    case NODE_MUL:
    case NODE_DIV:
    case NODE_BIT_AND:
    case NODE_BIT_OR:
    case NODE_BIT_XOR:
    case NODE_SHL:
    case NODE_SHR:
    case NODE_EQ:
    case NODE_NEQ:
    case NODE_LT:
    case NODE_GT:
    case NODE_LTE:
    case NODE_GTE: {
        char left_target[32] = {0};
        char right_target[32] = {0};
        lower_ast_node(node->left, prog, ctx, current_offset, left_target, sizeof(left_target));
        lower_ast_node(node->right, prog, ctx, current_offset, right_target, sizeof(right_target));

        if (out_target && target_size > 0) {
            new_temp(prog, out_target, target_size);
            IROpcode op = IR_ADD;
            switch (node->type) {
                case NODE_ADD: op = IR_ADD; break;
                case NODE_SUB: op = IR_SUB; break;
                case NODE_MUL: op = IR_MUL; break;
                case NODE_DIV: op = IR_DIV; break;
                default: op = IR_ADD; break;
            }

            IRInstruction *bin = create_ir_inst(op);
            strncpy(bin->target, out_target, sizeof(bin->target) - 1);
            strncpy(bin->arg1, left_target, sizeof(bin->arg1) - 1);
            strncpy(bin->arg2, right_target, sizeof(bin->arg2) - 1);
            append_inst(prog, bin);
        }
        break;
    }

    case NODE_IF: {
        char cond_target[32] = {0};
        char label_else[32] = {0};
        char label_end[32] = {0};

        new_label(prog, "L_else", label_else, sizeof(label_else));
        new_label(prog, "L_end", label_end, sizeof(label_end));

        lower_ast_node(node->cond, prog, ctx, current_offset, cond_target, sizeof(cond_target));

        IRInstruction *br = create_ir_inst(IR_JUMP_IF_FALSE);
        strncpy(br->arg1, cond_target, sizeof(br->arg1) - 1);
        strncpy(br->target, node->else_branch ? label_else : label_end, sizeof(br->target) - 1);
        append_inst(prog, br);

        lower_ast_node(node->then_branch, prog, ctx, current_offset, NULL, 0);

        if (node->else_branch) {
            IRInstruction *jmp = create_ir_inst(IR_JUMP);
            strncpy(jmp->target, label_end, sizeof(jmp->target) - 1);
            append_inst(prog, jmp);

            IRInstruction *lbl_else = create_ir_inst(IR_LABEL);
            strncpy(lbl_else->target, label_else, sizeof(lbl_else->target) - 1);
            append_inst(prog, lbl_else);

            lower_ast_node(node->else_branch, prog, ctx, current_offset, NULL, 0);
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
        strncpy(loop_ctx.continue_label, label_start, sizeof(loop_ctx.continue_label) - 1);
        loop_ctx.parent = ctx;

        lower_ast_node(node->then_branch ? node->then_branch : node->body, prog, &loop_ctx, current_offset, NULL, 0);

        IRInstruction *jmp = create_ir_inst(IR_JUMP);
        strncpy(jmp->target, label_start, sizeof(jmp->target) - 1);
        append_inst(prog, jmp);

        IRInstruction *lbl_end = create_ir_inst(IR_LABEL);
        strncpy(lbl_end->target, label_end, sizeof(lbl_end->target) - 1);
        append_inst(prog, lbl_end);
        break;
    }

    case NODE_FOR: {
        char label_start[32] = {0};
        char label_end[32] = {0};
        char cond_target[32] = {0};

        new_label(prog, "L_for_start", label_start, sizeof(label_start));
        new_label(prog, "L_for_end", label_end, sizeof(label_end));

        if (node->for_init) {
            lower_ast_node(node->for_init, prog, ctx, current_offset, NULL, 0);
        }

        IRInstruction *lbl_start = create_ir_inst(IR_LABEL);
        strncpy(lbl_start->target, label_start, sizeof(lbl_start->target) - 1);
        append_inst(prog, lbl_start);

        if (node->for_cond) {
            lower_ast_node(node->for_cond, prog, ctx, current_offset, cond_target, sizeof(cond_target));
            IRInstruction *br = create_ir_inst(IR_JUMP_IF_FALSE);
            strncpy(br->arg1, cond_target, sizeof(br->arg1) - 1);
            strncpy(br->target, label_end, sizeof(br->target) - 1);
            append_inst(prog, br);
        }

        ScopeContext loop_ctx;
        strncpy(loop_ctx.break_label, label_end, sizeof(label_end));
        strncpy(loop_ctx.continue_label, label_start, sizeof(loop_ctx.continue_label));
        loop_ctx.parent = ctx;

        if (node->for_body) {
            lower_ast_node(node->for_body, prog, &loop_ctx, current_offset, NULL, 0);
        }

        if (node->for_post) {
            lower_ast_node(node->for_post, prog, &loop_ctx, current_offset, NULL, 0);
        }

        IRInstruction *jmp = create_ir_inst(IR_JUMP);
        strncpy(jmp->target, label_start, sizeof(jmp->target) - 1);
        append_inst(prog, jmp);

        IRInstruction *lbl_end = create_ir_inst(IR_LABEL);
        strncpy(lbl_end->target, label_end, sizeof(lbl_end->target) - 1);
        append_inst(prog, lbl_end);
        break;
    }

    case NODE_FUNC_DECL: {
        IRInstruction *lbl = create_ir_inst(IR_LABEL);
        strncpy(lbl->target, node->var_name, sizeof(lbl->target) - 1);
        append_inst(prog, lbl);

        int32_t func_offset = 0;
        if (node->func_body) {
            lower_ast_node(node->func_body, prog, ctx, &func_offset, NULL, 0);
        }
        break;
    }

    case NODE_FUNC_CALL:
    case NODE_BUILTIN_CALL: {
        for (int i = 0; i < node->arg_count; i++) {
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
        strncpy(call->arg1, node->var_name, sizeof(call->arg1) - 1);
        call->imm_val = node->arg_count;
        append_inst(prog, call);
        break;
    }

    case PRINT_NODE: {
        char val_target[32] = {0};
        lower_ast_node(node->left, prog, ctx, current_offset, val_target, sizeof(val_target));

        IRInstruction *prt = create_ir_inst(IR_PRINT);
        strncpy(prt->arg1, val_target, sizeof(prt->arg1) - 1);
        append_inst(prog, prt);
        break;
    }

    case NODE_RETURN: {
        char ret_target[32] = {0};
        if (node->left) {
            lower_ast_node(node->left, prog, ctx, current_offset, ret_target, sizeof(ret_target));
        }
        IRInstruction *ret = create_ir_inst(IR_RETURN);
        strncpy(ret->arg1, ret_target, sizeof(ret->arg1) - 1);
        append_inst(prog, ret);
        break;
    }

    case NODE_ADDR_OF: {
        if (out_target && target_size > 0) {
            new_temp(prog, out_target, target_size);
            IRInstruction *addr = create_ir_inst(IR_ADDR_OF);
            strncpy(addr->target, out_target, sizeof(addr->target) - 1);
            if (node->left) {
                strncpy(addr->arg1, node->left->var_name, sizeof(addr->arg1) - 1);
            }
            append_inst(prog, addr);
        }
        break;
    }

    case NODE_DEREF: {
        char ptr_target[32] = {0};
        if (node->left) {
            lower_ast_node(node->left, prog, ctx, current_offset, ptr_target, sizeof(ptr_target));
        }

        if (out_target && target_size > 0) {
            new_temp(prog, out_target, target_size);
            IRInstruction *load = create_ir_inst(IR_LOAD_PTR);
            strncpy(load->target, out_target, sizeof(load->target) - 1);
            strncpy(load->arg1, ptr_target, sizeof(load->arg1) - 1);
            append_inst(prog, load);
        }
        break;
    }

    default:
        break;
    }
}

IRProgram* generate_ir(ASTNode *root) {
    IRProgram *prog = (IRProgram *)calloc(1, sizeof(IRProgram));
    if (!prog) {
        fprintf(stderr, "[Kaoru Fatal Error]: Allocation failed for IRProgram\n");
        exit(EXIT_FAILURE);
    }
    
    int32_t initial_offset = 0;
    lower_ast_node(root, prog, NULL, &initial_offset, NULL, 0);
    return prog;
}

void free_ir(IRProgram *prog) {
    if (!prog) return;

    IRInstruction *curr = prog->head;
    while (curr) {
        IRInstruction *next = curr->next;
        free(curr);
        curr = next;
    }

    free(prog);
}

void print_ir(IRProgram *prog) {
    if (!prog) return;

    IRInstruction *curr = prog->head;
    while (curr) {
        switch (curr->op) {
            case IR_ENTER_SCOPE:
                printf("    ENTER_SCOPE (Frame Size: %d bytes)\n", curr->imm_val);
                break;
            case IR_EXIT_SCOPE:
                printf("    EXIT_SCOPE  (Frame Size: %d bytes)\n", curr->imm_val);
                break;
            case IR_VAR_DECL:
                printf("    VAR_DECL    %s (Stack Offset: -%d)\n", curr->target, curr->stack_offset);
                break;
            case IR_ASSIGN:
                printf("    %s = %s\n", curr->target, curr->arg1[0] ? curr->arg1 : "imm");
                break;
            case IR_ADD:
                printf("    %s = %s + %s\n", curr->target, curr->arg1, curr->arg2);
                break;
            case IR_SUB:
                printf("    %s = %s - %s\n", curr->target, curr->arg1, curr->arg2);
                break;
            case IR_MUL:
                printf("    %s = %s * %s\n", curr->target, curr->arg1, curr->arg2);
                break;
            case IR_DIV:
                printf("    %s = %s / %s\n", curr->target, curr->arg1, curr->arg2);
                break;
            case IR_LABEL:
                printf("%s:\n", curr->target);
                break;
            case IR_JUMP:
                printf("    JUMP        %s\n", curr->target);
                break;
            case IR_JUMP_IF_FALSE:
                printf("    JUMP_IF_FALSE %s -> %s\n", curr->arg1, curr->target);
                break;
            case IR_PRINT:
                printf("    PRINT       %s\n", curr->arg1);
                break;
            case IR_RETURN:
                printf("    RETURN      %s\n", curr->arg1);
                break;
            case IR_PARAM:
                printf("    PARAM       %s\n", curr->arg1);
                break;
            case IR_CALL:
                printf("    %s = CALL %s (%d args)\n", curr->target[0] ? curr->target : "_", curr->arg1, curr->imm_val);
                break;
            case IR_ADDR_OF:
                printf("    %s = &%s\n", curr->target, curr->arg1);
                break;
            case IR_LOAD_PTR:
                printf("    %s = *%s\n", curr->target, curr->arg1);
                break;
            case IR_STORE_PTR:
                printf("    *%s = %s\n", curr->target, curr->arg1);
                break;
            default:
                printf("    IR_OP_%d\n", curr->op);
                break;
        }
        curr = curr->next;
    }
}