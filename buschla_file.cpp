#include "buschla_file.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

BuschlaFile* tryLoadBuschlaFile(const char* fileName) {
#define ERROR(fmt, ...) fprintf(stderr, "%s:%s:%d " fmt, __FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define SEEK(pos) { \
    int ret = fseek(file, (pos), SEEK_SET); \
    if (ret != 0) { \
        ERROR("fseek to %d failed. returned %d: %s\n", (pos), ret, strerror(ret)); \
        ON_ERROR \
    } \
}
#define READ(ptr, size) { \
    size_t read = fread((ptr), 1, (size), file); \
    if (read != (size)) { \
        ERROR("fread of %ld bytes failed. returned %ld\n", (size_t)(size), read); \
        ON_ERROR \
    } \
}

#define ON_ERROR { return NULL; }

    FILE* file = fopen(fileName, "r");
    if (file == NULL) {
        ERROR("fopen(%s): %s\n", fileName, strerror(errno));
        ON_ERROR
    }

    BuschlaFileHeader header;
    READ(&header, sizeof(BuschlaFileHeader));

    // TODO: These assertions should be handled more gracefully!
    assert(header.headerSize == sizeof(BuschlaFileHeader));
    assert(header.logLineStride == sizeof(LogLine));
    assert(strncmp(header.magic, "BUSCHLA", 7) == 0);

    assert(header.totalSize == header.headerSize + header.logLineCount * header.logLineStride + header.textBufferSize);
    // NOTE: This assumes the text buffer is at the end (which it probably always will be)
    assert(header.totalSize == header.textBufferOffset + header.textBufferSize);

    void* memory = malloc(header.totalSize);
    assert(memory != NULL);

#undef ON_ERROR
#define ON_ERROR { free(memory); return NULL; }

    SEEK(0);
    READ(memory, header.totalSize);

    int closeRet = fclose(file);
    if (closeRet != 0) {
        ERROR("fclose: %s\n", strerror(errno));
        ON_ERROR
    }

    BuschlaFile* buschlaFile = (BuschlaFile*)malloc(sizeof(BuschlaFile));
    assert(buschlaFile != NULL);

    // NOTE: file->header is the owning pointer.
    buschlaFile->header = (BuschlaFileHeader*)memory;
    buschlaFile->logLines = (LogLine*)((char*)memory + header.logLinesOffset);

    // Resolve relative string addresses //
    for (uint32_t i = 0; i < header.logLineCount; ++i) {
        LogLine* logLine = buschlaFile->logLines + i;
        const char* txt = ((char*)memory + (uint64_t)logLine->str.txt);
        logLine->str.txt = txt;
    }

    return buschlaFile;

#undef ON_ERROR
#undef SEEK
#undef READ
#undef ERROR
}

void freeBuschlaFile(BuschlaFile* file) {
    assert(file != NULL);
    assert(file->header != NULL);

    // NOTE: file->header is the owning pointer.
    free(file->header);
    free(file);
}
