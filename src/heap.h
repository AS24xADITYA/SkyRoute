#ifndef HEAP_H
#define HEAP_H

#include "common.h"

/*
 * Binary Min-Heap Priority Queue
 * Key encoding:
 *   EMERGENCY (priority=1) at timestamp t  ->  key = 0*100000 + t
 *   NORMAL    (priority=0) at timestamp t  ->  key = 1*100000 + t
 * heapExtractMin() always yields highest-priority earliest order.
 */
typedef struct {
    int order_id;
    int key;
    int priority;   /* 0=NORMAL, 1=EMERGENCY */
} HeapItem;

typedef struct {
    HeapItem data[MAX_HEAP_SIZE];
    int size;
} MinHeap;

extern MinHeap priority_heap;

void     initHeap(MinHeap *h);
void     heapInsert(MinHeap *h, int order_id, int priority, int timestamp);
HeapItem heapExtractMin(MinHeap *h);
int      heapIsEmpty(MinHeap *h);
int      heapPeekId(MinHeap *h);       /* order_id of top, -1 if empty */
void     heapPrintJSON(MinHeap *h);    /* writes JSON array to stdout   */

#endif /* HEAP_H */
