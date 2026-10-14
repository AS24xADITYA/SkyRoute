#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* Core fleet / graph limits */
#define MAX_DRONES           50
#define MAX_STATIONS         10
#define MAX_NODES            60
#define MAX_ZONES            50
#define MAX_QUEUE            200
#define TABLE_SIZE           101
#define MAX_WAITING          50
#define MAX_HISTORY          100
#define MIN_FLIGHT_THRESHOLD 20

/* New module limits */
#define MAX_HEAP_SIZE        200
#define TRIE_ALPHA           26
#define TRIE_MAX_WORD        52
#define MAX_TRIE_RESULTS     30
#define MAX_CUSTOMERS        100
#define MAX_PATH_LEN         MAX_NODES

/* Enumerations */
typedef enum { WAREHOUSE = 0, STATION = 1, CUSTOMER = 2 }              NodeKind;
typedef enum { NORMAL = 0, EMERGENCY = 1 }                             Priority;
typedef enum { IDLE = 0, DELIVERING = 1, CHARGING = 2, OFFLINE = 3 }  DroneStatus;
typedef enum { PENDING = 0, ASSIGNED = 1, IN_TRANSIT = 2,
               DELIVERED = 3, CANCELLED = 4 }                          OrderStatus;

#endif /* COMMON_H */
