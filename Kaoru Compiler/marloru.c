// marloru.c
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "mtypes.h"

#define MAX_LINES 100
#define MAX_LINE_LEN 128
#define FILE_TREE_WIDTH 20

/* State Marloru Interactive Editor */
typedef struct {
    char lines[MAX_LINES][MAX_LINE_LEN];
    int line_count;
    int cursor_x;
    int cursor_y;
    int indent_level;
    
    // File Tree State
    char file_list[10][32];
    int file_count;
    int selected_file_idx;
    bool active_in_editor; // true = Editor, false = choose in File Tree
} EditorState;

static EditorState state;

/* =========================================================================
 * Terminal Control (ANSI Escape Sequences)
 * ========================================================================= */
static void clear_screen() {
    printf("\033[2J\033[H");
}

static void move_cursor(int row, int col) {
    printf("\033[%d;%dH", row, col);
}


void marloru_load() {
    clear_screen();
    state.line_count = 0;
    state.cursor_x = 0;
    state.cursor_y = 0;
    state.indent_level = 0;
    state.active_in_editor = false; //start on File Tree 
    state.file_count = 0;
    state.selected_file_idx = 0;
    memset(state.lines, 0, sizeof(state.lines));

    /* 1. read real file in Directory (mlov,.ml) */
    DIR *d = opendir(".");
    if (d) {
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            char *dot = strrchr(dir->d_name, '.');
            if (dot && (strcmp(dot, ".mlov") == 0 || strcmp(dot, ".ml") == 0)) {
                if (state.file_count < 10) {
                    strncpy(state.file_list[state.file_count], dir->d_name, sizeof(state.file_list[0]) - 1);
                    state.file_list[state.file_count][sizeof(state.file_list[0]) - 1] = '\0';
                    state.file_count++;
                }
            }
        }
        closedir(d);
    }

    /* 2. if found .ml, load to Editor */
    if (state.file_count > 0) {
        FILE *f = fopen(state.file_list[0], "r");
        if (f) {
            char buffer[MAX_LINE_LEN];
            while (fgets(buffer, sizeof(buffer), f) && state.line_count < MAX_LINES) {
                // delete newline character attach fgets
                buffer[strcspn(buffer, "\r\n")] = 0;
                strncpy(state.lines[state.line_count], buffer, MAX_LINE_LEN - 1);
                state.lines[state.line_count][MAX_LINE_LEN - 1] = '\0';
                state.line_count++;
            }
            fclose(f);
        }
    }

    /* 3. Fallback: if not found .ml */
    if (state.line_count == 0) {
        state.line_count = 1;
        state.lines[0][0] = '\0';
    }
}

void marloru_draw() {
    clear_screen();

    /* 1. Draw Header Bar */
    printf("\033[7m=== MARLORU EDITOR (Kaoru Compiler Built-in TUI) === [ESC: Exit | TAB: Switch Tree/Editor]\033[0m\n");

    /* 2. Draw File Tree (Left Panel) & Text Editor (Right Panel) */
    int max_rows = 15;
    for (int r = 0; r < max_rows; r++) {
        // --- Left Panel: File Tree ---
        if (r < state.file_count) {
            if (!state.active_in_editor && r == state.selected_file_idx) {
                printf("\033[7m> %-16s\033[0m| ", state.file_list[r]); // Highlight Selection
            } else {
                printf("  %-16s| ", state.file_list[r]);
            }
        } else {
            printf("                  | ");
        }

        // --- Right Panel: Text Editor ---
        if (r < state.line_count) {
            printf("%2d | %s\n", r + 1, state.lines[r]);
        } else {
            printf("   |\n");
        }
    }

    /* 3. Draw Footer Status Bar */
    printf("\033[7m[File: %s] [Mode: %s] [Line: %d, Col: %d] [Indent Scope: %d]\033[0m\n",
           state.file_list[state.selected_file_idx],
           state.active_in_editor ? "EDITOR" : "FILE TREE",
           state.cursor_y + 1, state.cursor_x + 1,
           state.indent_level);
    if (state.active_in_editor) {
        int screen_row = 3 + state.cursor_y;
        int screen_col = FILE_TREE_WIDTH + 8 + state.cursor_x;
        move_cursor(screen_row, screen_col);
    }
    fflush(stdout);
}

bool marloru_update() {
    int c = getchar();

    if (c == 27) { // ESC Key -> quit Editor
        return false; 
    }

    /* switch control between File Tree and Editor or Key ']' or TAB */
    if (c == '\t') {
        state.active_in_editor = !state.active_in_editor;
        return true;
    }

    /* -------------------------------------------------------------------------
     * Mode 1: File Tree Navigation
     * ------------------------------------------------------------------------- */
    if (!state.active_in_editor) {
        if (c == 'w' || c == 'W') { 
            if (state.selected_file_idx > 0) state.selected_file_idx--;
        } else if (c == 's' || c == 'S') { 
            if (state.selected_file_idx < state.file_count - 1) state.selected_file_idx++;
        } else if (c == '\n') { 
            state.active_in_editor = true;
        }
        return true;
    }

    /* -------------------------------------------------------------------------
     * Mode 2: Editor Logic & Scope Auto-Indentation
     * ------------------------------------------------------------------------- */
    if (c == '\n') { 
        int last_char_idx = strlen(state.lines[state.cursor_y]) - 1;
        if (last_char_idx >= 0 && state.lines[state.cursor_y][last_char_idx] == '{') {
            state.indent_level++;
        }

        state.cursor_y++;
        if (state.cursor_y >= state.line_count) {
            state.line_count++;
        }
        state.cursor_x = 0;
        for (int i = 0; i < state.indent_level * 4; i++) {
            state.lines[state.cursor_y][state.cursor_x++] = ' ';
        }
        state.lines[state.cursor_y][state.cursor_x] = '\0';

    } else if (c == '}') { 
        if (state.indent_level > 0) state.indent_level--;
        state.lines[state.cursor_y][state.cursor_x++] = '}';
        state.lines[state.cursor_y][state.cursor_x] = '\0';

    } else if (c == 127 || c == '\b') { // Backspace
        if (state.cursor_x > 0) {
            state.cursor_x--;
            state.lines[state.cursor_y][state.cursor_x] = '\0';
        }
    } else if (c >= 32 && c <= 126) { // Printable Characters
        if (state.cursor_x < MAX_LINE_LEN - 1) {
            state.lines[state.cursor_y][state.cursor_x++] = (char)c;
            state.lines[state.cursor_y][state.cursor_x] = '\0';
        }
    }

    return true;
}

void run_marloru_editor() {
    marloru_load();
    marloru_draw();

    bool running = true;
    while (running) {
        running = marloru_update();
        if (running) {
            marloru_draw();
        }
    }

    clear_screen();
    printf("[Marloru]: Exited Interactive Editor Session.\n");
}