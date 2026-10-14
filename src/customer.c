#include "customer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * ============================================================================
 * CUSTOMER DIRECTORY — Binary Search Tree (BST)
 * Key: customer_id (integer)
 * Properties:
 * - Left subtree contains customer_id < root->customer_id
 * - Right subtree contains customer_id > root->customer_id
 * - Average Search/Insert/Delete: O(log n)
 * - In-order traversal visits customers in strictly ascending order by ID.
 * ============================================================================
 */

CustomerNode *customer_root = NULL;

void initCustomerDirectory(void) {
    customer_root = NULL;
}

CustomerNode* insertCustomer(CustomerNode *root, int id, const char *name, int node_id) {
    if (root == NULL) {
        CustomerNode *newNode = (CustomerNode *)malloc(sizeof(CustomerNode));
        if (!newNode) {
            fprintf(stderr, "[BST] Memory allocation failed for customer %d.\n", id);
            return NULL;
        }
        newNode->customer_id = id;
        strncpy(newNode->name, name, 49);
        newNode->name[49] = '\0';
        newNode->node_id = node_id;
        newNode->left = newNode->right = NULL;
        return newNode;
    }

    if (id < root->customer_id) {
        root->left = insertCustomer(root->left, id, name, node_id);
    } else if (id > root->customer_id) {
        root->right = insertCustomer(root->right, id, name, node_id);
    } else {
        fprintf(stderr, "[BST] Duplicate customer ID %d ignored.\n", id);
    }
    return root;
}

CustomerNode* searchCustomer(CustomerNode *root, int id) {
    if (root == NULL || root->customer_id == id) {
        return root;
    }
    if (id < root->customer_id) {
        return searchCustomer(root->left, id);
    }
    return searchCustomer(root->right, id);
}

CustomerNode* findMin(CustomerNode *node) {
    CustomerNode *current = node;
    while (current && current->left != NULL)
        current = current->left;
    return current;
}

CustomerNode* removeCustomer(CustomerNode *root, int id) {
    if (root == NULL) return root;

    if (id < root->customer_id) {
        root->left = removeCustomer(root->left, id);
    } else if (id > root->customer_id) {
        root->right = removeCustomer(root->right, id);
    } else {
        /* Node with one child or no child */
        if (root->left == NULL) {
            CustomerNode *temp = root->right;
            free(root);
            return temp;
        } else if (root->right == NULL) {
            CustomerNode *temp = root->left;
            free(root);
            return temp;
        }

        /* Node with two children: in-order successor */
        CustomerNode *temp = findMin(root->right);
        root->customer_id = temp->customer_id;
        strcpy(root->name, temp->name);
        root->node_id = temp->node_id;
        root->right = removeCustomer(root->right, temp->customer_id);
    }
    return root;
}

void inorderPrint(CustomerNode *root) {
    if (root != NULL) {
        inorderPrint(root->left);
        fprintf(stderr, "ID: %d | Name: %s | Node ID: %d\n", root->customer_id, root->name, root->node_id);
        inorderPrint(root->right);
    }
}

/* In-order traversal helper to collect customers into sorted arrays */
static void collectInOrder(CustomerNode *root, int ids[], char names[][50], int node_ids[], int max_c, int *count) {
    if (!root || *count >= max_c) return;
    collectInOrder(root->left, ids, names, node_ids, max_c, count);
    if (*count < max_c) {
        ids[*count] = root->customer_id;
        strncpy(names[*count], root->name, 49);
        names[*count][49] = '\0';
        node_ids[*count] = root->node_id;
        (*count)++;
    }
    collectInOrder(root->right, ids, names, node_ids, max_c, count);
}

int collectCustomers(CustomerNode *root, int ids[], char names[][50], int node_ids[], int max_c) {
    int count = 0;
    collectInOrder(root, ids, names, node_ids, max_c, &count);
    return count;
}

void freeCustomerBST(CustomerNode *root) {
    if (root != NULL) {
        freeCustomerBST(root->left);
        freeCustomerBST(root->right);
        free(root);
    }
}

