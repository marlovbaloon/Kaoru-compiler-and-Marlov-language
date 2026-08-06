#include "marloru.h"

#define MARLORU_VERSION "1.0.0"

/* Syntax highlight types */
#define HL_NORMAL 0
#define HL_NONPRINT 1
#define HL_COMMENT 2   
#define HL_KEYWORD1 4
#define HL_KEYWORD2 5
#define HL_STRING 6
#define HL_NUMBER 7
#define HL_MATCH 8      

#define HL_HIGHLIGHT_STRINGS (1<<0)
#define HL_HIGHLIGHT_NUMBERS (1<<1)

struct editorSyntax {
    char **filematch;
    char **keywords;
    char singleline_comment_start[4];
    char singleline_comment_end[4];
    int flags;
};

typedef struct erow {
    int idx;            
    int size;           
    int rsize;          
    char *chars;        
    char *render;       
    unsigned char *hl;  
    int hl_oc;          
} erow;

struct editorConfig {
    int cx, cy;  
    int rowoff;     
    int coloff;     
    int screenrows; 
    int screencols; 
    int numrows;    
    int rawmode;    
    erow *row;      
    int dirty;      
    char *filename; 
    char statusmsg[80];
    time_t statusmsg_time;
    struct editorSyntax *syntax;    
};

static struct editorConfig E;

enum KEY_ACTION {
    KEY_NULL = 0,
    CTRL_C = 3,
    CTRL_F = 6,
    CTRL_H = 8,
    TAB = 9,
    CTRL_L = 12,
    ENTER = 13,
    CTRL_Q = 17,
    CTRL_S = 19,
    ESC = 27,
    BACKSPACE = 127,
    ARROW_LEFT = 1000,
    ARROW_RIGHT,
    ARROW_UP,
    ARROW_DOWN,
    DEL_KEY,
    HOME_KEY,
    END_KEY,
    PAGE_UP,
    PAGE_DOWN
};

void editorSetStatusMessage(const char *fmt, ...);

/* =========================== Marlov Syntax DB ========================= */

char *MARLOV_HL_extensions[] = {".ml", ".mlov", NULL};
char *MARLOV_HL_keywords[] = {
    /* Directives & Keywords */
    "@sys.disk.read","@sys","if","else","while","for","return",
    /* Types */
    "@int|","@str|","@void|","@bool|", NULL
};

struct editorSyntax HLDB[] = {
    {
        MARLOV_HL_extensions,
        MARLOV_HL_keywords,
        "/--", "--/", /* Marlov Comment Syntax /-- ... --/ */
        HL_HIGHLIGHT_STRINGS | HL_HIGHLIGHT_NUMBERS
    }
};

#define HLDB_ENTRIES (sizeof(HLDB)/sizeof(HLDB[0]))

/* ======================= Low level terminal handling ====================== */

#ifdef MARLORU_LINUX
static struct termios orig_termios;

void disableRawMode(void) {
    if (E.rawmode) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        E.rawmode = 0;
    }
}

int enableRawMode(void) {
    if (E.rawmode) return 0;
    if (!isatty(STDIN_FILENO)) return -1;
    atexit(disableRawMode);
    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) return -1;

    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_cflag |= (CS8);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 1;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0) return -1;
    E.rawmode = 1;
    return 0;
}
#else
static DWORD orig_in_mode, orig_out_mode;

void disableRawMode(void) {
    if (E.rawmode) {
        HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleMode(hIn, orig_in_mode);
        SetConsoleMode(hOut, orig_out_mode);
        E.rawmode = 0;
    }
}

int enableRawMode(void) {
    if (E.rawmode) return 0;
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    GetConsoleMode(hIn, &orig_in_mode);
    GetConsoleMode(hOut, &orig_out_mode);
    atexit(disableRawMode);

    DWORD raw_in = orig_in_mode & ~(ENABLE_ECHO_INPUT | ENABLE_LINE_INPUT | ENABLE_PROCESSED_INPUT);
    DWORD raw_out = orig_out_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING;

    SetConsoleMode(hIn, raw_in);
    SetConsoleMode(hOut, raw_out);
    E.rawmode = 1;
    return 0;
}
#endif

int editorReadKey(void) {
#ifdef MARLORU_LINUX
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) == 0);
    if (nread == -1) return ESC;

    if (c == ESC) {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return ESC;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return ESC;

        if (seq[0] == '[') {
            if (seq[1] >= '0' && seq[1] <= '9') {
                if (read(STDIN_FILENO, &seq[2], 1) != 1) return ESC;
                if (seq[2] == '~') {
                    switch (seq[1]) {
                        case '3': return DEL_KEY;
                        case '5': return PAGE_UP;
                        case '6': return PAGE_DOWN;
                    }
                }
            } else {
                switch (seq[1]) {
                    case 'A': return ARROW_UP;
                    case 'B': return ARROW_DOWN;
                    case 'C': return ARROW_RIGHT;
                    case 'D': return ARROW_LEFT;
                    case 'H': return HOME_KEY;
                    case 'F': return END_KEY;
                }
            }
        }
        return ESC;
    }
    return c;
