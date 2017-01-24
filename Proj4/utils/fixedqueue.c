#include "fixedqueue.h"
#include <stdlib.h>

FixedQueue newFixedQueue(size_t size) {
    FixedQueue q = malloc(sizeof(*q));
    if (q == 0) {
        return 0;
    }
    q->array = calloc(size, sizeof(*q->array));
    if (q->array == 0) {
        free(q);
        return 0;
    }
    q->size = size;
    q->head = 0;
    return q;
}

void freeFixedQueue(FixedQueue q) {
    free(q->array);
    free(q);
}

int16_t pushPop(FixedQueue q, int16_t value) {
    int16_t r = q->array[q->head];
    q->array[q->head++] = value;
    q->head %= q->size;
    return r;
}
