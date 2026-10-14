#include "station.h"
#include <stdio.h>

/*
 * ============================================================================
 * AUTOMATED CHARGING STATIONS — Turnaround Queue Model
 *
 * Each charging station operates a fixed number of physical charging bays/pads.
 * When all slots are occupied, arriving drones enter a FIFO waiting queue.
 * When a charging drone is released (freeSlot), the next drone at the front
 * of the queue is immediately assigned the vacated pad in O(1) time.
 * ============================================================================
 */

ChargingStation stations[MAX_STATIONS];
int stationCount = 0;

void initStations(void) {
    stationCount = 0;
}

int addStation(int station_id, int node_id, int slots) {
    if (stationCount >= MAX_STATIONS) {
        return -1;
    }

    /* Check duplicate ID */
    for (int i = 0; i < stationCount; i++) {
        if (stations[i].station_id == station_id) {
            return -1;
        }
    }

    int idx = stationCount++;
    stations[idx].station_id = station_id;
    stations[idx].node_id = node_id;
    stations[idx].total_slots = slots;
    stations[idx].free_slots = slots;
    stations[idx].wq_front = 0;
    stations[idx].wq_rear = 0;
    stations[idx].wq_count = 0;

    return idx;
}

ChargingStation* findStation(int station_id) {
    for (int i = 0; i < stationCount; i++) {
        if (stations[i].station_id == station_id) {
            return &stations[i];
        }
    }
    return NULL;
}

void assignDrone(ChargingStation *cs, int drone_id) {
    if (cs->free_slots > 0) {
        cs->free_slots--;
    } else {
        if (cs->wq_count == MAX_WAITING) {
            fprintf(stderr, "[STATION] Station %d waiting queue is full.\n", cs->station_id);
            return;
        }
        cs->waiting_queue[cs->wq_rear] = drone_id;
        cs->wq_rear = (cs->wq_rear + 1) % MAX_WAITING;
        cs->wq_count++;
    }
}

int freeSlot(ChargingStation *cs) {
    if (cs->wq_count > 0) {
        /* Dequeue next drone from waiting queue into the freed bay */
        int next_drone = cs->waiting_queue[cs->wq_front];
        cs->wq_front = (cs->wq_front + 1) % MAX_WAITING;
        cs->wq_count--;
        return next_drone;
    } else {
        if (cs->free_slots < cs->total_slots) {
            cs->free_slots++;
        }
        return -1;
    }
}

void viewWaitingQueue(ChargingStation *cs) {
    fprintf(stderr, "[STATION] Station %d waiting queue: ", cs->station_id);
    if (cs->wq_count == 0) {
        fprintf(stderr, "Empty\n");
        return;
    }
    int count = 0;
    int i = cs->wq_front;
    while (count < cs->wq_count) {
        fprintf(stderr, "%d ", cs->waiting_queue[i]);
        i = (i + 1) % MAX_WAITING;
        count++;
    }
    fprintf(stderr, "\n");
}

