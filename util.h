#pragma once

#include <bits/types/FILE.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef struct {
    int readFd;
    int writeFd;
} Pipe;

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

#define TODO(msg) (assert(false && #msg))

#define TERM_COL_CLEAR "\033[0m"
#define TERM_COL_BLACK_BOLD "\033[30;1m"
#define TERM_COL_RED_BOLD "\033[31;1m"
#define TERM_COL_GREEN_BOLD "\033[32;1m"
#define TERM_COL_YELLOW_BOLD "\033[33;1m"
#define TERM_COL_BLUE_BOLD "\033[34;1m"
#define TERM_COL_MAGENTA_BOLD "\033[35;1m"
#define TERM_COL_CYAN_BOLD "\033[36;1m"
#define TERM_COL_WHITE_BOLD "\033[37;1m"

#define ASCII_IS_CONTROL(c) ((unsigned char)(c) < 0x20 || (c) == 0x7F)

#define UTF8_IS_CONTINUATION(c) (((c) & 0xC0) == 0x80)
#define UTF8_IS_START(c) ((unsigned char)(c) >= 0xC0)
#define UTF8_IS_START_2BYTE(c) (((c) & 0xE0) == 0xC0)
#define UTF8_IS_START_3BYTE(c) (((c) & 0xF0) == 0xE0)
#define UTF8_IS_START_4BYTE(c) (((c) & 0xF8) == 0xF0)


size_t getStringLength(const char* str);

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

void hexdump(FILE* stream, void* memory, size_t size, size_t itemSize = 0);

typedef struct {
    uint64_t begin;
    uint64_t end;
    float elapsedMs;
} Timer;

#define TIME_SCOPE(name) Timer name; for (bool latch = timerBegin(&name); latch; latch = timerEnd(&name))

bool timerBegin(Timer* t);
bool timerEnd(Timer* t);
