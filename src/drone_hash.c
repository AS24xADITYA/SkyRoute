#include "drone_hash.h"
#include <stdio.h>
#include <stdlib.h>

/*
 * Hash Table with Separate Chaining for O(1) Drone Index Lookups.
 * Table size is 101 (a prime number to minimize collisions).
 */
HashNode *drone_table[TABLE_SIZE];

/* Modulo division hash function */
static int hashFn(int drone_id) {
    int h = drone_id % TABLE_SIZE;
    if (h < 0) h += TABLE_SIZE;
    return h;
}

void initHashTable(void) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        drone_table[i] = NULL;
    }
}

/* Insert or update mapping (drone_id -> array_index) */
void hashInsert(int drone_id, int array_index) {
    int h = hashFn(drone_id);
    HashNode *curr = drone_table[h];

    /* If key already exists in this bucket, update its index (prevents duplicates) */
    while (curr) {
        if (curr->drone_id == drone_id) {
            curr->array_index = array_index;
            return;
        }
        curr = curr->next;
    }

    /* Create new node and insert at head of bucket chain */
    HashNode *node = (HashNode *)malloc(sizeof(HashNode));
    if (!node) {
        fprintf(stderr, "[HASH] Memory allocation failed in hashInsert for drone %d.\n", drone_id);
        return;
    }
    node->drone_id = drone_id;
    node->array_index = array_index;
    node->next = drone_table[h];
    drone_table[h] = node;
}

/* Look up array index for a given drone ID. Returns -1 if not found. */
int hashFind(int drone_id) {
    int h = hashFn(drone_id);
    HashNode *curr = drone_table[h];
    while (curr) {
        if (curr->drone_id == drone_id) {
            return curr->array_index;
        }
        curr = curr->next;
    }
    return -1;
}

/* Remove a drone entry from the hash table */
void hashRemove(int drone_id) {
    int h = hashFn(drone_id);
    HashNode *curr = drone_table[h];
    HashNode *prev = NULL;

    while (curr) {
        if (curr->drone_id == drone_id) {
            if (prev) {
                prev->next = curr->next;
            } else {
                drone_table[h] = curr->next;
            }
            free(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}

/* Free all allocated chain nodes */
void freeHashTable(void) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        HashNode *curr = drone_table[i];
        while (curr) {
            HashNode *temp = curr;
            curr = curr->next;
            free(temp);
        }
        drone_table[i] = NULL;
    }
}

