#ifndef HISTORY_H
#define HISTORY_H

#include "common.h"
#include "drone.h"
#include "request.h"

typedef struct {
    int order_id;
    int drone_id;
    DroneStatus prev_drone_status;
    OrderStatus prev_order_status;
} AssignmentRecord;

typedef struct {
    AssignmentRecord records[MAX_HISTORY];
    int top; /* -1 when empty */
} UndoStack;

extern UndoStack undo_stack;

/* Initialize the undo stack */
void initUndoStack();

/* Push an assignment record */
void pushAssignment(AssignmentRecord r);

/* Undo the last assignment */
int undoLast(DroneManager *dm);

#endif // HISTORY_H
