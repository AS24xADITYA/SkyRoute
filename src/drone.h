#ifndef DRONE_H
#define DRONE_H

#include "common.h"

/*
 * Drone record in fleet.
 * Tracks battery, location node, operating status, and capacity.
 */
typedef struct {
    int drone_id;
    int battery_pct;        /* 0 - 100% */
    int location_id;        /* Node ID on the air corridor graph */
    DroneStatus status;     /* IDLE, DELIVERING, CHARGING, OFFLINE */
    float max_carry_weight; /* Maximum package weight in kg */
} Drone;

/*
 * DroneManager: Sequential array storing fleet drones.
 * Paired with hash table for O(1) ID lookups.
 */
typedef struct DroneManager {
    Drone drones[MAX_DRONES];
    int count;
} DroneManager;

/* Initialize Drone Manager */
void initDroneManager(DroneManager *dm);

/* Add a new drone. Returns array index on success, -1 on failure. */
int addDrone(DroneManager *dm, Drone d);

/* Decommission an IDLE drone by ID. Shifts array and re-indexes hash table. */
int removeDrone(DroneManager *dm, int id);

/* Linear search by drone ID. Returns array index or -1 if not found. */
int searchDroneIndex(DroneManager *dm, int id);

/* Battery management helper. Clamps to [0, 100]. */
void updateBattery(DroneManager *dm, int id, int delta);

/* Rebuild the hash table from the current drone array order. */
void rehashAllDrones(DroneManager *dm);

#endif // DRONE_H

