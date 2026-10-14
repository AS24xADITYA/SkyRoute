#include "history.h"
#include <stdio.h>

UndoStack undo_stack;

void initUndoStack() {
    undo_stack.top = -1;
}

void pushAssignment(AssignmentRecord r) {
    if (undo_stack.top >= MAX_HISTORY - 1) {
        return;
    }
    undo_stack.top++;
    undo_stack.records[undo_stack.top] = r;
}

int undoLast(DroneManager *dm) {
    if (undo_stack.top == -1) {
        return 0;
    }
    
    AssignmentRecord r = undo_stack.records[undo_stack.top];
    undo_stack.top--;
    
    // Revert Order
    DeliveryRequest *order = getOrder(&active_orders, r.order_id);
    if (order) {
        order->status = r.prev_order_status;
    }
    
    // Revert Drone
    int drone_idx = searchDroneIndex(dm, r.drone_id);
    if (drone_idx != -1) {
        dm->drones[drone_idx].status = r.prev_drone_status;
    }
    
    return 1;
}
