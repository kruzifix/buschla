#include "util.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

#include <SDL3/SDL.h>

size_t getStringLength(const char* str) {
    if (str == NULL) {
        return 0;
    }

    return strlen(str);
}

bool getExecutablePath(const char* callingPath, char* dest) {
    // TODO: do actual checking of the path sizes!
    // if callingPath or the concatenation of workingDirector + callingPath is bigger then PATH_MAX,
    // we will write out of bounds!
    char workingDirectory[PATH_MAX];
    if (getcwd(workingDirectory, sizeof(workingDirectory)) == NULL) {
        return false;
    }

    // the callingPath in argv can be
    // - absolute (start with /)
    // - relative (start with ./ or ../)
    // - relative? (starts without .)
    char path[PATH_MAX];
    if (callingPath[0] == '/') {
        strcpy(path, callingPath);
    }
    else {
        strcpy(path, workingDirectory);
        strcat(path, "/");
        strcat(path, callingPath);
    }

    return realpath(path, dest) != NULL;
}

char* splitAtLastOccurence(char* src, char c) {
    char* p = src;
    char* lastOccurence = NULL;
    while (*p) {
        if (*p == c) {
            lastOccurence = p;
        }
        ++p;
    }

    if (lastOccurence != NULL) {
        *lastOccurence = '\0';
        return lastOccurence + 1;
    }

    return NULL;
}

bool concatPaths_(char* dest, const char* paths[], int count) {
    char buffer[PATH_MAX];
    char* p = buffer;
    for (int i = 0; i < count; ++i) {
        const char* path = paths[i];
        while (*path == ' ') {
            ++path;
        }

        size_t len = getStringLength(path);
        if (len == 0) {
            continue;
        }

        // TODO: Remove trailing whitespaces?

        if (i > 0) {
            // Insert / between path entries
            if (p[-1] != '/' && path[0] != '/') {
                *p = '/';
                ++p;
            }
        }

        p = stpcpy(p, path);
    }

    return realpath(buffer, dest) != NULL;
}

static void _hexdump_printChars(FILE* stream, void* memory, size_t count) {
    for (size_t i = 0; i < count; ++i) {
        char c = ((char*)memory)[i];
        bool printable = c >= 0x20 && c <= 0x7E;
        putchar(printable ? c : '.');
    }
}

void hexdump(FILE* stream, void* memory, size_t size, size_t itemSize) {
    fprintf(stream, "dumping memory at %s%p%s  size: %ld (0x%lX)\n", TERM_COL_GREEN_BOLD, memory, TERM_COL_CLEAR, size, size);
    if (size > 0xFFFF) {
        fputs(TERM_COL_RED_BOLD, stream);
        fprintf(stream, "  WARNING! hexdump() currently only supports 16-bits for the size,\n    truncating the size to 0xFFFF\n");
        fputs(TERM_COL_CLEAR, stream);

        size = 0xFFFF;
    }

    fputs("     ", stream);
    fputs(TERM_COL_WHITE_BOLD, stream);
    const int rowSize = 16;
    for (int i = 0; i < rowSize; ++i) {
        fprintf(stream, "%02X ", i);
    }
    fputs(TERM_COL_CLEAR, stream);

    size_t sizeRemainder = size % rowSize;
    for (size_t i = 0; i < size; ++i) {
        if (i % rowSize == 0) {
            // TODO: check size to determine how many digits the address is at maximum
            fprintf(stream, "\n%s%04lX%s ", TERM_COL_RED_BOLD, i, TERM_COL_CLEAR);
        }

        bool setColor = true;
        if (itemSize > 0) {
            int itemIndex = i / itemSize;
            fprintf(stream, "\033[%d;%dm", 32 + (itemIndex % 6), 2 + 2 * (itemIndex % 2));
        }
        fprintf(stream, "%02X ", ((uint8_t*)memory)[i]);
        if (setColor) {
            fputs(TERM_COL_CLEAR, stream);
        }

        bool atEndOfMemory = i == size - 1;
        bool atEndOfRow = (i % rowSize) == (rowSize - 1);

        if (atEndOfMemory) {
            if (sizeRemainder == 0) {
                _hexdump_printChars(stream, ((char*)memory) + (i - rowSize), rowSize);
            }
            else {
                for (size_t i = 0; i < rowSize - sizeRemainder; ++i) {
                    fputs("   ", stream);
                }
                _hexdump_printChars(stream, ((char*)memory) + (size - sizeRemainder), sizeRemainder);
            }
        }
        else if (atEndOfRow) {
            _hexdump_printChars(stream, ((char*)memory) + (i - rowSize), rowSize);
        }
    }

    putc('\n', stream);
}

#define TIMER_CLOCK_ID CLOCK_MONOTONIC_RAW
bool timerBegin(Timer* t) {
    t->begin = SDL_GetTicksNS();
    return true;
}

bool timerEnd(Timer* t) {
    t->end = SDL_GetTicksNS();
    long diffNano = t->end - t->begin;
    t->elapsedMs = diffNano * 1e-6f;
    return false;
}
