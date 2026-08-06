#ifndef MARLORU_H
#define MARLORU_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <stdbool.h>

#if defined(_WIN32) || defined(_WIN64)
    #define MARLORU_WINDOWS
    #include <windows.h>
    #include <conio.h>
    #include <io.h>
    #ifndef STDIN_FILENO
    #define STDIN_FILENO 0
    #define STDOUT_FILENO 1
    #endif
#else
    #define MARLORU_LINUX
    #include <termios.h>
    #include <unistd.h>
    #include <sys/ioctl.h>
    #include <fcntl.h>
    #include <signal.h>
#endif

void run_marloru_editor(void);
void open_marloru_file(const char *filename);

#endif