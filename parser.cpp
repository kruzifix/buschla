#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buschla_file.h"

// We want to store:
// - All of the text of the log lines
// - Info per LogLine
//      text ptr
//      frameNumber
//      time
//      values?
// - List of keywords, so we can filter
// - For each value:
//      meta, i.e. min, max, avg, ...
//      array of actual values (one for each frame?)
//      if a frame did not log this value, somehow store hat?

// NOTES: How do we best do filtering by keywords/values?
// We assign each keyword an index
// Each LogLine has a bitfield corresponding to which keywords it contains
// The bitfield will be very sparse
// Maybe its better to store a list of indices ?
// OR we could only store sections of the bitfield?


// Returns pointer to statically allocated buffer.
// DO NOT store this pointer!
// DO NOT free this pointer!
// The contents will be modified the next time fetchLine is called.
// If an error occured while reading, NULL is returned instead.
static char* fetchLine(FILE* stream, uint32_t* lengthOut) {
    static char* buffer = NULL;
    static uint32_t capacity = 0;

    if (stream == NULL) {
        *lengthOut = 0;
        return NULL;
    }

    uint32_t length = 0;
    while (true) {
        char c;
        size_t count = fread(&c, 1, 1, stream);
        if (count != 1) {
            if (length == 0) {
                *lengthOut = 0;
                return NULL;
            }

            break;
        }

        if (c == '\n') {
            break;
        }

        // Ignore other non-printable characters
        // 0x7F is DEL
        if (ASCII_IS_CONTROL(c) || UTF8_IS_CONTINUATION(c)) {
            continue;
        }

        bool isAscii = c >= 0 && c < 0x80;
        bool isCodePointStart = UTF8_IS_START(c);

        bool addToBuffer = isAscii;
        if (isCodePointStart) {
            c = '?';
            addToBuffer = true;
        }

        if (addToBuffer) {
            assert(length <= capacity);
            if (capacity == 0 || length == capacity) {
                capacity = (capacity == 0) ? 4096 : (capacity + capacity / 2);
                buffer = (char*)realloc(buffer, capacity);
            }

            buffer[length] = c;
            ++length;
        }
    }

    *lengthOut = length;
    return buffer;
}

#define TOKEN_KINDS(X) \
    X(TOK_EOF) \
    X(TOK_SINGLE_SPECIAL) \
    X(TOK_INTEGER) \
    X(TOK_HEX) \
    X(TOK_BINARY) \
    X(TOK_FLOAT) \
    X(TOK_WORD) \
    X(TOK_PATH)
// TODO: DateTime

typedef enum {
#define X(x) x,
    TOKEN_KINDS(X)
#undef X
} TokenKind;

const char* tokenKindStrs[] = {
#define X(x) #x,
    TOKEN_KINDS(X)
#undef X
};

typedef struct {
    StrView str;
    const char* pos;

    TokenKind tokenKind;
    StrView tokenStr;
} Lexer;

static bool isWhiteSpace(char c) {
    return c == ' ' || c == '\t';
}

static bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

static bool isHexDigit(char c) {
    return (c >= '0' && c <= '9') ||
        (c >= 'a' && c <= 'f') ||
        (c >= 'A' && c <= 'F');
}

static bool isLetter(char c) {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z');
}

