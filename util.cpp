#include "util.h"

#include <assert.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>

size_t string_length(const char* str) {
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
    char path[PATH_MAX];
    if (callingPath[0] == '/') {
        strcpy(path, callingPath);
    }
    else {
        assert(callingPath[0] == '.');
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

        size_t len = string_length(path);
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

void hexdump(FILE* stream, void* memory, size_t size) {
    fprintf(stream, "dumping memory at %p  size: %ld\n", memory, size);
    fprintf(stream, "     ");
    for (int i = 0; i < 16; ++i) {
        fprintf(stream, "%02X ", i);
    }
    for (size_t i = 0; i < size; ++i) {
        if (i % 16 == 0) {
            if (i > 0) {
                _hexdump_printChars(stream, ((char*)memory) + (i - 16), 16);
            }
            // TODO: check size to determine how many digits the address is at maximum
            fprintf(stream, "\n%04ld ", i);
        }
        fprintf(stream, "%02X ", ((uint8_t*)memory)[i]);
    }
    size_t remainder = size % 16;
    if (remainder > 0) {
        for (size_t i = 0; i < 16 - remainder; ++i) {
            fprintf(stream, "   ");
        }
        _hexdump_printChars(stream, ((char*)memory) + (size - remainder), remainder);
    }
    putc('\n', stream);
}
