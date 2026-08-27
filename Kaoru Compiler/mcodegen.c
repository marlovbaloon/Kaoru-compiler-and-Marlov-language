// mcodegen.c - Marlov Compiler Code Generation Module
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "mtypes.h"

/* =========================================================================
 * Forward Declarations & Helpers
 * ========================================================================= */
void generate_code_from_ast(FILE *out, ASTNode *node, SecurityContext *sec_ctx);
void generate_print_asm(FILE *out, ASTNode *node, SecurityContext *sec_ctx);
void generate_if_asm(FILE *out, ASTNode *node, SecurityContext *sec_ctx);
void generate_block_asm(FILE *out, ASTNode *node, SecurityContext *sec_ctx);
void generate_binary_op_asm(FILE *out, ASTNode *node, SecurityContext *sec_ctx);
void generate_builtin_call_asm(FILE *out, ASTNode *node, SecurityContext *sec_ctx);
void generate_while_asm(FILE *out, ASTNode *node, SecurityContext *sec_ctx);
void generate_for_asm(FILE *out, ASTNode *node, SecurityContext *sec_ctx);
void generate_func_decl_asm(FILE *out, ASTNode *node, SecurityContext *sec_ctx);
void generate_func_call_asm(FILE *out, ASTNode *node, SecurityContext *sec_ctx);
void generate_return_asm(FILE *out, ASTNode *node, SecurityContext *sec_ctx);

static int label_counter = 0;
static int str_label_counter = 0;

void emit_symbol_linkage(FILE *out, SymbolTable *symtab) {
    if (!symtab) return;
    Symbol *curr = symtab->head;
    while (curr) {
        if (curr->is_declared && !curr->is_defined) {
            /* Declared in .mlov but not defined in this .ml -> Mark as EXTERN for Assembly */
            fprintf(out, "    .extern %s\n", curr->name);
        } else if (curr->is_defined) {
            /* Defined in this .ml -> Export as GLOBAL */
            fprintf(out, "    .global %s\n", curr->name);
        }
        curr = curr->next;
    }
}

/* Registers for ABI argument passing */
#if defined(__x86_64__) || defined(_M_X64)
static const char *ARG_REGS[] = {"rdi", "rsi", "rdx", "rcx", "r8", "r9"};
#elif defined(__aarch64__) || defined(_M_ARM64)
static const char *ARG_REGS[] = {"x0", "x1", "x2", "x3", "x4", "x5"};
#endif

/* =========================================================================
 * Runtime Helpers (C-Level Abstraction)
 * ========================================================================= */
void mlov_print_int(int64_t val) {
    printf("%ld\n", val);
}

void mlov_print_str(const char *val) {
    printf("%s\n", val);
}

/* =========================================================================
 * Cross-Platform Hardware Signature Generator (Host/Target Helper)
 * ========================================================================= */
uint64_t get_hardware_signature(void) {
    uint64_t signature = 0;

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
    __asm__ __volatile__ (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );
    signature = ((uint64_t)eax << 32) | (ebx ^ ecx ^ edx);

#elif defined(__3DS__) || defined(_3DS)
    uint32_t cpuid_reg = 0;
    uint32_t proc_id = 0;
    __asm__ __volatile__ ("mrc p15, 0, %0, c0, c0, 0" : "=r"(cpuid_reg));
    __asm__ __volatile__ ("mrc p15, 0, %0, c13, c0, 1" : "=r"(proc_id));
    signature = ((uint64_t)cpuid_reg << 32) | (proc_id ^ 0x3D53D53D);

#elif defined(__aarch64__) || defined(_M_ARM64)
    uint64_t midr = 0;
    __asm__ __volatile__ ("mrs %0, midr_el1" : "=r"(midr));
    signature = midr ^ 0xA64A64A64A64A64ULL;

#elif defined(__arm__) || defined(_M_ARM)
    uint32_t midr = 0;
    __asm__ __volatile__ ("mrc p15, 0, %0, c0, c0, 0" : "=r"(midr));
    signature = ((uint64_t)midr << 32) | (midr ^ 0x41524D37);

#else
    signature = 0x4D41524C4F563231ULL; /* "MARLOV21" ASCII Hash fallback */
#endif

    return signature;
}

