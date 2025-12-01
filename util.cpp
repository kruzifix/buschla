#include "util.h"

#ifdef WINDOWS
#define PATH_MAX 4096
#include <direct.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <unistd.h>
#endif

#include <assert.h>
#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include <SDL3/SDL.h>

// FIXME: Maybe its nicer to have a util_linx.cpp and util_windows.cpp
// which implement the platform variations of functions?
// for now we only have one such function, so maybe revisit when we have more!

char* tmpf(const char* fmt, ...) {
    static char* buffer = NULL;
    static uint32_t capacity = 0;

    if (fmt == NULL) {
        return NULL;
    }

    va_list args;
    va_start(args, fmt);
    int requiredSize = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    assert(requiredSize >= 0 && "tmpf: first call to vsnprintf returned a negative value");

    uint32_t requiredCapacity = (uint32_t)requiredSize + 1;
    if (capacity < requiredCapacity) {
        if (capacity == 0) {
            const uint32_t initialCapacity = 128;
            capacity = requiredCapacity < initialCapacity ? initialCapacity : requiredCapacity;
        }
        // Grow capacity by 1.5x until we have enough to fit the requirement.
        while (capacity < requiredCapacity) {
            capacity += capacity / 2;
        }

        buffer = (char*)realloc(buffer, capacity);
        assert(buffer != NULL && "tmpf: realloc failed (buy more RAM? lol!)");
    }

    va_start(args, fmt);
    int writtenSize = vsnprintf(buffer, capacity, fmt, args);
    va_end(args);

    assert(writtenSize >= 0 && "tmpf: second call to vsnprintf returned a negative value");
    assert(writtenSize == requiredSize);

    return buffer;
}

size_t getStringLength(const char* str) {
    if (str == NULL) {
        return 0;
    }

    return strlen(str);
}

#ifdef LINUX
char* getExecutableFilePath(const char* callingPath) {
    char* workingDir = get_current_dir_name();

    // the callingPath in argv can be
    // - absolute (start with /)
    // - relative (start with ./ or ../)
    // - relative? (starts without . or /)
    if (callingPath[0] == '/') {
        // yes, I know, its stupid, but its to ensure the same behavior independent of input.
        return tmpf("%s", callingPath);
    }

    char* tmp = tmpf("%s/%s", workingDir, callingPath);
    if (!cleanAbsolutePath(tmp)) {
        return NULL;
    }

    return tmp;
}
#else
bool getExecutablePath(const char* callingPath, char* dest) {
    // TODO: do actual checking of the path sizes!
    // TODO: use tmpf!
    char workingDirectory[PATH_MAX];
    if (_getcwd(workingDirectory, sizeof(workingDirectory)) == NULL) {
        return false;
    }

    // TODO: This is making a bunch of assumptions ....
    // needs to be reworked!
    strcpy(dest, workingDirectory);
    strcat(dest, "/");
    strcat(dest, callingPath);

    char* p = dest;
    while (*p) {
        if (*p == '\\') {
            *p = '/';
        }
        ++p;
    }

    return true;
}
#endif

bool cleanAbsolutePath(char* path) {
    // TODO: replace \ with /
    // TODO: WINDOWS absolute paths start differently!
    if (*path != '/') {
        return false;
    }

    char* start = path + 1;
    char* end = start;
    int totalLength = 0;
    while (*end) {
        ++end;
        ++totalLength;
    }

    // remove trailing /
    int index = totalLength - 1;
    while (start[index] == '/') {
        start[index] = '\0';
        --totalLength;
    }

    if (totalLength == 1 && start[0] == '.') {
        path[0] = '/';
        path[1] = '\0';
        return true;
    }

    // keep counter
    // iterate through parts backwards
    // - if we encounter normal part:
    //     counter == 0: prepend to output
    //     counter > 0: ignore it and decrease counter
    // - if we encounter '..': increase counter
    // - if we encounter '.': ignore it
    int collapseCounter = 0;
    int outputIndex = totalLength - 1;
    for (int endIndex = totalLength - 1; endIndex >= 0; --endIndex) {
        int startIndex = endIndex;
        while (startIndex >= 0 && start[startIndex] != '/') {
            --startIndex;
        }

        char* part = start + startIndex + 1;
        int length = endIndex - startIndex;
        bool singleDot = (length == 1) && (part[0] == '.');
        bool doubleDot = (length == 2) && (part[0] == '.') && (part[1] == '.');

        if (doubleDot) {
            ++collapseCounter;
        }
        else if (!singleDot) {
            if (collapseCounter > 0) {
                --collapseCounter;
            }
            else if (endIndex == outputIndex) {
                // no need to modify string, everything is in the correct place.
                outputIndex -= length + 1;
            }
            else {
                for (int i = length - 1; i >= 0; --i) {
                    start[outputIndex] = part[i];
                    --outputIndex;
                    assert(outputIndex >= 0);
                }
                start[outputIndex--] = '/';
            }
        }

        endIndex = startIndex;
    }

    if (collapseCounter > 0 || outputIndex == totalLength - 1) {
        path[0] = '/';
        path[1] = '\0';
        return true;
    }

    memmove(path, start + outputIndex + 1, totalLength - outputIndex + 1);
    return true;
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
#define NANOS_PER_SEC 1000000000
// The maximum time span representable is 584 years.
static uint64_t nanos_since_unspecified_epoch() {
#ifdef WINDOWS
    LARGE_INTEGER Time;
    QueryPerformanceCounter(&Time);

    static LARGE_INTEGER Frequency = { 0 };
    if (Frequency.QuadPart == 0) {
        QueryPerformanceFrequency(&Frequency);
    }

    uint64_t Secs = Time.QuadPart / Frequency.QuadPart;
    uint64_t Nanos = Time.QuadPart % Frequency.QuadPart * NANOS_PER_SEC / Frequency.QuadPart;
    return NANOS_PER_SEC * Secs + Nanos;
#else
    struct timespec ts;
    clock_gettime(TIMER_CLOCK_ID, &ts);

    return NANOS_PER_SEC * ts.tv_sec + ts.tv_nsec;
#endif
}

bool timerBegin(Timer* t) {
    t->begin = nanos_since_unspecified_epoch();
    return true;
}

bool timerEnd(Timer* t) {
    t->end = nanos_since_unspecified_epoch();
    long diffNano = t->end - t->begin;
    t->elapsedMs = diffNano * 1e-6f;
    return false;
}
