#include "dynamic_array.h"

#include "util.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define DISABLE_DEBUG_PRINTING

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
