//mcodegen.c
#include <stdio.h>
#include "mtypes.h"

/* Function to retrieve bit-level Hardware ID using Inline Assembly (X86-64) */
uint64_t get_hardware_signature() {
    uint32_t eax, ebx, ecx, edx;
    
    /* Retrieving CPU Hardware Serial/Info via CPUID command */
    __asm__ __volatile__ (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(1)
    );

    /* Combine bit values using XOR & Shift to create a machine-specific signature */
    uint64_t signature = ((uint64_t)eax << 32) | (ebx ^ ecx ^ edx);
    return signature;
}

/* permissions gatekeep Hardware identity*/
void generate_security_header(FILE *out, SecurityContext *ctx) {
    fprintf(out, "; --- MARLOV HARDWARE-BOUND SECURITY GATEKEEPER ---\n");
    fprintf(out, "global _start\n");
    fprintf(out, "_start:\n");
    
    if (ctx->permissions & PERM_DISK_READ) {
        /* check Hardware Signature on Register */
        fprintf(out, "    mov rax, 1\n");
        fprintf(out, "    cpuid\n");
        fprintf(out, "    xor rbx, rcx\n");
        fprintf(out, "    cmp rbx, 0x%lX ; Verification Key\n", ctx->hardware_hash);
        fprintf(out, "    jne self_destruct; (domain Collapse)\n");
    } else {
        fprintf(out, "    ; Security Violation: File did not request @sys.disk.read\n");
        fprintf(out, "jmp self_destruct\n");
    }
    
    fprintf(out, "\nself_destruct:\n");
    fprintf(out, "ud2\n");
}