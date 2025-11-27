#pragma once

#include <stdint.h>

#define DEFINE_DYNAMIC_ARRAY(name, type) typedef struct { \
    type* items; \
    uint32_t count; \
    uint32_t capacity; \
} name;

#define da_item_size(arrayPtr) sizeof(*((arrayPtr)->items))

#define DA_EXPAND_ARGS(arrayPtr) (_DummyDynamicArray*)(arrayPtr), da_item_size(arrayPtr)

#define da_reserve(arrayPtr, requestedSize) _da_reserve(DA_EXPAND_ARGS(arrayPtr), requestedSize)
// Sets count = 0, does not de-allocate.
#define da_reset(arrayPtr) _da_reset(DA_EXPAND_ARGS(arrayPtr))
#define da_free(arrayPtr) _da_free(DA_EXPAND_ARGS(arrayPtr))

// Expects an lvalue, which it then takes a pointer to.
#define da_append(arrayPtr, item) da_append_ptr(arrayPtr, &item)
// Expects a pointer to an item.
#define da_append_ptr(arrayPtr, itemPtr) _da_append(DA_EXPAND_ARGS(arrayPtr), (void*)(itemPtr))
#if __cplusplus
// Adds an item and returns a pointer to it.
#define da_append_get(arrayPtr) (decltype((arrayPtr)->items))_da_append_get(DA_EXPAND_ARGS(arrayPtr))
#else
// Adds an item and returns a pointer to it.
#define da_append_get(arrayPtr) _da_append_get(DA_EXPAND_ARGS(arrayPtr))
#endif

DEFINE_DYNAMIC_ARRAY(_DummyDynamicArray, void)

void _da_reserve(_DummyDynamicArray* array, uint32_t itemSize, uint32_t requestedSize);
void _da_reset(_DummyDynamicArray* array, uint32_t itemSize);
void _da_free(_DummyDynamicArray* array, uint32_t itemSize);
void _da_append(_DummyDynamicArray* array, uint32_t itemSize, void* item);
void* _da_append_get(_DummyDynamicArray* array, uint32_t itemSize);


// --------------------------------- CHUNK ARRAY --------------------------------- //
// On the top level we have a dynamic array of pointers to memory blocks.
// Each block is the same power-of-2 size
// When indexing into the chunk array the requested index can then be split into a global and local index
// by bit shifting & and-ing.

// NOTE: For now lets just make a string oriented chunk array! we can always generalize if needed.
// TODO: This can maybe be even bigger?
// TODO: Or we say that the first chunk in a chunk array is 4096 and for smaller strings,
// TODO: And any chunk allocated later on are bigger?
#define CHARS_CHUNK_SIZE 4096

typedef struct {
    const char* txt;
    uint32_t len;
} StrView;

typedef struct {
    char* content;
    // "Points" to next free byte (i.e. stores how much space is occupied)
    uint32_t count;
    uint32_t strCount;
} CharsChunk;

DEFINE_DYNAMIC_ARRAY(Chars, CharsChunk)

// Copies given null-terminated string into the chunk array
// The returned pointer will stay valid, it is never moved
// DO NOT free this pointer
char* ca_commit(Chars* chars, const char* str);

// Copies given string view into the chunk array
// The returned pointer will stay valid, it is never moved
// DO NOT free this pointer
char* ca_commit_view(Chars* chars, StrView view);

// Uses vsnprintf to directly format a string into the chunk array
// The returned pointer will stay valid, it is never moved
// DO NOT free this pointer
char* ca_commitf(Chars* chars, const char* fmt, ...);

// Resets the count of each chunk, does not de-allocate.
void ca_reset(Chars* chars);

void ca_dump(FILE* stream, Chars* chars);