static bool isPath(char c) {
    return (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z') ||
        c == ':' || c == '.' || c == '/' || c == '\\';
}

static bool nextToken(Lexer* lex) {
#define RETURN_TOKEN(kind) { \
    lex->tokenKind = (kind); \
    lex->pos = p; \
    lex->tokenStr.txt = tokenStart; \
    lex->tokenStr.len = p - tokenStart; \
    return true; \
}

    const char* p = lex->pos;
    while (isWhiteSpace(*p)) {
        ++p;
    }

    const char* tokenStart = p;
    switch (*p) {
    case '\0':
        return false;
    case '0': {
        if (p[1] == 'x' && isHexDigit(p[2])) {
            p += 2;
            while (isHexDigit(*p)) {
                ++p;
            }

            RETURN_TOKEN(TOK_HEX)
        }

        if (p[1] == 'b') {
            p += 2;
            while (*p == '0' || *p == '1') {
                ++p;
            }
            RETURN_TOKEN(TOK_BINARY)
        }
    } // Fall through on purpose!
    case '1': case '2': case '3':
    case '4': case '5': case '6':
    case '7': case '8': case '9': {
    lex_integer:
        while (isDigit(*p)) {
            ++p;
        }

        if (*p == 'e' || *p == 'E') {
            goto lex_float_exponent;
        }

        if (*p == '.') {
            ++p;
            while (isDigit(*p)) {
                ++p;
            }

            if (*p == 'e' || *p == 'E') {
            lex_float_exponent:
                ++p;
                if (*p == '+' || *p == '-') {
                    ++p;
                }
                while (isDigit(*p)) {
                    ++p;
                }
            }

            RETURN_TOKEN(TOK_FLOAT)
        }

        RETURN_TOKEN(TOK_INTEGER)
    } break;
    case '-':
        if (p[1] == '.' && isDigit(p[2])) {
            ++p;
            goto lex_integer;
        }
    case '.':
    case '+': {
        if (isDigit(p[1])) {
            ++p;
            goto lex_integer;
        }
    } // Fall through on purpose!
    case ':': case '=': case '|':
    case '(': case ')':
    case '[': case ']':
    case '<': case '>': {
    lex_single_special:
        ++p;
        RETURN_TOKEN(TOK_SINGLE_SPECIAL)
    } break;
    case '~': {
        if (p[1] != '/') {
            goto lex_single_special;
        }
    } // Fall through on purpose!
    case '/': {
    lex_path:
        while (isPath(*p)) {
            ++p;
        }
        RETURN_TOKEN(TOK_PATH)
    } break;
    default: {
        // Windows absolute paths start with 'A:/' or 'A:\'
        if (isLetter(*p) && p[1] == ':' && (p[2] == '/' || p[2] == '\\')) {
            goto lex_path;
        }

        while (isLetter(*p) || *p == '_') {
            ++p;
        }
        RETURN_TOKEN(TOK_WORD)
    } break;
    }

    return false;

#undef RETURN_TOKEN
}

static void parseLine(LogLine* line) {
    // go through line token by token, try to parse frame number, frame time, values and keywords.

    Lexer lex;
    memcpy(&lex.str, &line->str, sizeof(StrView));
    lex.pos = lex.str.txt;

    printf("parsing '%s':\n", line->str.txt);

    // NOTE: If we need to backtrack while parsing,
    // that should be rather easy, 
    // i.e. store the lexers pos and restore it

    while (nextToken(&lex)) {
        printf("(%s): '%.*s'\n", tokenKindStrs[lex.tokenKind], lex.tokenStr.len, lex.tokenStr.txt);

        if (lex.tokenStr.len == 0) {
            assert(0 && "WHAT THE HELL=!=?");
        }
    }
}

#define _STR_(x) #x
#define STR(x) _STR_(x)

static int writeOutput(FILE* file, Chars* textBuffer, LogLines* logLines) {
    // TODO: I like the structure that we have in tryLoadBuschlaFile, also implement WRITE properly and put it in a header file!
#define ERROR(fmt, ...) fprintf(stderr, __FILE__ ":" STR(__LINE__) " " fmt, __VA_ARGS__)
#define SEEK(pos) { int ret = fseek(file, (pos), SEEK_SET); if (ret != 0) { ERROR("fseek to %d failed. returned %d: %s\n", (pos), ret, strerror(ret)); return 105; } }
#define WRITE(ptr, size) { size_t written = fwrite((ptr), 1, (size), file); if (written != (size)) { ERROR("fwrite of '%s' failed\n", #ptr); return 110; } }

    BuschlaFileHeader header;
    uint32_t headerSize = (uint32_t)sizeof(BuschlaFileHeader);
    memset(&header, 0xdc, headerSize);

    header.headerSize = headerSize;

    memcpy(header.magic, "BUSCHLA", sizeof(header.magic));
    header.version = 1;

    uint32_t logLineStride = (uint32_t)sizeof(LogLine);
    header.logLineStride = logLineStride;

    uint32_t logLinesOffset = headerSize;
    header.logLinesOffset = logLinesOffset;

    uint32_t logLineCount = logLines->count;
    header.logLineCount = logLineCount;

    uint32_t textBufferOffset = logLinesOffset + logLineCount * logLineStride;
    header.textBufferOffset = textBufferOffset;

    uint32_t currentLogLineOffset = logLinesOffset;
    uint32_t currentTextBufferOffset = textBufferOffset;
    for (uint32_t i = 0; i < logLines->count; ++i) {
        uint32_t textAddress = currentTextBufferOffset;

        LogLine* logLine = logLines->items + i;
        uint32_t textLength = logLine->str.len;

        SEEK(currentTextBufferOffset);
        WRITE(logLine->str.txt, textLength + 1);
        currentTextBufferOffset += textLength + 1;

        logLine->str.txt = (const char*)(uint64_t)textAddress;

        SEEK(currentLogLineOffset);
        WRITE(logLine, logLineStride);
        currentLogLineOffset += logLineStride;
    }

    assert(currentLogLineOffset == textBufferOffset);

    header.textBufferSize = currentTextBufferOffset - textBufferOffset;
    header.totalSize = textBufferOffset + header.textBufferSize;

    SEEK(0);
    WRITE(&header, headerSize);

    return 0;

#undef WRITE
#undef ERROR
#undef SEEK
}

