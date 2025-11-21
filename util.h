#pragma once

#include <stdbool.h>
#include <bits/types/FILE.h>
#include <cstddef>

typedef struct {
    int readFd;
    int writeFd;
} Pipe;

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

size_t string_length(const char* str);

// Pass argv[0] to callingPath.
// Assumes dest points to a buffer that is atleast PATH_MAX big!
bool getExecutablePath(const char* callingPath, char* dest);

// Modifies given 'string', inserts null terminator at last occurence of 'c'.
// Returns pointer to first character after last occurence of 'c', NULL on error (no 'c' found).
char* splitAtLastOccurence(char* src, char c);

// NOTE: concatPaths uses 'realpath' internally, which means this only works with actually existing paths!
// TODO: replace usage of 'realpath' so we can work with any directories (also imaginary trees)!
#define concatPaths(dest, ...) \
    concatPaths_(dest, ((const char*[]){__VA_ARGS__}), (sizeof((const char*[]){__VA_ARGS__})/sizeof(const char*)))

// Assumes dest points to a buffer that is big enough (i.e. atleast PATH_MAX)!
// returns return value of realpath, check errno if this returned false!
bool concatPaths_(char* dest, const char* paths[], int count);

// TODO:
void hexdump(FILE* stream, void* memory, size_t size);
