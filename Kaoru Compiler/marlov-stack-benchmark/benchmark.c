#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

extern void marlov_test_scope(void);

void cpp_test_scope(void) {
    volatile uint64_t a = 1;
    {
        volatile uint64_t b = 2;
        if (a == b) {
            volatile uint64_t c = 3;
            (void)c; // Prevent unused variable warning
            return; 
        }
    }
}

void verify_stack_invariant(void (*func)(void), const char* lang_name) {
    uint64_t sp_before = 0;
    uint64_t sp_after = 0;

    __asm__ __volatile__("mov %%rsp, %0" : "=r"(sp_before));
    
    func();
    
    __asm__ __volatile__("mov %%rsp, %0" : "=r"(sp_after));

    int64_t drift = (int64_t)sp_before - (int64_t)sp_after;
    bool is_aligned = (sp_after % 8) == 0;

    printf("=== %s Benchmark ===\n", lang_name);
    printf("SP Initial : 0x%lx\n", sp_before);
    printf("SP Exit    : 0x%lx\n", sp_after);
    printf("Drift      : %ld bytes\n", drift);
    printf("Aligned 8  : %s\n\n", is_aligned ? "PASS" : "FAIL");
}

int main(void) {
    verify_stack_invariant(cpp_test_scope, "C/C++ (GCC/LLVM)");
    verify_stack_invariant(marlov_test_scope, "Marlov (Kaoru Compiler)");
    
    return 0;
}