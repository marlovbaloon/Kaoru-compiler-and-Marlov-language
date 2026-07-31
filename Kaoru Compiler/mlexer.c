//mlexer.c
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "mtypes.h"

Token next_token(FILE *file) {
    int c;
    Token tok;
    tok.value[0] = '\0';

    while ((c = fgetc(file)) != EOF) {
        if (isspace(c)) continue;

        /* read behavior form symbol @ */
        if (c == '@') {
            char buffer[32] = "@";
            int idx = 1;
            while ((c = fgetc(file)) != EOF) {
                if (isalpha(c) || c == '.') {
                    if (idx < 31) { 
                        buffer[idx++] = c;
                    }
                } else {
                    ungetc(c, file);
                    break;
                }
            }           
            buffer[idx] = '\0';
            if (idx == 1) {
                printf("Kaoru Error: TL;DR No Cap Fr Fr\n"); 
            }
            strcpy(tok.value, buffer);
            if (strcmp(buffer, "@sys.disk.read") == 0) tok.type = TOKEN_AT_SYS;
            else if (strcmp(buffer, "@func") == 0) tok.type = TOKEN_AT_FUNC;
            else if (strcmp(buffer, "@int") == 0) tok.type = TOKEN_AT_INT;
            else if (strcmp(buffer, "@print") == 0) tok.type = TOKEN_AT_PRINT;
            else if (strcmp(buffer, "@debug") == 0) tok.type = TOKEN_AT_DEBUG;
            return tok;
        }

        if (c == '{') { tok.type = TOKEN_LBRACE; return tok; }
        if (c == '}') { tok.type = TOKEN_RBRACE; return tok; }
        if (c == ';') { tok.type = TOKEN_SEMICOLON; return tok; }
        
        /* variable scan */
        if (isalnum(c)) {
            int idx = 0;
            tok.value[idx++] = c;
            while (isalnum(c = fgetc(file))) tok.value[idx++] = c;
            ungetc(c, file);
            tok.value[idx] = '\0';
            tok.type = TOKEN_IDENTIFIER;
            return tok;
        }
    }
    tok.type = TOKEN_EOF;
    return tok;
}