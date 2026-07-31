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
            while (isalpha(c = fgetc(file)) || c == '.') {
                buffer[idx++] = c;
            } else{
                printf("Kaoru Error: TL;DR No Cap Fr Fr\n");
                break;
            }
            ungetc(c, file);
            buffer[idx] = '\0';

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