/* =========================================================================
 * Security Gatekeeper Assembly Output (Target Binary Routine)
 * ========================================================================= */
void generate_runtime_header(FILE *out, SecurityContext *sec_ctx) {
    fprintf(out, "// --- KAORU RUNTIME SECURITY GUARD (EMBEDDED IN TARGET BINARY) ---\n");
    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "#include <stdlib.h>\n");
    fprintf(out, "#include <stdbool.h>\n\n");
    
    if (sec_ctx) {
        fprintf(out, "static const unsigned long long AUTHORIZED_HARDWARE_HASH = 0x%llXULL;\n", 
                (unsigned long long)sec_ctx->hardware_hash);
        fprintf(out, "static const bool HAS_DISK_READ_PERM = %s;\n\n", 
                (sec_ctx->permissions & PERM_DISK_READ) ? "true" : "false");
    }

    fprintf(out, "void verify_hardware_and_permissions() {\n");
    fprintf(out, "    unsigned long long current_cpuid = get_hardware_signature();\n");
    fprintf(out, "    if (AUTHORIZED_HARDWARE_HASH != 0 && current_cpuid != AUTHORIZED_HARDWARE_HASH) {\n");
    fprintf(out, "        printf(\"[Marlov Runtime Error]: Hardware signature mismatch! Unauthorized device.\\n\");\n");
    fprintf(out, "        exit(137);\n");
    fprintf(out, "    }\n");
    fprintf(out, "}\n\n");
}

