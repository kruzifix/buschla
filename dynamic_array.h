#pragma once

#include <stdint.h>

#define DEFINE_DYNAMIC_ARRAY(name, type) typedef struct { \
    type* items; \
    uint32_t count; \
    uint32_t capacity; \
} name;

#define da_item_size(arrayPtr) sizeof(*((arrayPtr)->items))

#define da_reserve(arrayPtr, size) _da_reserve((_DummyDynamicArray*)arrayPtr, size, da_item_size(arrayPtr))

#define da_append(array, item)

DEFINE_DYNAMIC_ARRAY(_DummyDynamicArray, void)

void _da_reserve(_DummyDynamicArray* array, uint32_t size, uint32_t itemSize);

