// main.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "mtypes.h"

/* Include Source Modules */
#include "mcodegen.c"  
#include "mlexer.c"    
#include "mparser.c"   
#include "marloru.c"  /* Included Marloru TUI Editor */

#define KAORU_VERSION "1.0.0"

extern void parse_program(FILE *in, SecurityContext *sec_ctx);
ASTNode *parse(FILE *in);
void free_ast(ASTNode *node);

/* (Help Menu) */
static void print_usage(const char *prog_name) {
    printf("KAORU COMPILER - MARLOV LANGUAGE (v%s)\n", KAORU_VERSION);
    printf("Architecture: Native C + Assembly\n\n");
    printf("Usage: %s <source_file.ml> [options]\n\n", prog_name);
    printf("Options:\n");
    printf("  -o <output>    Specify output executable/assembly name (default: a.out)\n");
    printf("  --dump-ast     Print AST structure for debugging\n");
    printf("  -v, --version  Display compiler version information\n");
    printf("  -h, --help     Display this help message\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        run_interactive_cli();
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
    fclose(source);

    if (!root) {
        printf("[Kaoru Compiler]: Parsing finished (Empty AST).\n");
        return 0;
    }

    if (dump_ast) {
        printf("[Kaoru Debug]: AST Root Type = %d\n", root->type);
    }

    /* -------------------------------------------------------------------------
     * 4. Code Generation Phase
     * ------------------------------------------------------------------------- */
    FILE *file_out = fopen(output_file, "w");
    if (!file_out) {
        printf("[Kaoru Fatal Error]: Cannot open output file '%s'\n", output_file);
        free_ast(root);
        return 1;
    }

    /* Generate Runtime Guard Headers & Assembly Entry */
    generate_runtime_header(file_out, &sec_ctx);
    generate_assembly_entry(file_out, &sec_ctx);

    /* Direct AST to Assembly/Code Generation */
    generate_code_from_ast(file_out, root, &sec_ctx);

    fclose(file_out);
    free_ast(root);

    printf("===========================================\n");
    printf("[Kaoru Compiler]: Compilation Successful!\n");
    printf("[Kaoru Compiler]: Output binary generated: %s\n", output_file);
    printf("===========================================\n");

    return 0;
}