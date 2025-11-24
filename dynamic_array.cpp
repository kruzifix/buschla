#include "dynamic_array.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

void _da_reserve(_DummyDynamicArray* array, uint32_t size, uint32_t itemSize) {
    if (array->capacity < size) {
        uint32_t bytes = size * itemSize;
        void* newItems = malloc(bytes);
        memset(newItems, 0, bytes);

        if (array->count > 0) {
            assert(array->items != NULL);

            memcpy(newItems, array->items, array->count * itemSize);
        }

        if (array->items != NULL) {
            free(array->items);
        }

        array->items = newItems;
        array->capacity = size;
    }
}