void generate_assembly_entry(FILE *out, SecurityContext *ctx) {
    (void)ctx; /* Unused here: No longer trap during compilation */

#if defined(__x86_64__) || defined(_M_X64)
    fprintf(out, ".global main\n");
    fprintf(out, "main:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    fprintf(out, "    ; Optional: Call Runtime Hardware Guard Check\n");
    fprintf(out, "    ; call verify_hardware_and_permissions\n");
#elif defined(__aarch64__) || defined(_M_ARM64)
    fprintf(out, ".global main\n");
    fprintf(out, "main:\n");
    fprintf(out, "    stp x29, x30, [sp, #-16]!\n");
    fprintf(out, "    mov x29, sp\n");
    fprintf(out, "    ; Optional: Call Runtime Hardware Guard Check\n");
    fprintf(out, "    ; bl verify_hardware_and_permissions\n");
#endif
}

/* =========================================================================
 * AST Code Generator Dispatcher
 * ========================================================================= */
void generate_code_from_ast(FILE *out, ASTNode *node, SecurityContext *sec_ctx) {
    if (!node) return;

    switch (node->type) {
        case NODE_INT:
#if defined(__x86_64__) || defined(_M_X64)
            fprintf(out, "    mov rax, %d\n", node->val);
#elif defined(__aarch64__) || defined(_M_ARM64)
            fprintf(out, "    mov x0, #%d\n", node->val);
#endif
            break;

        case NODE_BOOL:
#if defined(__x86_64__) || defined(_M_X64)
            fprintf(out, "    mov rax, %d\n", node->val ? 1 : 0);
#elif defined(__aarch64__) || defined(_M_ARM64)
            fprintf(out, "    mov x0, #%d\n", node->val ? 1 : 0);
#endif
            break;

        case NODE_STR: {
            int str_id = str_label_counter++;
            fprintf(out, ".section .rodata\n");
            fprintf(out, ".LC_str_%d:\n", str_id);
            fprintf(out, "    .string \"%s\"\n", node->str_val);
            fprintf(out, ".text\n");
#if defined(__x86_64__) || defined(_M_X64)
            fprintf(out, "    lea rax, .LC_str_%d[rip]\n", str_id);
#elif defined(__aarch64__) || defined(_M_ARM64)
            fprintf(out, "    adrp x0, .LC_str_%d\n", str_id);
            fprintf(out, "    add x0, x0, :lo12:.LC_str_%d\n", str_id);
#endif
            break;
        }

        case NODE_VAR_REF:
            fprintf(out, "    ; Symbol reference: %s\n", node->var_name);
            break;

        case NODE_ADD:
        case NODE_SUB:
        case NODE_MUL:
        case NODE_DIV:
        case NODE_EQ:
        case NODE_NEQ:
        case NODE_LT:
        case NODE_GT:
        case NODE_LTE:
        case NODE_GTE:
            generate_binary_op_asm(out, node, sec_ctx);
            break;

        case NODE_VAR_DECL:
            generate_code_from_ast(out, node->left, sec_ctx);
            fprintf(out, "    ; Variable Decl: %s initialized\n", node->var_name);
            break;

        case NODE_BLOCK:
            generate_block_asm(out, node, sec_ctx);
            break;

        case NODE_IF:
            generate_if_asm(out, node, sec_ctx);
            break;

        case NODE_WHILE:
            generate_while_asm(out, node, sec_ctx);
            break;

        case NODE_FOR:
            generate_for_asm(out, node, sec_ctx);
            break;

        case NODE_FUNC_DECL:
            generate_func_decl_asm(out, node, sec_ctx);
            break;

        case NODE_FUNC_CALL:
            generate_func_call_asm(out, node, sec_ctx);
            break;

        case NODE_RETURN:
            generate_return_asm(out, node, sec_ctx);
            break;

        case PRINT_NODE:
            generate_print_asm(out, node, sec_ctx);
            break;

        case NODE_BUILTIN_CALL:
            generate_builtin_call_asm(out, node, sec_ctx);
            break;

        case NODE_EXIT:
            fprintf(out, "    ; --- @exit Statement ---\n");
            generate_code_from_ast(out, node->left, sec_ctx); 
#if defined(__x86_64__) || defined(_M_X64)
            fprintf(out, "    mov rdi, rax\n");
            fprintf(out, "    call exit\n");
#elif defined(__aarch64__) || defined(_M_ARM64)
            fprintf(out, "    bl exit\n");
#endif
            break;

        case NODE_PANIC:
            fprintf(out, "    ; --- @panic Statement ---\n");
            generate_code_from_ast(out, node->left, sec_ctx);
#if defined(__x86_64__) || defined(_M_X64)
            fprintf(out, "    mov rdi, rax\n");
            fprintf(out, "    call mlov_print_str\n");
            fprintf(out, "    ud2\n");
#elif defined(__aarch64__) || defined(_M_ARM64)
            fprintf(out, "    bl mlov_print_str\n");
            fprintf(out, "    .word 0xd4200000\n");
#endif
            break;

        default:
            fprintf(out, "    ; [CodeGen]: Unhandled AST Node type (%d)\n", node->type);
            break;
    }
}

/* =========================================================================
 * Generate Assembly for Function Declarations, Calls & Returns
 * ========================================================================= */
void generate_func_decl_asm(FILE *out, ASTNode *node, SecurityContext *sec_ctx) {
    if (!node || node->type != NODE_FUNC_DECL) return;

    fprintf(out, "\n.global %s\n", node->var_name);
    fprintf(out, "%s:\n", node->var_name);

#if defined(__x86_64__) || defined(_M_X64)
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
#elif defined(__aarch64__) || defined(_M_ARM64)
    fprintf(out, "    stp x29, x30, [sp, #-16]!\n");
    fprintf(out, "    mov x29, sp\n");
#endif

    fprintf(out, "    ; Bind parameters (%d count)\n", node->param_count);
    for (int i = 0; i < node->param_count; i++) {
        if (i < 6) {
            fprintf(out, "    ; param [%s] passed via %s\n", node->params[i], ARG_REGS[i]);
        }
    }

    if (node->func_body) {
        generate_code_from_ast(out, node->func_body, sec_ctx);
    }

#if defined(__x86_64__) || defined(_M_X64)
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n");
#elif defined(__aarch64__) || defined(_M_ARM64)
    fprintf(out, "    ldp x29, x30, [sp], #16\n");
    fprintf(out, "    ret\n");
#endif
}

void generate_func_call_asm(FILE *out, ASTNode *node, SecurityContext *sec_ctx) {
    if (!node || node->type != NODE_FUNC_CALL) return;

    fprintf(out, "    ; --- Function Call: %s (%d args) ---\n", node->var_name, node->arg_count);

    /* Evaluate args and push onto stack */
    for (int i = 0; i < node->arg_count; i++) {
        generate_code_from_ast(out, node->args[i], sec_ctx);
#if defined(__x86_64__) || defined(_M_X64)
        fprintf(out, "    push rax\n");
#elif defined(__aarch64__) || defined(_M_ARM64)
        fprintf(out, "    str x0, [sp, #-16]!\n");
#endif
    }

    /* Pop evaluation results into target register locations according to ABI */
    for (int i = node->arg_count - 1; i >= 0; i--) {
        if (i < 6) {
#if defined(__x86_64__) || defined(_M_X64)
            fprintf(out, "    pop %s\n", ARG_REGS[i]);
#elif defined(__aarch64__) || defined(_M_ARM64)
            fprintf(out, "    ldr %s, [sp], #16\n", ARG_REGS[i]);
#endif
        }
    }

#if defined(__x86_64__) || defined(_M_X64)
    fprintf(out, "    call %s\n", node->var_name);
#elif defined(__aarch64__) || defined(_M_ARM64)
    fprintf(out, "    bl %s\n", node->var_name);
#endif
}

void generate_return_asm(FILE *out, ASTNode *node, SecurityContext *sec_ctx) {
    if (!node || node->type != NODE_RETURN) return;

    fprintf(out, "    ; --- Return Statement ---\n");
    if (node->left) {
        generate_code_from_ast(out, node->left, sec_ctx);
    }

#if defined(__x86_64__) || defined(_M_X64)
    fprintf(out, "    mov rsp, rbp\n");
    fprintf(out, "    pop rbp\n");
    fprintf(out, "    ret\n");
#elif defined(__aarch64__) || defined(_M_ARM64)
    fprintf(out, "    ldp x29, x30, [sp], #16\n");
    fprintf(out, "    ret\n");
#endif
}

/* =========================================================================
 * Generate Assembly for Builtin Directives (@open, @read, @write, etc.)
 * ========================================================================= */
void generate_builtin_call_asm(FILE *out, ASTNode *node, SecurityContext *sec_ctx) {
    if (!node || node->type != NODE_BUILTIN_CALL) return;

    fprintf(out, "    ; --- Directives Call: Builtin %d ---\n", node->builtin_kind);

    /* 1. Special Handling: Builtins that do not follow standard C call flow */
    if (node->builtin_kind == BUILTIN_SIZEOF) {
        fprintf(out, "    ; Builtin sizeof evaluation\n");
#if defined(__x86_64__) || defined(_M_X64)
        fprintf(out, "    mov rax, 8\n");
#elif defined(__aarch64__) || defined(_M_ARM64)
        fprintf(out, "    mov x0, #8\n");
#endif
        return;
    }

    if (node->builtin_kind == BUILTIN_PANIC) {
        if (node->arg_count > 0) {
            generate_code_from_ast(out, node->args[0], sec_ctx);
        }
#if defined(__x86_64__) || defined(_M_X64)
        fprintf(out, "    mov rdi, rax\n");
        fprintf(out, "    call mlov_print_str\n");
        fprintf(out, "    ud2\n");
#elif defined(__aarch64__) || defined(_M_ARM64)
        fprintf(out, "    bl mlov_print_str\n");
        fprintf(out, "    .word 0xd4200000\n");
#endif
        return;
    }

    /* 2. Standard C-Call Builtins: Push arguments */
    for (int i = 0; i < node->arg_count; i++) {
        generate_code_from_ast(out, node->args[i], sec_ctx);
#if defined(__x86_64__) || defined(_M_X64)
        fprintf(out, "    push rax\n");
#elif defined(__aarch64__) || defined(_M_ARM64)
        fprintf(out, "    str x0, [sp, #-16]!\n");
#endif
    }

    /* 3. Pop arguments into ABI registers */
    for (int i = node->arg_count - 1; i >= 0; i--) {
        if (i < 6) {
#if defined(__x86_64__) || defined(_M_X64)
            fprintf(out, "    pop %s\n", ARG_REGS[i]);
#elif defined(__aarch64__) || defined(_M_ARM64)
            fprintf(out, "    ldr %s, [sp], #16\n", ARG_REGS[i]);
#endif
        }
    }

    /* 4. Resolve Target Function Name */
    const char *target_func = NULL;
    switch (node->builtin_kind) {
        case BUILTIN_OPEN:   target_func = "fopen"; break;
        case BUILTIN_READ:   target_func = "fread"; break;
        case BUILTIN_WRITE:  target_func = "fwrite"; break;
        case BUILTIN_CLOSE:  target_func = "fclose"; break;
        case BUILTIN_ALLOC:  target_func = "malloc"; break;
        case BUILTIN_FREE:   target_func = "free"; break;
        case BUILTIN_EXIT:   target_func = "exit"; break;
        default:
            fprintf(out, "    ; Unknown builtin call mapping\n");
            return;
    }

    /* 5. Emit Call Instruction */
    if (target_func) {
#if defined(__x86_64__) || defined(_M_X64)
        fprintf(out, "    call %s\n", target_func);
#elif defined(__aarch64__) || defined(_M_ARM64)
        fprintf(out, "    bl %s\n", target_func);
#endif
    }
}

/* =========================================================================
 * Generate Assembly for Binary Operations
 * ========================================================================= */
void generate_binary_op_asm(FILE *out, ASTNode *node, SecurityContext *sec_ctx) {
    generate_code_from_ast(out, node->right, sec_ctx);
#if defined(__x86_64__) || defined(_M_X64)
    fprintf(out, "    push rax\n");
#elif defined(__aarch64__) || defined(_M_ARM64)
    fprintf(out, "    str x0, [sp, #-16]!\n");
#endif

    generate_code_from_ast(out, node->left, sec_ctx);

#if defined(__x86_64__) || defined(_M_X64)
    fprintf(out, "    pop rbx\n");
    switch (node->type) {
        case NODE_ADD: fprintf(out, "    add rax, rbx\n"); break;
        case NODE_SUB: fprintf(out, "    sub rax, rbx\n"); break;
        case NODE_MUL: fprintf(out, "    imul rax, rbx\n"); break;
        case NODE_DIV: 
            fprintf(out, "    cqo\n");
            fprintf(out, "    idiv rbx\n"); 
            break;
        case NODE_EQ:
            fprintf(out, "    cmp rax, rbx\n    sete al\n    movzx rax, al\n");
            break;
        case NODE_NEQ:
            fprintf(out, "    cmp rax, rbx\n    setne al\n    movzx rax, al\n");
            break;
        case NODE_LT:
            fprintf(out, "    cmp rax, rbx\n    setl al\n    movzx rax, al\n");
            break;
        case NODE_GT:
            fprintf(out, "    cmp rax, rbx\n    setg al\n    movzx rax, al\n");
            break;
        case NODE_LTE:
            fprintf(out, "    cmp rax, rbx\n    setle al\n    movzx rax, al\n");
            break;
        case NODE_GTE:
            fprintf(out, "    cmp rax, rbx\n    setge al\n    movzx rax, al\n");
            break;
        default: break;
    }

#elif defined(__aarch64__) || defined(_M_ARM64)
    fprintf(out, "    ldr x1, [sp], #16\n");
    switch (node->type) {
        case NODE_ADD: fprintf(out, "    add x0, x0, x1\n"); break;
        case NODE_SUB: fprintf(out, "    sub x0, x0, x1\n"); break;
        case NODE_MUL: fprintf(out, "    mul x0, x0, x1\n"); break;
        case NODE_DIV: fprintf(out, "    sdiv x0, x0, x1\n"); break;
        case NODE_EQ:
            fprintf(out, "    cmp x0, x1\n    cset x0, eq\n");
            break;
        case NODE_NEQ:
            fprintf(out, "    cmp x0, x1\n    cset x0, ne\n");
            break;
        case NODE_LT:
            fprintf(out, "    cmp x0, x1\n    cset x0, lt\n");
            break;
        case NODE_GT:
            fprintf(out, "    cmp x0, x1\n    cset x0, gt\n");
            break;
        case NODE_LTE:
            fprintf(out, "    cmp x0, x1\n    cset x0, le\n");
            break;
        case NODE_GTE:
            fprintf(out, "    cmp x0, x1\n    cset x0, ge\n");
            break;
        default: break;
    }
#endif
}

/* =========================================================================
 * Generate Assembly for Block Scopes
 * ========================================================================= */
void generate_block_asm(FILE *out, ASTNode *node, SecurityContext *sec_ctx) {
    if (!node || node->type != NODE_BLOCK) return;
    fprintf(out, "    ; --- Scope Block ENTER ---\n");
    for (int i = 0; i < node->stmt_count; i++) {
        generate_code_from_ast(out, node->statements[i], sec_ctx);
    }
    fprintf(out, "    ; --- Scope Block EXIT ---\n");
}

/* =========================================================================
 * Generate Assembly for @print
 * ========================================================================= */
void generate_print_asm(FILE *out, ASTNode *node, SecurityContext *sec_ctx) {
    if (!node || node->type != PRINT_NODE || !node->left) return;
    generate_code_from_ast(out, node->left, sec_ctx);
    fprintf(out, "    ; --- @print Statement ---\n");

#if defined(__x86_64__) || defined(_M_X64)
    fprintf(out, "    mov rdi, rax\n");
    if (node->left->type == NODE_STR) {
        fprintf(out, "    call mlov_print_str\n");
    } else {
        fprintf(out, "    call mlov_print_int\n");
    }
#elif defined(__aarch64__) || defined(_M_ARM64)
    if (node->left->type == NODE_STR) {
        fprintf(out, "    bl mlov_print_str\n");
    } else {
        fprintf(out, "    bl mlov_print_int\n");
    }
#endif
}

/* =========================================================================
 * Generate Assembly for Conditional If Statements
 * ========================================================================= */
void generate_if_asm(FILE *out, ASTNode *node, SecurityContext *sec_ctx) {
    if (!node || node->type != NODE_IF || !node->cond) return;

    int cur_id = label_counter++;
    fprintf(out, "    ; --- If Statement Start ---\n");
    
    generate_code_from_ast(out, node->cond, sec_ctx);

#if defined(__x86_64__) || defined(_M_X64)
    fprintf(out, "    cmp rax, 0\n");
    if (node->else_branch) {
        fprintf(out, "    je .L_else_%d\n", cur_id);
    } else {
        fprintf(out, "    je .L_end_if_%d\n", cur_id);
    }
#elif defined(__aarch64__) || defined(_M_ARM64)
    if (node->else_branch) {
        fprintf(out, "    cbz x0, .L_else_%d\n", cur_id);
    } else {
        fprintf(out, "    cbz x0, .L_end_if_%d\n", cur_id);
    }
#endif

    generate_code_from_ast(out, node->then_branch, sec_ctx);

    if (node->else_branch) {
#if defined(__x86_64__) || defined(_M_X64)
        fprintf(out, "    jmp .L_end_if_%d\n", cur_id);
        fprintf(out, ".L_else_%d:\n", cur_id);
#elif defined(__aarch64__) || defined(_M_ARM64)
        fprintf(out, "    b .L_end_if_%d\n", cur_id);
        fprintf(out, ".L_else_%d:\n", cur_id);
#endif
        generate_code_from_ast(out, node->else_branch, sec_ctx);
    }

    fprintf(out, ".L_end_if_%d:\n", cur_id);
    fprintf(out, "    ; --- If Statement End ---\n");
}

/* =========================================================================
 * Generate Assembly for While Loops
 * ========================================================================= */
void generate_while_asm(FILE *out, ASTNode *node, SecurityContext *sec_ctx) {
    if (!node || node->type != NODE_WHILE || !node->cond) return;

    int cur_id = label_counter++;
    fprintf(out, "    ; --- While Statement Start ---\n");
    fprintf(out, ".L_while_start_%d:\n", cur_id);

    generate_code_from_ast(out, node->cond, sec_ctx);

#if defined(__x86_64__) || defined(_M_X64)
    fprintf(out, "    cmp rax, 0\n");
    fprintf(out, "    je .L_while_end_%d\n", cur_id);
#elif defined(__aarch64__) || defined(_M_ARM64)
    fprintf(out, "    cbz x0, .L_while_end_%d\n", cur_id);
#endif

    generate_code_from_ast(out, node->then_branch, sec_ctx);

#if defined(__x86_64__) || defined(_M_X64)
    fprintf(out, "    jmp .L_while_start_%d\n", cur_id);
#elif defined(__aarch64__) || defined(_M_ARM64)
    fprintf(out, "    b .L_while_start_%d\n", cur_id);
#endif

    fprintf(out, ".L_while_end_%d:\n", cur_id);
    fprintf(out, "    ; --- While Statement End ---\n");
}

/* =========================================================================
 * Generate Assembly for For Loops
 * ========================================================================= */
void generate_for_asm(FILE *out, ASTNode *node, SecurityContext *sec_ctx) {
    if (!node || node->type != NODE_FOR) return;

    int cur_id = label_counter++;
    fprintf(out, "    ; --- For Loop Start ---\n");

    if (node->for_init) {
        generate_code_from_ast(out, node->for_init, sec_ctx);
    }

    fprintf(out, ".L_for_start_%d:\n", cur_id);

    if (node->for_cond) {
        generate_code_from_ast(out, node->for_cond, sec_ctx);
#if defined(__x86_64__) || defined(_M_X64)
        fprintf(out, "    cmp rax, 0\n");
        fprintf(out, "    je .L_for_end_%d\n", cur_id);
#elif defined(__aarch64__) || defined(_M_ARM64)
        fprintf(out, "    cbz x0, .L_for_end_%d\n", cur_id);
#endif
    }

    if (node->for_body) {
        generate_code_from_ast(out, node->for_body, sec_ctx);
    }

    if (node->for_post) {
        generate_code_from_ast(out, node->for_post, sec_ctx);
    }

#if defined(__x86_64__) || defined(_M_X64)
    fprintf(out, "    jmp .L_for_start_%d\n", cur_id);
#elif defined(__aarch64__) || defined(_M_ARM64)
    fprintf(out, "    b .L_for_start_%d\n", cur_id);
#endif

    fprintf(out, ".L_for_end_%d:\n", cur_id);
    fprintf(out, "    ; --- For Loop End ---\n");
}