#else
    int c = _getch();
    if (c == 0 || c == 224) {
        switch (_getch()) {
            case 72: return ARROW_UP;
            case 80: return ARROW_DOWN;
            case 75: return ARROW_LEFT;
            case 77: return ARROW_RIGHT;
            case 83: return DEL_KEY;
            case 71: return HOME_KEY;
            case 79: return END_KEY;
            case 73: return PAGE_UP;
            case 81: return PAGE_DOWN;
        }
    }
    if (c == '\r') return ENTER;
    if (c == 8) return BACKSPACE;
    return c;
#endif
}

int getWindowSize(int *rows, int *cols) {
#ifdef MARLORU_LINUX
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) return -1;
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
#else
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        *cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        *rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
        return 0;
    }
    return -1;
#endif
}

/* ====================== Syntax Highlighting Engine ===================== */

int is_separator(int c) {
    return c == '\0' || isspace(c) || strchr(",.()+-/*=~%[];{}", c) != NULL;
}

void editorUpdateSyntax(erow *row) {
    row->hl = realloc(row->hl, row->rsize);
    memset(row->hl, HL_NORMAL, row->rsize);
    if (E.syntax == NULL) return;

    char **keywords = E.syntax->keywords;
    char *p = row->render;
    int i = 0, prev_sep = 1, in_string = 0;

    while (*p) {
        /* Marlov Comment Parsing: /-- ... --/ */
        if (strncmp(p, "/--", 3) == 0) {
            memset(row->hl + i, HL_COMMENT, row->rsize - i);
            break;
        }

        if (in_string) {
            row->hl[i] = HL_STRING;
            if (*p == '\\' && *(p + 1)) {
                row->hl[i + 1] = HL_STRING;
                p += 2; i += 2;
                continue;
            }
            if (*p == in_string) in_string = 0;
            p++; i++;
            continue;
        } else if (*p == '"' || *p == '\'') {
            in_string = *p;
            row->hl[i] = HL_STRING;
            p++; i++;
            prev_sep = 0;
            continue;
        }

        if (isdigit(*p) && (prev_sep || (i > 0 && row->hl[i - 1] == HL_NUMBER))) {
            row->hl[i] = HL_NUMBER;
            p++; i++;
            prev_sep = 0;
            continue;
        }

        if (prev_sep) {
            int j;
            for (j = 0; keywords[j]; j++) {
                int klen = strlen(keywords[j]);
                int kw2 = keywords[j][klen - 1] == '|';
                if (kw2) klen--;

                if (!memcmp(p, keywords[j], klen) && is_separator(*(p + klen))) {
                    memset(row->hl + i, kw2 ? HL_KEYWORD2 : HL_KEYWORD1, klen);
                    p += klen; i += klen;
                    break;
                }
            }
            if (keywords[j] != NULL) {
                prev_sep = 0;
                continue;
            }
        }
        prev_sep = is_separator(*p);
        p++; i++;
    }
}

int editorSyntaxToColor(int hl) {
    switch (hl) {
        case HL_COMMENT: return 36;     /* Cyan */
        case HL_KEYWORD1: return 33;    /* Yellow (@sys, if) */
        case HL_KEYWORD2: return 32;    /* Green (@int, @str) */
        case HL_STRING: return 35;      /* Magenta */
        case HL_NUMBER: return 31;      /* Red */
        default: return 37;             /* White */
    }
}

void editorSelectSyntaxHighlight(char *filename) {
    if (!filename) return;
    for (unsigned int j = 0; j < HLDB_ENTRIES; j++) {
        struct editorSyntax *s = HLDB + j;
        unsigned int i = 0;
        while (s->filematch[i]) {
            char *p = strstr(filename, s->filematch[i]);
            if (p && p[strlen(s->filematch[i])] == '\0') {
                E.syntax = s;
                return;
            }
            i++;
        }
    }
}

/* ======================= Editor Operations ======================= */

void editorUpdateRow(erow *row) {
    int tabs = 0, j, idx = 0;
    for (j = 0; j < row->size; j++) if (row->chars[j] == TAB) tabs++;

    free(row->render);
    row->render = malloc(row->size + tabs * 7 + 1);

    for (j = 0; j < row->size; j++) {
        if (row->chars[j] == TAB) {
            row->render[idx++] = ' ';
            while (idx % 8 != 0) row->render[idx++] = ' ';
        } else {
            row->render[idx++] = row->chars[j];
        }
    }
    row->rsize = idx;
    row->render[idx] = '\0';
    editorUpdateSyntax(row);
}

