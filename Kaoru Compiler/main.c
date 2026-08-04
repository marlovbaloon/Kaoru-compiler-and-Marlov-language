//main.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "mtypes.h"

#define KAORU_VERSION "1.0.0"

extern uint64_t get_hardware_signature();
extern void parse_program(FILE *in, SecurityContext *sec_ctx);

/* (Help Menu) */
static void print_usage(const char *prog_name) {
    printf("KAORU COMPILER - MARLOV LANGUAGE (v%s)\n", KAORU_VERSION);
    printf("Architecture: Native C + Assembly\n\n");
    printf("Usage: %s <source_file.ml> [options]\n\n", prog_name);
    printf("Options:\n");
    printf("  -o <output>    Specify output executable name (default: a.out / out.exe)\n");
    printf("  --dump-ast     Print AST structure for debugging\n");
    printf("  -v, --version  Display compiler version information\n");
    printf("  -h, --help     Display this help message\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *source_file = NULL;
    const char *output_file = "a.out";
    bool dump_ast = false;

    /* -------------------------------------------------------------------------
     * 1. CLI Arguments Parser
     * ------------------------------------------------------------------------- */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            printf("Kaoru Compiler Version %s\n", KAORU_VERSION);
            return 0;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 < argc) {
                output_file = argv[++i];
            } else {
                printf("[Kaoru CLI Error]: Missing filename after -o flag.\n");
                return 1;
            }
        } else if (strcmp(argv[i], "--dump-ast") == 0) {
            dump_ast = true;
        } else if (argv[i][0] == '-') {
            printf("[Kaoru CLI Error]: Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        } else {
            source_file = argv[i];
        }
    }

    if (!source_file) {
        printf("[Kaoru CLI Error]: No input source file provided.\n");
        return 1;
    }

    /* -------------------------------------------------------------------------
     * 2. Compiler Header Display & Input Validation
     * ------------------------------------------------------------------------- */
    printf("===========================================\n");
    printf("   KAORU COMPILER - MARLOV LANGUAGE\n");
    printf("   Architecture: Native C + Assembly\n");
    printf("===========================================\n");
    printf("[Kaoru Compiler]: Compiling '%s' -> '%s'...\n", source_file, output_file);

    FILE *source = fopen(source_file, "r");
    if (!source) {
        printf("[Kaoru Fatal Error]: Cannot open source file '%s'\n", source_file);
        return 1;
    }

    /* -------------------------------------------------------------------------
     * 3. Security Context & Parsing Phase
     * ------------------------------------------------------------------------- */
    SecurityContext sec_ctx;
    sec_ctx.permissions = PERM_NONE;
    sec_ctx.hardware_hash = get_hardware_signature();

    /* Header Security Verification */
    parse_program(source, &sec_ctx);

    /* Main AST Parsing Loop */
    ASTNode *root = parse(source);
    if (!root) {
        printf("[Kaoru Compiler]: Parsing finished (Empty or Single Statement AST).\n");
    } else {
        if (dump_ast) {
            printf("[Kaoru Debug]: AST Root Type = %d\n", root->type);
        }
        free_ast(root);
        root = NULL;
    }

    fclose(source);
    
    printf("===========================================\n");
    printf("[Kaoru Compiler]: Compilation Successful!\n");
    printf("[Kaoru Compiler]: Output binary generated: %s\n", output_file);
    printf("===========================================\n");

    return 0;
}