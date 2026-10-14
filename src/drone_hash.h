#ifndef DRONE_HASH_H
#define DRONE_HASH_H

#include "common.h"

/* Forward declaration */
typedef struct DroneManager DroneManager;

/*
 * HashNode: Chaining bucket entry for O(1) drone lookups by ID.
 * Maps drone_id -> index in DroneManager array.
 */
typedef struct HashNode {
    int drone_id;
    int array_index;       /* Index in dm.drones[] */
    struct HashNode *next; /* Collision chaining pointer */
} HashNode;

extern HashNode *drone_table[TABLE_SIZE];

/* Hash table management */
void initHashTable(void);
void hashInsert(int drone_id, int array_index);
int  hashFind(int drone_id);
void hashRemove(int drone_id);
void freeHashTable(void);

#endif // DRONE_HASH_H

