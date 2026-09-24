//mstack.c 
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "mtypes.h"

typedef struct {
    Symbol symbols[64];
    int symbol_count;
    int scope_depth;
} StackFrame;

static StackFrame current_frame;

void scope_enter() {
    current_frame.scope_depth++;
}

/* Immediately release Stack RAM when out of curly braces { }; */
void scope_exit(uint32_t *current_stack_offset) {
    /* Assembly executes the add rsp, N instruction to immediately release RAM without relying on the Garbage Collector */
    current_frame.scope_depth--;
}

void add_symbol(const char *name, int32_t offset) {
    strcpy(current_frame.symbols[current_frame.symbol_count].name, name);
    current_frame.symbols[current_frame.symbol_count].stack_offset = offset;
    current_frame.symbol_count++;
}