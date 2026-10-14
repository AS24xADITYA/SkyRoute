#ifndef CUSTOMER_H
#define CUSTOMER_H

#include "common.h"

typedef struct CustomerNode {
    int customer_id;
    char name[50];
    int node_id;                 /* location in the Graph */
    struct CustomerNode *left, *right;
} CustomerNode;

extern CustomerNode *customer_root;

/* Initialize BST */
void initCustomerDirectory();

/* Insert customer */
CustomerNode* insertCustomer(CustomerNode *root, int id, const char *name, int node_id);

/* Search customer */
CustomerNode* searchCustomer(CustomerNode *root, int id);

/* Remove customer */
CustomerNode* removeCustomer(CustomerNode *root, int id);

/* Print in-order */
void inorderPrint(CustomerNode *root);

/* Collect all customers into linear arrays (in-order BST traversal) */
int collectCustomers(CustomerNode *root, int ids[], char names[][50], int node_ids[], int max_c);

/* Free all nodes */
void freeCustomerBST(CustomerNode *root);

#endif // CUSTOMER_H
