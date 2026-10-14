#include "drone.h"
#include "drone_hash.h"
#include <stdio.h>

void initDroneManager(DroneManager *dm) {
    dm->count = 0;
}

/*
 * Re-indexes all drones into the hash table.
 * Wipes old hash entries and re-inserts every drone at its current array index.
 * Time complexity: O(n).
 */
void rehashAllDrones(DroneManager *dm) {
    freeHashTable();
    initHashTable();
    for (int i = 0; i < dm->count; i++) {
        hashInsert(dm->drones[i].drone_id, i);
    }
}

/*
 * Add a new drone to the fleet array and index it in the hash table.
 * Returns the array index on success, or -1 on duplicate/capacity error.
 */
int addDrone(DroneManager *dm, Drone d) {
    if (dm->count >= MAX_DRONES) {
        fprintf(stderr, "[DRONE] Capacity reached. Cannot add drone %d.\n", d.drone_id);
        return -1;
    }

    if (hashFind(d.drone_id) != -1 || searchDroneIndex(dm, d.drone_id) != -1) {
        fprintf(stderr, "[DRONE] Duplicate ID %d rejected.\n", d.drone_id);
        return -1;
    }

    int index = dm->count;
    dm->drones[index] = d;
    dm->count++;

    hashInsert(d.drone_id, index);
    return index;
}

/*
 * Decommission a drone from the fleet.
 * Only drones in IDLE state can be safely decommissioned.
 * Shifts remaining elements left and calls rehashAllDrones to keep indices synchronized.
 * Returns 1 on success, 0 on failure.
 */
int removeDrone(DroneManager *dm, int id) {
    int index = hashFind(id);
    if (index == -1) index = searchDroneIndex(dm, id);
    if (index == -1) {
        fprintf(stderr, "[DRONE] ID %d not found for decommission.\n", id);
        return 0;
    }

    if (dm->drones[index].status != IDLE) {
        fprintf(stderr, "[DRONE] Drone %d is not IDLE (status=%d), cannot decommission.\n",
                id, dm->drones[index].status);
        return 0;
    }

    /* Shift remaining drones to fill the gap */
    for (int i = index; i < dm->count - 1; i++) {
        dm->drones[i] = dm->drones[i + 1];
    }
    dm->count--;

    /* Keep hash table 100% in sync with new array indices */
    rehashAllDrones(dm);
    return 1;
}

/* Linear search through the drone array (fallback if hash table is bypassed) */
int searchDroneIndex(DroneManager *dm, int id) {
    for (int i = 0; i < dm->count; i++) {
        if (dm->drones[i].drone_id == id) {
            return i;
        }
    }
    return -1;
}

/* Clamped battery charge/discharge helper */
void updateBattery(DroneManager *dm, int id, int delta) {
    int index = hashFind(id);
    if (index == -1) index = searchDroneIndex(dm, id);
    if (index == -1) return;

    int new_battery = dm->drones[index].battery_pct + delta;
    if (new_battery > 100) new_battery = 100;
    if (new_battery < 0) new_battery = 0;

    dm->drones[index].battery_pct = new_battery;
}

