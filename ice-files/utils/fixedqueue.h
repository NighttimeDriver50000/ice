#ifndef FIXEDQUEUE_H
#define FIXEDQUEUE_H

#include <stdint.h>
#include <stddef.h>

typedef struct FixedQueueStruct {
    int16_t *array;
    size_t size;
    size_t head;
} * FixedQueue;

FixedQueue newFixedQueue(size_t size);

void freeFixedQueue(FixedQueue q);

int16_t pushPop(FixedQueue q, int16_t value);

#endif
