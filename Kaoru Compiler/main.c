//main.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "mtypes.h"

/* Include Source Modules */
#include "mcodegen.c"  
#include "mlexer.c"    
#include "mparser.c"   
#include "marloru.c"  
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
    printf("  -o <output>    Specify output executable name (default: a.out / out.exe)\n");
    printf("  --dump-ast     Print AST structure for debugging\n");
    printf("  -v, --version  Display compiler version information\n");
    printf("  -h, --help     Display this help message\n");
}
static void run_interactive_cli() {
	char input[512];
	printf("==================================================\n");
	printf("   KAORU COMPILER INTERACTIVE CLI (v%s)\n", KAORU_VERSION);
	printf("   Type 'exit' or 'quit' to exit.\n");
	printf("   Type 'tui' to open Marloru Editor.\n");
	printf("   Type 'help' for available commands.\n");
	printf("==================================================\n\n");
	
	while (1) {
		printf("kaoru> ");
		fflush(stdout);
		
		if (!fgets(input, sizeof(input), stdin)) break;
		
		// ลบ newline สิ้นสุดข้อความ
		input[strcspn(input, "\r\n")] = 0;
		
		if (strlen(input) == 0) continue;
		
		if (strcmp(input, "exit") == 0 || strcmp(input, "quit") == 0) {
			printf("[Kaoru]: Exiting Interactive CLI...\n");
			break;
		} else if (strcmp(input, "help") == 0) {
			print_usage("kaoru");
		} else if (strcmp(input, "tui") == 0) {
			run_marloru_editor();
		} else {
			printf("[Kaoru CLI]: Processing command or file '%s'...\n", input);
			FILE *f = fopen(input, "r");
			if (f) {
				SecurityContext sec_ctx;
				sec_ctx.permissions = PERM_NONE;
				sec_ctx.hardware_hash = get_hardware_signature();
				
				parse_program(f, &sec_ctx);
				ASTNode *root = parse(f);
				if (root) free_ast(root);
				fclose(f);
				printf("[Kaoru CLI]: Parsing finished successfully.\n");
			} else {
				printf("[Kaoru CLI Error]: Unknown command or file '%s' not found.\n", input);
			}
		}
		printf("\n");
	}
}
int main(int argc, char *argv[]) {
    
    if (argc < 2) {
        run_interactive_cli()();
        return 0;
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
                printf("[Kaoru CLI Error]: I can not found you file.\n Missing filename after -o flag.\n");
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
        printf("[Kaoru CLI Error]: Go get the file for me, babe.\n No input source file provided.\n");
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
