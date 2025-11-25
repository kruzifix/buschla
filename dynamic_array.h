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
#define da_free(arrayPtr) _da_free(DA_EXPAND_ARGS(arrayPtr))
// Expects an lvalue, which it then takes a pointer to.
#define da_append(arrayPtr, item) da_append_ptr(arrayPtr, &item)
// Expects a pointer to an item.
#define da_append_ptr(arrayPtr, itemPtr) _da_append(DA_EXPAND_ARGS(arrayPtr), (void*)(itemPtr))

DEFINE_DYNAMIC_ARRAY(_DummyDynamicArray, void)

// TODO: we kinda always need the itemSize, should we just bake it into the struct?
void _da_reserve(_DummyDynamicArray* array, uint32_t itemSize, uint32_t requestedSize);
void _da_free(_DummyDynamicArray* array, uint32_t itemSize);
void _da_append(_DummyDynamicArray* array, uint32_t itemSize, void* item);


// --------------------------------- CHUNK ARRAY --------------------------------- //
// On the top level we have a dynamic array of pointers to memory blocks.
// Each block is the same power-of-2 size
// When indexing into the chunk array the requested index can then be split into a global and local index
// by bit shifting & and-ing.

// NOTE: For now lets just make a string oriented chunk array! we can always generalize if needed.
#define CHARS_CHUNK_SIZE 4096

typedef struct {
    char* content;
    // "Points" to next free byte (i.e. stores how much space is occupied)
    uint32_t count;
} CharsChunk;

DEFINE_DYNAMIC_ARRAY(Chars, CharsChunk)

char* ca_commit(Chars* chars, const char* str);
char* ca_commit_view(Chars* chars, StrView view);

void ca_dump(FILE* stream, Chars* chars);
