#include "request.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * ============================================================================
 * DISPATCH QUEUES & REQUEST LIST
 *
 * 1. Dual FIFO Circular Queues (emergencyQueue & normalQueue):
 *    - O(1) enqueue and dequeue.
 *    - Strict priority partitioning: emergency orders served before normal orders.
 *
 * 2. Singly Linked List (active_orders):
 *    - Dynamic collection of all placed orders.
 *    - O(1) insert at head, O(n) search and deletion.
 *
 * 3. Binary Min-Heap (priority_heap):
 *    - O(log n) composite priority queue using urgency key + timestamp.
 * ============================================================================
 */

OrderList active_orders;
IntQueue normalQueue;
IntQueue emergencyQueue;
static int g_order_timestamp = 0;

void initQueue(IntQueue *q) {
    q->front = 0;
    q->rear = 0;
    q->count = 0;
}

void enqueue(IntQueue *q, int order_id) {
    if (q->count == MAX_QUEUE) {
        fprintf(stderr, "[QUEUE] Overflow: cannot enqueue order %d.\n", order_id);
        return;
    }
    q->items[q->rear] = order_id;
    q->rear = (q->rear + 1) % MAX_QUEUE;
    q->count++;
}

int dequeue(IntQueue *q) {
    if (q->count == 0) return -1;
    int item = q->items[q->front];
    q->front = (q->front + 1) % MAX_QUEUE;
    q->count--;
    return item;
}

int isQueueEmpty(IntQueue *q) {
    return q->count == 0;
}

void initOrderList(OrderList *list) {
    list->head = NULL;
}

void insertOrder(OrderList *list, DeliveryRequest r) {
    OrderNode *newNode = (OrderNode *)malloc(sizeof(OrderNode));
    if (!newNode) {
        fprintf(stderr, "[ORDER] Memory allocation failed for order %d.\n", r.order_id);
        return;
    }
    newNode->data = r;
    newNode->next = list->head;
    list->head = newNode;
}

int removeOrder(OrderList *list, int order_id) {
    OrderNode *curr = list->head;
    OrderNode *prev = NULL;

    while (curr) {
        if (curr->data.order_id == order_id) {
            if (prev) prev->next = curr->next;
            else list->head = curr->next;
            free(curr);
            return 1;
        }
        prev = curr;
        curr = curr->next;
    }
    return 0; /* Not found */
}

DeliveryRequest* getOrder(OrderList *list, int order_id) {
    OrderNode *curr = list->head;
    while (curr) {
        if (curr->data.order_id == order_id) {
            return &(curr->data);
        }
        curr = curr->next;
    }
    return NULL;
}

void traverseOrders(OrderList *list) {
    OrderNode *curr = list->head;
    while (curr) {
        fprintf(stderr, "[ORDER] ID: %d, Node: %d, Status: %d\n",
                curr->data.order_id, curr->data.customer_node_id, curr->data.status);
        curr = curr->next;
    }
}

void freeOrderList(OrderList *list) {
    OrderNode *curr = list->head;
    while (curr) {
        OrderNode *temp = curr;
        curr = curr->next;
        free(temp);
    }
    list->head = NULL;
}

void initRequests(void) {
    initQueue(&normalQueue);
    initQueue(&emergencyQueue);
    initOrderList(&active_orders);
    initHeap(&priority_heap);
    g_order_timestamp = 0;
}

void addRequest(DeliveryRequest r) {
    r.status = PENDING;
    insertOrder(&active_orders, r);
    g_order_timestamp++;

    /* Enqueue into dual FIFO queues */
    if (r.priority == EMERGENCY) {
        enqueue(&emergencyQueue, r.order_id);
    } else {
        enqueue(&normalQueue, r.order_id);
    }

    /* Enqueue into Binary Min-Heap */
    heapInsert(&priority_heap, r.order_id, (int)r.priority, g_order_timestamp);
}

int cancelRequest(int order_id) {
    DeliveryRequest *r = getOrder(&active_orders, order_id);
    if (!r) {
        fprintf(stderr, "[ORDER] ID %d not found for cancellation.\n", order_id);
        return 0;
    }
    if (r->status != PENDING) {
        fprintf(stderr, "[ORDER] Cannot cancel order %d: status is not PENDING.\n", order_id);
        return 0;
    }

    r->status = CANCELLED;
    return 1;
}

void completeRequest(int order_id) {
    DeliveryRequest *r = getOrder(&active_orders, order_id);
    if (r) {
        r->status = DELIVERED;
    }
}

/* Returns next order ID using dual FIFO queues (Emergency first, then Standard) */
int getNextOrder(void) {
    while (!isQueueEmpty(&emergencyQueue)) {
        int oid = dequeue(&emergencyQueue);
        DeliveryRequest *r = getOrder(&active_orders, oid);
        if (r && r->status == PENDING) return oid;
    }
    while (!isQueueEmpty(&normalQueue)) {
        int oid = dequeue(&normalQueue);
        DeliveryRequest *r = getOrder(&active_orders, oid);
        if (r && r->status == PENDING) return oid;
    }
    return -1;
}

/* Alternative: Returns next order ID extracting from Binary Min-Heap in O(log n) */
int getNextOrderHeap(void) {
    while (!heapIsEmpty(&priority_heap)) {
        HeapItem it = heapExtractMin(&priority_heap);
        DeliveryRequest *r = getOrder(&active_orders, it.order_id);
        if (r && r->status == PENDING) return it.order_id;
    }
    return -1;
}

