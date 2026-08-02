//mcodegen.c
#include <stdio.h>
#include <stdint.h>
#include "mtypes.h"

/* =========================================================================
 * Cross-Platform Hardware Signature Generator
 * Supports: x86_64, x86, ARM64, ARMv7, and Nintendo 3DS (ARMv6K / ARM11)
 * ========================================================================= */
uint64_t get_hardware_signature() {
    uint64_t signature = 0;

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
    /* ---------------------------------------------------------------------
     * 1. x86 / x86_64 Architecture (PC / Laptop)
     * --------------------------------------------------------------------- */
    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
    __asm__ __volatile__ (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );
    signature = ((uint64_t)eax << 32) | (ebx ^ ecx ^ edx);

#elif defined(__3DS__) || defined(_3DS)
    /* ---------------------------------------------------------------------
     * 2. Nintendo 3DS Architecture (ARM11 / ARMv6K)
     * Uses ARM Coprocessor 15 (c13) Process ID / Hardware Register
     * --------------------------------------------------------------------- */
    uint32_t cpuid_reg = 0;
    uint32_t proc_id = 0;
    
    /* read Main ID Register from ARM CP15 (c0, c0, 0) */
    __asm__ __volatile__ ("mrc p15, 0, %0, c0, c0, 0" : "=r"(cpuid_reg));
    /* read Process ID Register from ARM CP15 (c13, c0, 1) */
    __asm__ __volatile__ ("mrc p15, 0, %0, c13, c0, 1" : "=r"(proc_id));

    signature = ((uint64_t)cpuid_reg << 32) | (proc_id ^ 0x3D53D53D);

#elif defined(__aarch64__) || defined(_M_ARM64)
    /* ---------------------------------------------------------------------
     * 3. ARM64 Architecture (Apple Silicon M1/M2/M3, Android, Raspberry Pi 4/5)
     * --------------------------------------------------------------------- */
    uint64_t midr = 0;
    /* read Main ID Register (MIDR_EL1) from ARM64 */
    __asm__ __volatile__ ("mrs %0, midr_el1" : "=r"(midr));
    signature = midr ^ 0xA64A64A64A64A64ULL;

#elif defined(__arm__) || defined(_M_ARM)
    /* ---------------------------------------------------------------------
     * 4. Generic ARMv7 / 32-bit ARM (Raspberry Pi older models, Embedded)
     * --------------------------------------------------------------------- */
    uint32_t midr = 0;
    __asm__ __volatile__ ("mrc p15, 0, %0, c0, c0, 0" : "=r"(midr));
    signature = ((uint64_t)midr << 32) | (midr ^ 0x41524D37);

#else
    /* ---------------------------------------------------------------------
     * 5. Generic / Unknown CPU Fallback
     * --------------------------------------------------------------------- */
    signature = 0x4D41524C4F563231ULL; /* "MARLOV21" ASCII Hash fallback */
#endif

    return signature;
}

/* =========================================================================
 * Architecture-Aware Assembly Code Generator
 * Emits specific target assembly for x86_64 vs ARM/3DS
 * ========================================================================= */
void generate_security_header(FILE *out, SecurityContext *ctx) {
    fprintf(out, "; --- MARLOV HARDWARE-BOUND SECURITY GATEKEEPER ---\n");

#if defined(__x86_64__) || defined(_M_X64)
    /* --- Target: x86-64 --- */
    fprintf(out, "global _start\n");
    fprintf(out, "_start:\n");
    
    if (ctx->permissions & PERM_DISK_READ) {
        fprintf(out, "    mov rax, 1\n");
        fprintf(out, "    cpuid\n");
        fprintf(out, "    xor rbx, rcx\n");
        fprintf(out, "    cmp rbx, 0x%llX ; Verification Key\n", (unsigned long long)ctx->hardware_hash);
        fprintf(out, "    jne self_destruct\n");
    } else {
        fprintf(out, "    ; Security Violation: Missing @sys.disk.read\n");
        fprintf(out, "    jmp self_destruct\n");
    }
    
    fprintf(out, "\nself_destruct:\n");
    fprintf(out, "    ud2 ; x86 Undefined Instruction\n");

#elif defined(__3DS__) || defined(_3DS) || defined(__arm__) || defined(_M_ARM) || defined(__aarch64__)
    /* --- Target: ARM / Nintendo 3DS --- */
    fprintf(out, ".global _start\n");
    fprintf(out, "_start:\n");
    
    if (ctx->permissions & PERM_DISK_READ) {
        fprintf(out, "    mrc p15, 0, r0, c0, c0, 0 ; Read ARM CP15 ID\n");
        fprintf(out, "    ldr r1, =0x%X ; Verification Hash\n", (uint32_t)(ctx->hardware_hash & 0xFFFFFFFF));
        fprintf(out, "    cmp r0, r1\n");
        fprintf(out, "    bne self_destruct\n");
    } else {
        fprintf(out, "    @ Security Violation: Missing @sys.disk.read\n");
        fprintf(out, "    b self_destruct\n");
    }
    
    fprintf(out, "\nself_destruct:\n");
    fprintf(out, "    .word 0xe7f000f0 ; ARM Permanent Undefined Instruction (Trap/Crash)\n");

#else
    /* --- Target: Generic C / Fallback --- */
    fprintf(out, "; Generic Target Execution Context\n");
#endif
}