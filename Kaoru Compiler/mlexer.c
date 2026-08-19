//mlexer.c
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "mtypes.h"

static uint32_t current_line = 1;

Token next_token(FILE *file) {
    int c;
    Token tok;
    tok.value[0] = '\0';

    while ((c = fgetc(file)) != EOF) {
        if (c == '\n') {
            current_line++;
            continue;
        }
        if (isspace(c)) continue;

        tok.line = current_line;

        /* 1. Handle Symbol Prefix @ */
        if (c == '@') {
            char buffer[32] = "@";
            int idx = 1;
            while ((c = fgetc(file)) != EOF) {
                if (isalpha(c) || c == '.') {
                    if (idx < 31) buffer[idx++] = (char)c;
                } else {
                    ungetc(c, file);
                    break;
                }
            }          
            buffer[idx] = '\0';
            
            snprintf(tok.value, sizeof(tok.value), "%s", buffer);
            if (strcmp(buffer, "@sys.disk.read") == 0) tok.type = TOKEN_AT_SYS;
            else if (strcmp(buffer, "@import") == 0) tok.type = TOKEN_AT_IMPORT;
            else if (strcmp(buffer, "@func") == 0) tok.type = TOKEN_AT_FUNC;
            else if (strcmp(buffer, "@int") == 0) tok.type = TOKEN_AT_INT;
            else if (strcmp(buffer, "@str") == 0) tok.type = TOKEN_AT_STR;
            else if (strcmp(buffer, "@bool") == 0) tok.type = TOKEN_AT_BOOL;
            else if (strcmp(buffer, "@tbl") == 0) tok.type = TOKEN_AT_TBL;
            else if (strcmp(buffer, "@print") == 0) tok.type = TOKEN_AT_PRINT;
            else if (strcmp(buffer, "@debug") == 0) tok.type = TOKEN_AT_DEBUG;
            else {
                printf("Kaoru Syntax Error (Line %u): Unknown @ directive %s\n", current_line, buffer);
                tok.type = TOKEN_EOF;
            }
            return tok;
        }

        /* 2. String Literals */
        if (c == '"') {
            int idx = 0;
            while ((c = fgetc(file)) != EOF && c != '"') {
                if (c == '\n') current_line++;
                if (idx < (int)sizeof(tok.value) - 1) tok.value[idx++] = (char)c;
            }
            tok.value[idx] = '\0';
            tok.type = TOKEN_STRING_LIT;
            return tok;
        }

        /* 3. Numbers */
        if (isdigit(c)) {
            int idx = 0;
            tok.value[idx++] = (char)c;
            while ((c = fgetc(file)) != EOF && isdigit(c)) {
                if (idx < (int)sizeof(tok.value) - 1) tok.value[idx++] = (char)c;
            }
            ungetc(c, file);
            tok.value[idx] = '\0';
            tok.type = TOKEN_NUMBER;
            return tok;
        }

        /* 4. Identifiers & General Keywords */
        if (isalpha(c) || c == '_') {
            int idx = 0;
            tok.value[idx++] = (char)c;
            while ((c = fgetc(file)) != EOF && (isalnum(c) || c == '_')) {
                if (idx < (int)sizeof(tok.value) - 1) tok.value[idx++] = (char)c;
            }
            ungetc(c, file);
            tok.value[idx] = '\0';

            if (strcmp(tok.value, "if") == 0) tok.type = TOKEN_IF;
            else if (strcmp(tok.value, "else") == 0) tok.type = TOKEN_ELSE;
            else if (strcmp(tok.value, "while") == 0) tok.type = TOKEN_WHILE;
            else if (strcmp(tok.value, "return") == 0) tok.type = TOKEN_RETURN;
            else tok.type = TOKEN_IDENTIFIER;

            return tok;
        }

        /* 5. Delimiters & Single/Double Operators */
        if (c == '{') { tok.type = TOKEN_LBRACE; strcpy(tok.value, "{"); return tok; }
        if (c == '}') { tok.type = TOKEN_RBRACE; strcpy(tok.value, "}"); return tok; }
        if (c == '(') { tok.type = TOKEN_LPAREN; strcpy(tok.value, "("); return tok; }
        if (c == ')') { tok.type = TOKEN_RPAREN; strcpy(tok.value, ")"); return tok; }
        if (c == ';') { tok.type = TOKEN_SEMICOLON; strcpy(tok.value, ";"); return tok; }
        if (c == '+') { tok.type = TOKEN_PLUS; strcpy(tok.value, "+"); return tok; }
        if (c == '-') { tok.type = TOKEN_MINUS; strcpy(tok.value, "-"); return tok; }
        if (c == '*') { tok.type = TOKEN_STAR; strcpy(tok.value, "*"); return tok; }

        /* Single/Double Slash & Line Comments */
        if (c == '/') {
            int next_c = fgetc(file);
            if (next_c == '/') {
                while ((c = fgetc(file)) != EOF && c != '\n');
                if (c == '\n') current_line++;
                continue;
            } else {
                ungetc(next_c, file);
                tok.type = TOKEN_SLASH;
                strcpy(tok.value, "/");
                return tok;
            }
        }

        if (c == '=') {
            int next_c = fgetc(file);
            if (next_c == '=') {
                tok.type = TOKEN_EQ;
                strcpy(tok.value, "==");
            } else {
                ungetc(next_c, file);
                tok.type = TOKEN_ASSIGN;
                strcpy(tok.value, "=");
            }
            return tok;
        }

        if (c == '!') {
            int next_c = fgetc(file);
            if (next_c == '=') {
                tok.type = TOKEN_EQ;
                strcpy(tok.value, "!=");
            } else {
                ungetc(next_c, file);
                tok.type = TOKEN_NOT;
                strcpy(tok.value, "!");
            }
            return tok;
        }

        if (c == '&') {
            int next_c = fgetc(file);
            if (next_c == '&') {
                tok.type = TOKEN_AND;
                strcpy(tok.value, "&&");
                return tok;
            } else {
                ungetc(next_c, file);
            }
        }

        if (c == '|') {
            int next_c = fgetc(file);
            if (next_c == '|') {
                tok.type = TOKEN_OR;
                strcpy(tok.value, "||");
                return tok;
            } else {
                ungetc(next_c, file);
            }
        }
    }

    tok.line = current_line;
    tok.type = TOKEN_EOF;
    tok.value[0] = '\0';
    return tok;
}