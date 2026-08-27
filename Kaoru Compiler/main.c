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

#define KAORU_VERSION "1.0.0"

static void print_usage(const char *prog_name) {
    printf("KAORU COMPILER - MARLOV LANGUAGE (v%s)\n", KAORU_VERSION);
    printf("Usage: %s <source.ml> [-include <header.mlov>] [-o <output>]\n", prog_name);
}

/* Helper function to free Symbol Table memory */
static void cleanup_symbol_table(SymbolTable *symtab) {
    if (!symtab) return;
    SymbolNode *curr = symtab->head;
    while (curr) {
        SymbolNode *next = curr->next;
        free(curr); // Clean up symbol node allocations
        curr = next;
    }
    symtab->head = NULL;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char *source_file = NULL;
    const char *header_file = NULL;
    const char *output_file = "a.out";
    bool dump_ast = false;

    /* CLI Arguments Parser */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-o") == 0 && i + 1 < argc) {
            output_file = argv[++i];
        } else if (strcmp(argv[i], "-include") == 0 && i + 1 < argc) {
            header_file = argv[++i];
        } else if (strcmp(argv[i], "--dump-ast") == 0) {
            dump_ast = true;
        } else if (argv[i][0] != '-') {
            source_file = argv[i];
        }
    }

    if (!source_file) {
        printf("[Kaoru CLI Error]: No input .ml source file provided.\n");
        print_usage(argv[0]);
        return 1;
    }

    /* Initialize Global Symbol Table */
    SymbolTable symtab = { .head = NULL };
    SecurityContext sec_ctx = { .permissions = 0, .hardware_hash = 0 };

    /* 1. Parse .mlov Header File (If provided via CLI or directive) */
    if (header_file) {
        printf("[Kaoru Compiler]: Loading Header '%s'...\n", header_file);
        if (!parse_mlov_header(header_file, &symtab, &sec_ctx)) {
            cleanup_symbol_table(&symtab);
            return 1;
        }
    }

    /* 2. Parse .ml Main Source File */
    FILE *source = fopen(source_file, "r");
    if (!source) {
        printf("[Kaoru Fatal Error]: Cannot open source file '%s'\n", source_file);
        cleanup_symbol_table(&symtab);
        return 1;
    }

    parse_program(source, &sec_ctx);
    ASTNode *root = parse_with_symbols(source, &symtab);
    fclose(source);

    /* 3. Code Generation */
    FILE *file_out = fopen(output_file, "w");
    if (!file_out) {
        printf("[Kaoru Fatal Error]: Cannot open output file '%s'\n", output_file);
        if (root) free_ast(root);
        cleanup_symbol_table(&symtab);
        return 1;
    }

    generate_runtime_header(file_out, &sec_ctx);
    generate_assembly_entry(file_out, &sec_ctx);

    if (root) {
        generate_code_from_ast(file_out, root, &sec_ctx);
        free_ast(root);
    }

    fclose(file_out);
    printf("[Kaoru Compiler]: Successfully compiled '%s' using symbols from '%s' -> '%s'\n", 
           source_file, header_file ? header_file : "N/A", output_file);

    /* Free symbol table before normal exit */
    cleanup_symbol_table(&symtab);

    return 0;
}