#include <stdio.h>
#include <stdlib.h>
#include "mtypes.h"

extern uint64_t get_hardware_signature();
extern void parse_program(FILE *in, SecurityContext *sec_ctx);

int main(int argc, char *argv[]) {
    printf("===========================================\n");
    printf("   KAORU COMPILER - MARLOV LANGUAGE\n");
    printf("   Architecture: Native C + Assembly\n");
    printf("===========================================\n");

    if (argc < 2) {
        printf("Usage: kaoru <source_file.ml>\n");
        return 1;
    }

    FILE *source = fopen(argv[1], "r");
    if (!source) {
        printf("Error: Cannot open source file %s\n", argv[1]);
        return 1;
    }

    /* make Security Context im bits*/
    SecurityContext sec_ctx;
    sec_ctx.permissions = PERM_NONE;
    sec_ctx.hardware_hash = get_hardware_signature();

    /* analysis file */
    parse_program(source, &sec_ctx);

    fclose(source);
    printf("[Kaoru Compiler]: Compilation Successful! Executable Generated.\n");
    return 0;
}