static void printUsage(int argc, char** argv) {
    printf("Usage: %s <input file path>\n", argv[0]);
}

int main(int argc, char** argv) {
    Timer timer;
    timerBegin(&timer);

    // TODO: What kind of command line options do we want?
    // and how do we parse them?

    // FIXME: For now: assume the first option is the file name we want to read.

    if (argc != 2) {
        printUsage(argc, argv);
        return 1;
    }

    //# -------------- Read Input -------------- #//

    char* fileName = argv[1];

    printf("opening file '%s' for read\n", fileName);
    FILE* inputFile = fopen(fileName, "r");
    if (inputFile == NULL) {
        perror("fopen input");
        return 50;
    }

    Chars textBuffer;
    memset(&textBuffer, 0, sizeof(Chars));
    LogLines logLines;
    memset(&logLines, 0, sizeof(LogLines));

    printf("parsing log lines\n");

    int lines = 0;
    while (true) {
        StrView lineView;
        lineView.len = 0;
        lineView.txt = fetchLine(inputFile, &lineView.len);

        if (lineView.txt == NULL) {
            break;
        }

        if (lineView.len > 0) {
            LogLine* logLine = da_append_get(&logLines);
            logLine->lineNum = lines + 1;
            logLine->str.txt = ca_commit_view(&textBuffer, lineView);
            logLine->str.len = lineView.len;

            parseLine(logLine);
        }

        ++lines;
    }

    printf("parsed %u lines\n", lines);

    int fcloseRet = fclose(inputFile);
    if (fcloseRet != 0) {
        perror("fclose input");
    }

    printf("finished parsing file\n");

    // ca_dump(stdout, &textBuffer);

    //# -------------- Write Output -------------- #//

    const char* outputFileName = "out.buschla";
    printf("opening file '%s' for write\n", outputFileName);
    FILE* outputFile = fopen(outputFileName, "w");
    if (outputFile == NULL) {
        perror("fopen output");
        return 100;
    }

    int exitCode = writeOutput(outputFile, &textBuffer, &logLines);

    // TODO: if an error occured while writing, should we delete the output file?

    fcloseRet = fclose(outputFile);
    if (fcloseRet != 0) {
        perror("fclose output");
    }

    timerEnd(&timer);
    printf("finished writing file\ntook %.3fms\n", timer.elapsedMs);

    // #define OFFSET(st, m) ((size_t)&(((st*)0)->m))
    // #define MEMBER(st, m) printf("  %8ld: %s (%ld bytes)\n", OFFSET(st, m), #m, sizeof(st::m));
    //     printf("LogLine (%ld bytes):\n", sizeof(LogLine));
    //     MEMBER(LogLine, str);
    //     MEMBER(LogLine, lineNum);

    //     printf("BuschlaFileHeader (%ld bytes):\n", sizeof(BuschlaFileHeader));
    //     MEMBER(BuschlaFileHeader, magic);
    //     MEMBER(BuschlaFileHeader, version);
    //     MEMBER(BuschlaFileHeader, headerSize);
    //     MEMBER(BuschlaFileHeader, totalSize);
    //     MEMBER(BuschlaFileHeader, logLineCount);
    //     MEMBER(BuschlaFileHeader, logLineStride);
    //     MEMBER(BuschlaFileHeader, logLinesOffset);
    //     MEMBER(BuschlaFileHeader, textBufferSize);
    //     MEMBER(BuschlaFileHeader, textBufferOffset);

    return exitCode;
}
