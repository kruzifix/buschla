#pragma once

#include <stdint.h>


// TODO: What about a ChunkArray ?
// On the top level we have a dynamic array of pointers to memory blocks
// each block is the same power-of-2 size
// the requested index can then be split into a global and local index by bit shifting & and-ing


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

