#ifndef REQUEST_H
#define REQUEST_H

#include "common.h"
#include "heap.h"

typedef struct {
    int order_id;
    int customer_node_id;
    float package_weight;
    Priority priority;
    OrderStatus status;
} DeliveryRequest;

/* Active Delivery Requests Linked List */
typedef struct OrderNode {
    DeliveryRequest data;
    struct OrderNode *next;
} OrderNode;

typedef struct {
    OrderNode *head;
} OrderList;

/* Circular Array Queue for scheduling */
typedef struct {
    int items[MAX_QUEUE];
    int front, rear, count;
} IntQueue;

extern OrderList active_orders;
extern IntQueue normalQueue;
extern IntQueue emergencyQueue;

/* Queue Operations */
void initQueue(IntQueue *q);
void enqueue(IntQueue *q, int order_id);
int dequeue(IntQueue *q);
int isQueueEmpty(IntQueue *q);

/* Linked List Operations */
void initOrderList(OrderList *list);
void insertOrder(OrderList *list, DeliveryRequest r);
int removeOrder(OrderList *list, int order_id);
DeliveryRequest* getOrder(OrderList *list, int order_id);
void traverseOrders(OrderList *list);
void freeOrderList(OrderList *list);

/* High-level Request Operations */
void initRequests();
void addRequest(DeliveryRequest r);
int cancelRequest(int order_id);
void completeRequest(int order_id);
int getNextOrder();
int getNextOrderHeap();

#endif // REQUEST_H
