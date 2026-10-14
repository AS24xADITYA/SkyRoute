#ifndef STATION_H
#define STATION_H

#include "common.h"

typedef struct {
    int station_id;
    int node_id; // Added node_id to link with graph
    int total_slots;
    int free_slots;
    int waiting_queue[MAX_WAITING]; /* circular array queue of drone_id */
    int wq_front, wq_rear, wq_count;
} ChargingStation;

extern ChargingStation stations[MAX_STATIONS];
extern int stationCount;

/* Initialize station module */
void initStations();

/* Add a new station */
int addStation(int station_id, int node_id, int slots);

/* Assign drone to station (occupy slot or enqueue) */
void assignDrone(ChargingStation *cs, int drone_id);

/* Free a slot and pull next drone from queue if any */
int freeSlot(ChargingStation *cs); // returns drone_id that was pulled, or -1

/* View waiting queue */
void viewWaitingQueue(ChargingStation *cs);

/* Find station by ID */
ChargingStation* findStation(int station_id);

#endif // STATION_H