void editorInsertRow(int at, char *s, size_t len) {
    if (at > E.numrows) return;
    E.row = realloc(E.row, sizeof(erow) * (E.numrows + 1));
    if (at != E.numrows) {
        memmove(E.row + at + 1, E.row + at, sizeof(E.row[0]) * (E.numrows - at));
        for (int j = at + 1; j <= E.numrows; j++) E.row[j].idx++;
    }
    E.row[at].size = len;
    E.row[at].chars = malloc(len + 1);
    memcpy(E.row[at].chars, s, len);
    E.row[at].chars[len] = '\0';
    E.row[at].hl = NULL;
    E.row[at].render = NULL;
    E.row[at].rsize = 0;
    E.row[at].idx = at;
    editorUpdateRow(&E.row[at]);
    E.numrows++;
    E.dirty++;
}

void editorFreeRow(erow *row) {
    free(row->render);
    free(row->chars);
    free(row->hl);
}

void editorDelRow(int at) {
    if (at >= E.numrows) return;
    editorFreeRow(&E.row[at]);
    memmove(E.row + at, E.row + at + 1, sizeof(E.row[0]) * (E.numrows - at - 1));
    for (int j = at; j < E.numrows - 1; j++) E.row[j].idx++;
    E.numrows--;
    E.dirty++;
}

void editorInsertChar(int c) {
    int filerow = E.rowoff + E.cy;
    if (filerow == E.numrows) editorInsertRow(E.numrows, "", 0);
    
    erow *row = &E.row[filerow];
    int filecol = E.coloff + E.cx;
    if (filecol > row->size) filecol = row->size;

    row->chars = realloc(row->chars, row->size + 2);
    memmove(row->chars + filecol + 1, row->chars + filecol, row->size - filecol + 1);
    row->size++;
    row->chars[filecol] = c;
    editorUpdateRow(row);
    E.cx++;
    E.dirty++;
}

void editorInsertNewline(void) {
    int filerow = E.rowoff + E.cy;
    int filecol = E.coloff + E.cx;
    if (filerow >= E.numrows) {
        editorInsertRow(E.numrows, "", 0);
    } else {
        erow *row = &E.row[filerow];
        if (filecol > row->size) filecol = row->size;
        editorInsertRow(filerow + 1, row->chars + filecol, row->size - filecol);
        row = &E.row[filerow];
        row->chars[filecol] = '\0';
        row->size = filecol;
        editorUpdateRow(row);
    }
    E.cy++;
    E.cx = 0;
}

void editorDelChar(void) {
    int filerow = E.rowoff + E.cy;
    int filecol = E.coloff + E.cx;
    if (filerow >= E.numrows || (filecol == 0 && filerow == 0)) return;

    erow *row = &E.row[filerow];
    if (filecol > 0) {
        memmove(row->chars + filecol - 1, row->chars + filecol, row->size - filecol + 1);
        row->size--;
        editorUpdateRow(row);
        E.cx--;
    } else {
        E.cx = E.row[filerow - 1].size;
        editorInsertRow(filerow - 1, "", 0);
        /* Merge line functionality */
        row = &E.row[filerow];
        E.row[filerow - 1].chars = realloc(E.row[filerow - 1].chars, E.row[filerow - 1].size + row->size + 1);
        memcpy(E.row[filerow - 1].chars + E.row[filerow - 1].size, row->chars, row->size + 1);
        E.row[filerow - 1].size += row->size;
        editorUpdateRow(&E.row[filerow - 1]);
        editorDelRow(filerow);
        E.cy--;
    }
    E.dirty++;
}

/* ======================= File I/O ======================= */

int editorOpen(const char *filename) {
    free(E.filename);
    E.filename = strdup(filename);
    editorSelectSyntaxHighlight(E.filename);

    FILE *fp = fopen(filename, "r");
    if (!fp) return -1;

    char line[1024];
    while (fgets(line, sizeof(line), fp)) {
        int linelen = strlen(line);
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r')) linelen--;
        editorInsertRow(E.numrows, line, linelen);
    }
    fclose(fp);
    E.dirty = 0;
    return 0;
}

int editorSave(void) {
    if (!E.filename) return -1;
    FILE *fp = fopen(E.filename, "w");
    if (!fp) return -1;

    for (int j = 0; j < E.numrows; j++) {
        fprintf(fp, "%s\n", E.row[j].chars);
    }
    fclose(fp);
    E.dirty = 0;
    editorSetStatusMessage("[Kaoru TUI]: File saved successfully!");
    return 0;
}

/* ======================= Screen Rendering ======================= */

struct abuf { char *b; int len; };
#define ABUF_INIT {NULL, 0}

