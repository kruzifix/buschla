#pragma once

#include "dynamic_array.h"
#include "util.h"

typedef struct {
    StrView str;

    // TODO: Is this really needed?
    uint32_t lineNum;

} LogLine;

DEFINE_DYNAMIC_ARRAY(LogLines, LogLine)

// NOTE: Any char* is relatively addressed (describes byte offset to string start from start of file)
typedef struct {
    // B U S C H L A
    char magic[7];

    uint8_t version;

    // Size of Header (bytes)
    uint32_t headerSize;

    // Total Blob Size (bytes)
    uint32_t totalSize;

    // Number of Log Lines
    uint32_t logLineCount;
    // Size of a single Log Lines Array entry (bytes)
    uint32_t logLineStride;
    // Start of Log Lines Array (offset in bytes)
    uint32_t logLinesOffset;

    // Size of Text Buffer (bytes)
    uint32_t textBufferSize;
    // Start of Text Buffer (offset in bytes)
    uint32_t textBufferOffset;
} BuschlaFileHeader;

typedef struct {
    BuschlaFileHeader* header;
    LogLine* logLines;
} BuschlaFile;

BuschlaFile* tryLoadBuschlaFile(const char* fileName);
void freeBuschlaFile(BuschlaFile* file);
