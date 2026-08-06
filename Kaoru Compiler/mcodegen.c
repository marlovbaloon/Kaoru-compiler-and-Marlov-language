//mcodegen.c
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "mtypes.h"

/* =========================================================================
 * Forward Declarations & Prototypes
 * ========================================================================= */
void generate_code_from_ast(FILE *out, ASTNode *node);
void generate_print_asm(FILE *out, ASTNode *node);

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
 * Supports: x86_64, x86, ARM64, ARMv7, and Nintendo 3DS (ARMv6K / ARM11)
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
    fprintf(out, "static const unsigned long long AUTHORIZED_HARDWARE_HASH = 0x%LXULL;\n", 
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
    
    fprintf(out, "\nself_destruct:\n");
    fprintf(out, "    ud2 ; x86 Undefined Instruction\n");

#elif defined(__3DS__) || defined(_3DS) || defined(__arm__) || defined(_M_ARM) || defined(__aarch64__)
    fprintf(out, ".global _start\n");
    fprintf(out, "_start:\n");
    
    if (ctx && (ctx->permissions & PERM_DISK_READ)) {
        fprintf(out, "    mrc p15, 0, r0, c0, c0, 0 ; Read ARM CP15 ID\n");
        fprintf(out, "    ldr r1, =0x%X ; Verification Hash\n", (uint32_t)(ctx->hardware_hash & 0xFFFFFFFF));
        fprintf(out, "    cmp r0, r1\n");
        fprintf(out, "    bne self_destruct\n");
    } else {
        fprintf(out, "    @ Security Violation: Missing @sys.disk.read\n");
        fprintf(out, "    b self_destruct\n");
    }
    
    fprintf(out, "\nself_destruct:\n");
    fprintf(out, "    .word 0xe7f000f0 ; ARM Permanent Undefined Instruction Trap\n");

#else
    fprintf(out, "; Generic Target Execution Context\n");
#endif
}

/* =========================================================================
 * AST Code Generator Dispatcher (Main Generator Entry)
 * ========================================================================= */
void generate_code_from_ast(FILE *out, ASTNode *node) {
    if (!node) return;

    switch (node->type) {
        case PRINT_NODE:
            generate_print_asm(out, node);
            break;

        /* add another node in future */
        default:
            fprintf(out, "    ; [CodeGen]: Unhandled AST Node type (%d)\n", node->type);
            break;
    }
}

/* =========================================================================
 * Generate Assembly for @print
 * ========================================================================= */
void generate_print_asm(FILE *out, ASTNode *node) {
    if (!node || node->type != PRINT_NODE || !node->left) return;
    generate_code_from_ast(out, node->left);
    fprintf(out, "    ; --- @print Statement ---\n");
    if (node->left->type == NODE_STR) {
#if defined(__x86_64__) || defined(_M_X64)
        fprintf(out, "    mov rdi, rax\n");
        fprintf(out, "    call mlov_print_str\n");
#elif defined(__aarch64__) || defined(_M_ARM64)
        fprintf(out, "    mov x0, x0\n");
        fprintf(out, "    bl mlov_print_str\n");
#endif
    } 
    else {
#if defined(__x86_64__) || defined(_M_X64)
        fprintf(out, "    mov rdi, rax\n");
        fprintf(out, "    call mlov_print_int\n");
#elif defined(__aarch64__) || defined(_M_ARM64)
        fprintf(out, "    mov x0, x0\n");
        fprintf(out, "    bl mlov_print_int\n");
#endif
    }
}
