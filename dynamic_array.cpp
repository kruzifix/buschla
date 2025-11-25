#include "dynamic_array.h"

#include "util.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

//#define DISABLE_DEBUG_PRINTING

#ifdef DISABLE_DEBUG_PRINTING
#define printf(fmt, ...) //printf(fmt, __VA_ARGS__)
#endif

void _da_reserve(_DummyDynamicArray* array, uint32_t itemSize, uint32_t requestedSize) {
    assert(array->count <= array->capacity);
    assert(requestedSize > 0);
    if (array->capacity < requestedSize) {
        printf("%s: growing capacity %u -> %u\n", __FUNCTION__, array->capacity, requestedSize);
        uint32_t bytes = requestedSize * itemSize;
        void* newItems = malloc(bytes);
        printf("%s: allocated %u bytes (itemSize is %u)\n", __FUNCTION__, bytes, itemSize);
        // NOTE: If we already have items in the array,
        // don't zero out those bytes, they will be overriden anyway!
        uint32_t offset = array->count * itemSize;
        printf("%s: count is %u, zeroing out %u bytes at offset %u\n", __FUNCTION__, array->count, bytes - offset, offset);
        // TODO: Is it bad to do this? Should the caller/user be responsible for properly initializing all their values?
        memset((uint8_t*)newItems + offset, 0, bytes - offset);

        if (array->count > 0) {
            assert(array->items != NULL);

            printf("%s: copying %u items\n", __FUNCTION__, array->count);
            memcpy(newItems, array->items, array->count * itemSize);
        }

        if (array->items != NULL) {
            free(array->items);
        }

        array->items = newItems;
        array->capacity = requestedSize;
        printf("%s: capacity is now %u\n", __FUNCTION__, array->capacity);
    }
}

void _da_free(_DummyDynamicArray* array, uint32_t itemSize) {
    if (array->items != NULL) {
        free(array->items);
        memset(array, 0, sizeof(_DummyDynamicArray));
    }
}

void _da_append(_DummyDynamicArray* array, uint32_t itemSize, void* item) {
    if (array->items == NULL) {
        assert(array->capacity == 0);
        assert(array->count == 0);

        // Lets try to allocate as much of a 4K page initially.
        // on Linux 'getconf PAGESIZE' can be used to query this value!
        uint32_t initialSize = (1 << 12) / itemSize;
        _da_reserve(array, itemSize, initialSize);
    }

    assert(array->items != NULL);
    assert(array->capacity > 0);
    assert(array->count <= array->capacity);
    if (array->count == array->capacity) {
        // NOTE: For now, lets grow by 1.5x
        uint32_t newSize = array->capacity + (array->capacity >> 1);
        _da_reserve(array, itemSize, newSize);
    }

    printf("%s: appending item (count: %u) (capacity: %u) (itemSize: %u)\n", __FUNCTION__, array->count, array->capacity, itemSize);

    void* itemPtr = (uint8_t*)array->items + (array->count * itemSize);
    memcpy(itemPtr, item, itemSize);
    ++array->count;
}

char* ca_commit(Chars* chars, const char* str) {
    if (str == NULL) {
        return NULL;
    }

    size_t len = strlen(str);
    StrView view;
    view.txt = str;
    view.len = len;

    return ca_commit_view(chars, view);
}

static char* _ca_commit_to_chunk(CharsChunk* chunk, StrView view) {
    char* dest = chunk->content + chunk->count;
    memcpy(dest, view.txt, view.len);
    dest[view.len] = '\0';
    chunk->count += view.len + 1;
    printf("%s: appending %u bytes to chunk %p. count is now %u\n", __FUNCTION__, view.len + 1, chunk, chunk->count);
    return dest;
}

char* ca_commit_view(Chars* chars, StrView view) {
    if (view.txt == NULL ||
        view.len == 0) {
        return NULL;
    }

    // MEH! We kinda have to include a null terminator, so std lib functions work with this nicely...
    uint32_t requiredSpace = view.len + 1;
    assert(requiredSpace <= CHARS_CHUNK_SIZE);

    printf("%s: trying to commit str with len %u\n", __FUNCTION__, view.len);

    for (uint32_t i = 0; i < chars->count; ++i) {
        uint32_t chunkCount = chars->items[i].count;
        uint32_t freeSpace = CHARS_CHUNK_SIZE - chunkCount;
        if (freeSpace >= requiredSpace) {
            printf("%s: found enough space in chunk with index %u\n", __FUNCTION__, i);
            return _ca_commit_to_chunk(chars->items + i, view);
        }
    }

    printf("%s: allocating new chunk\n", __FUNCTION__);
    CharsChunk chunk;
    chunk.content = (char*)malloc(CHARS_CHUNK_SIZE);
    chunk.count = 0;
    char* ret = _ca_commit_to_chunk(&chunk, view);
    da_append(chars, chunk);
    return ret;
}

void ca_dump(FILE* stream, Chars* chars) {
    fprintf(stream, "dumping Chars at %p (%d chunks)\n", chars, chars->count);
    for (uint32_t i = 0; i < chars->count; ++i) {
        fprintf(stream, "Chunk %d (count %d)\n", i, chars->items[i].count);

        char* content = chars->items[i].content;
        char* str = content;
        char* p = content + 1;
        uint32_t index = 0;
        for (uint32_t j = 0; j < chars->items[i].count; ++j) {
            if (*p == '\0') {
                if (p == str + 1 && *str == '\0') {
                    break;
                }

                fprintf(stream, "%4d: '%s'\n", index, str);
                ++index;
                str = p + 1;
                p = str;
            }

            ++p;
        }
    }
}