void abAppend(struct abuf *ab, const char *s, int len) {
    char *new_b = realloc(ab->b, ab->len + len);
    if (new_b) {
        memcpy(new_b + ab->len, s, len);
        ab->b = new_b;
        ab->len += len;
    }
}

void editorRefreshScreen(void) {
    struct abuf ab = ABUF_INIT;
    abAppend(&ab, "\x1b[?25l", 6);
    abAppend(&ab, "\x1b[H", 3);

    for (int y = 0; y < E.screenrows; y++) {
        int filerow = E.rowoff + y;
        if (filerow >= E.numrows) {
            if (E.numrows == 0 && y == E.screenrows / 3) {
                char welcome[80];
                int welcomelen = snprintf(welcome, sizeof(welcome),
                    "Marloru TUI Editor -- v%s (Marlov Lang Support)", MARLORU_VERSION);
                int padding = (E.screencols - welcomelen) / 2;
                if (padding) { abAppend(&ab, "~", 1); padding--; }
                while (padding--) abAppend(&ab, " ", 1);
                abAppend(&ab, welcome, welcomelen);
            } else {
                abAppend(&ab, "~\x1b[0K\r\n", 7);
            }
            continue;
        }

        erow *r = &E.row[filerow];
        int len = r->rsize - E.coloff;
        if (len > 0) {
            if (len > E.screencols) len = E.screencols;
            char *c = r->render + E.coloff;
            unsigned char *hl = r->hl + E.coloff;
            int curr_color = -1;

            for (int j = 0; j < len; j++) {
                int color = editorSyntaxToColor(hl[j]);
                if (color != curr_color) {
                    char buf[16];
                    int clen = snprintf(buf, sizeof(buf), "\x1b[%dm", color);
                    abAppend(&ab, buf, clen);
                    curr_color = color;
                }
                abAppend(&ab, c + j, 1);
            }
            abAppend(&ab, "\x1b[39m", 5);
        }
        abAppend(&ab, "\x1b[0K\r\n", 6);
    }

    /* Status Bar */
    abAppend(&ab, "\x1b[7m", 4);
    char status[80];
    int len = snprintf(status, sizeof(status), " %s - %d lines %s",
        E.filename ? E.filename : "[New File]", E.numrows, E.dirty ? "(modified)" : "");
    if (len > E.screencols) len = E.screencols;
    abAppend(&ab, status, len);
    while (len < E.screencols) { abAppend(&ab, " ", 1); len++; }
    abAppend(&ab, "\x1b[0m\r\n", 6);

    /* Message Bar */
    abAppend(&ab, "\x1b[0K", 4);
    int msglen = strlen(E.statusmsg);
    if (msglen && time(NULL) - E.statusmsg_time < 5)
        abAppend(&ab, E.statusmsg, msglen <= E.screencols ? msglen : E.screencols);

    /* Set Cursor */
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", (E.cy - E.rowoff) + 1, (E.cx - E.coloff) + 1);
    abAppend(&ab, buf, strlen(buf));
    abAppend(&ab, "\x1b[?25h", 6);

    fwrite(ab.b, ab.len, 1, stdout);
    fflush(stdout);
    free(ab.b);
}

void editorSetStatusMessage(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(E.statusmsg, sizeof(E.statusmsg), fmt, ap);
    va_end(ap);
    E.statusmsg_time = time(NULL);
}

/* ======================= Editor Loop & Main API ======================= */

void initEditor(void) {
    E.cx = 0; E.cy = 0;
    E.rowoff = 0; E.coloff = 0;
    E.numrows = 0; E.row = NULL;
    E.dirty = 0; E.filename = NULL;
    E.syntax = NULL;
    getWindowSize(&E.screenrows, &E.screencols);
    E.screenrows -= 2;
}

void run_marloru_editor(void) {
    enableRawMode();
    initEditor();
    editorSetStatusMessage("MARLORU TUI | Ctrl-S = Save | Ctrl-Q = Quit");

    while (1) {
        editorRefreshScreen();
        int c = editorReadKey();
        switch (c) {
            case CTRL_Q:
                disableRawMode();
                /* Clear Screen when exiting back to Kaoru CLI */
                printf("\x1b[2J\x1b[H");
                return;
            case CTRL_S:
                editorSave();
                break;
            case ENTER:
                editorInsertNewline();
                break;
            case BACKSPACE:
            case DEL_KEY:
                editorDelChar();
                break;
            case ARROW_UP: if (E.cy > 0) E.cy--; break;
            case ARROW_DOWN: if (E.cy < E.numrows) E.cy++; break;
            case ARROW_LEFT: if (E.cx > 0) E.cx--; break;
            case ARROW_RIGHT: E.cx++; break;
            default:
                if (!iscntrl(c) && c < 128) editorInsertChar(c);
                break;
        }
    }
}