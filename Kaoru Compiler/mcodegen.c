// mcodegen.c
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "mtypes.h"

/* =========================================================================
 * Forward Declarations & Helpers
 * ========================================================================= */
void generate_code_from_ast(FILE *out, ASTNode *node);
void generate_print_asm(FILE *out, ASTNode *node);
void generate_if_asm(FILE *out, ASTNode *node);
void generate_block_asm(FILE *out, ASTNode *node);
void generate_binary_op_asm(FILE *out, ASTNode *node);

static int label_counter = 0;

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
 * Cross-Platform Hardware Signature Generator
 * ========================================================================= */
uint64_t get_hardware_signature() {
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
 * Security Gatekeeper Assembly Output
 * ========================================================================= */
void generate_runtime_header(FILE *out, SecurityContext *sec_ctx) {
    fprintf(out, "// --- KAORU RUNTIME SECURITY GUARD ---\n");
    fprintf(out, "#include <stdio.h>\n");
    fprintf(out, "#include <stdlib.h>\n");
    fprintf(out, "#include <stdbool.h>\n\n");
    fprintf(out, "static const unsigned long long AUTHORIZED_HARDWARE_HASH = 0x%llXULL;\n", 
            (unsigned long long)sec_ctx->hardware_hash);
    fprintf(out, "static const bool HAS_DISK_READ_PERM = %s;\n\n", 
            (sec_ctx->permissions & PERM_DISK_READ) ? "true" : "false");
    fprintf(out, "void verify_hardware_and_permissions() {\n");
    fprintf(out, "    unsigned long long current_cpuid = get_hardware_signature();\n");
    fprintf(out, "    if (current_cpuid != AUTHORIZED_HARDWARE_HASH) {\n");
    fprintf(out, "        printf(\"[Marlov Runtime Error]: Hardware signature mismatch! Unauthorized device.\\n\");\n");
    fprintf(out, "        exit(137);\n");
    fprintf(out, "    }\n");
    fprintf(out, "}\n\n");
}

void generate_assembly_entry(FILE *out, SecurityContext *ctx) {
#if defined(__x86_64__) || defined(_M_X64)
    fprintf(out, ".global main\n");
    fprintf(out, "main:\n");
    fprintf(out, "    push rbp\n");
    fprintf(out, "    mov rbp, rsp\n");
    if (ctx && !(ctx->permissions & PERM_DISK_READ)) {
        fprintf(out, "    ; Security Violation Trap\n");
        fprintf(out, "    ud2\n");
    }
#elif defined(__aarch64__) || defined(_M_ARM64)
    fprintf(out, ".global main\n");
    fprintf(out, "main:\n");
    fprintf(out, "    stp x29, x30, [sp, #-16]!\n");
    fprintf(out, "    mov x29, sp\n");
    if (ctx && !(ctx->permissions & PERM_DISK_READ)) {
        fprintf(out, "    .word 0xd4200000 ; BRK trap\n");
    }
#endif
}

/* =========================================================================
 * AST Code Generator Dispatcher
 * ========================================================================= */
void generate_code_from_ast(FILE *out, ASTNode *node) {
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
            generate_binary_op_asm(out, node);
            break;

        case NODE_VAR_DECL:
            generate_code_from_ast(out, node->left);
            fprintf(out, "    ; Variable Decl: %s initialized\n", node->var_name);
            break;

        case NODE_BLOCK:
            generate_block_asm(out, node);
            break;

        case NODE_IF:
            generate_if_asm(out, node);
            break;

        case PRINT_NODE:
            generate_print_asm(out, node);
            break;

        default:
            fprintf(out, "    ; [CodeGen]: Unhandled AST Node type (%d)\n", node->type);
            break;
    }
}

/* =========================================================================
 * Generate Assembly for Binary Operations (+, -, *, /, Relational)
 * ========================================================================= */
void generate_binary_op_asm(FILE *out, ASTNode *node) {
    // Generate Right AST (ผลลัพธ์ไปฝั่ง Stack หรือ Register สำรอง)
    generate_code_from_ast(out, node->right);
#if defined(__x86_64__) || defined(_M_X64)
    fprintf(out, "    push rax\n");
#elif defined(__aarch64__) || defined(_M_ARM64)
    fprintf(out, "    str x0, [sp, #-16]!\n");
#endif

    // Generate Left AST
    generate_code_from_ast(out, node->left);

#if defined(__x86_64__) || defined(_M_X64)
    fprintf(out, "    pop rbx\n"); // rbx = Right, rax = Left
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
    fprintf(out, "    ldr x1, [sp], #16\n"); // x0 = Left, x1 = Right
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
void generate_block_asm(FILE *out, ASTNode *node) {
    if (!node || node->type != NODE_BLOCK) return;
    fprintf(out, "    ; --- Scope Block ENTER ---\n");
    for (int i = 0; i < node->stmt_count; i++) {
        generate_code_from_ast(out, node->statements[i]);
    }
    fprintf(out, "    ; --- Scope Block EXIT ---\n");
}

/* =========================================================================
 * Generate Assembly for @print
 * ========================================================================= */
void generate_print_asm(FILE *out, ASTNode *node) {
    if (!node || node->type != PRINT_NODE || !node->left) return;
    generate_code_from_ast(out, node->left);
    fprintf(out, "    ; --- @print Statement ---\n");

#if defined(__x86_64__) || defined(_M_X64)
    fprintf(out, "    mov rdi, rax\n");
    if (node->left->type == NODE_STR) {
        fprintf(out, "    call mlov_print_str\n");
    } else {
        fprintf(out, "    call mlov_print_int\n");
    }
#elif defined(__aarch64__) || defined(_M_ARM64)
    fprintf(out, "    mov x0, x0\n");
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
void generate_if_asm(FILE *out, ASTNode *node) {
    if (!node || node->type != NODE_IF || !node->cond) return;

    int cur_id = label_counter++;
    fprintf(out, "    ; --- If Statement Start ---\n");
    
    // Evaluate condition -> Result is in rax / x0 (1 = true, 0 = false)
    generate_code_from_ast(out, node->cond);

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

    // Then Branch
    generate_code_from_ast(out, node->then_branch);

    if (node->else_branch) {
#if defined(__x86_64__) || defined(_M_X64)
        fprintf(out, "    jmp .L_end_if_%d\n", cur_id);
        fprintf(out, ".L_else_%d:\n", cur_id);
#elif defined(__aarch64__) || defined(_M_ARM64)
        fprintf(out, "    b .L_end_if_%d\n", cur_id);
        fprintf(out, ".L_else_%d:\n", cur_id);
#endif
        generate_code_from_ast(out, node->else_branch);
    }

    fprintf(out, ".L_end_if_%d:\n", cur_id);
    fprintf(out, "    ; --- If Statement End ---\n");
}