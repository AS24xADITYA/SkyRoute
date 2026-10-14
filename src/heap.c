#include "heap.h"
#include <stdio.h>

MinHeap priority_heap;

void initHeap(MinHeap *h) { h->size = 0; }

static void swapItems(HeapItem *a, HeapItem *b)
{ HeapItem t = *a; *a = *b; *b = t; }

static void siftUp(MinHeap *h, int i) {
    while (i > 0) {
        int p = (i - 1) / 2;
        if (h->data[p].key <= h->data[i].key) break;
        swapItems(&h->data[p], &h->data[i]);
        i = p;
    }
}

static void siftDown(MinHeap *h, int i) {
    int n = h->size;
    for (;;) {
        int s = i, l = 2*i+1, r = 2*i+2;
        if (l < n && h->data[l].key < h->data[s].key) s = l;
        if (r < n && h->data[r].key < h->data[s].key) s = r;
        if (s == i) break;
        swapItems(&h->data[s], &h->data[i]);
        i = s;
    }
}

void heapInsert(MinHeap *h, int order_id, int priority, int timestamp) {
    if (h->size >= MAX_HEAP_SIZE) {
        fprintf(stderr, "[HEAP] Full - cannot insert order %d\n", order_id);
        return;
    }
    int key = (priority == EMERGENCY ? 0 : 100000) + timestamp;
    h->data[h->size].order_id = order_id;
    h->data[h->size].key      = key;
    h->data[h->size].priority = priority;
    h->size++;
    siftUp(h, h->size - 1);
}

HeapItem heapExtractMin(MinHeap *h) {
    HeapItem empty = {-1, -1, -1};
    if (h->size == 0) return empty;
    HeapItem min = h->data[0];
    h->data[0] = h->data[h->size - 1];
    h->size--;
    if (h->size > 0) siftDown(h, 0);
    return min;
}

int heapIsEmpty(MinHeap *h)  { return h->size == 0; }
int heapPeekId(MinHeap *h)   { return h->size == 0 ? -1 : h->data[0].order_id; }

void heapPrintJSON(MinHeap *h) {
    printf("[");
    for (int i = 0; i < h->size; i++) {
        if (i) printf(",");
        printf("{\"order_id\":%d,\"key\":%d,\"priority\":%d}",
               h->data[i].order_id, h->data[i].key, h->data[i].priority);
    }
    printf("]");
}
