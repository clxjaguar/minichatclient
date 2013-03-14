#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLIST_H
#define CLIST_H

#include <stdlib.h>

/**
 * Represent a node in the list.
 */
typedef struct clist_node_struct clist_node;
struct clist_node_struct {
	clist_node *prev; /**< The previous node. */
	clist_node *next; /**< The next node. */
	void *data; /**< The data present in the node. */
	void *free_data; /**< An OBSOLETE method to free a node: (void)(clist_node *node). */
	void *free_content; /**< A method to free a node: (void)(T *object). */
};

typedef struct clist_struct clist;
struct clist_struct {
	clist_node *first;
	clist_node *last;
	size_t size;
};

// Old
/** Obsolete. */
clist *new_clist();
/** Obsolete. */
clist_node *new_clist_node();
/**
 * OBSOLETE:
 * Simply free() the data field if it is not NULL.
 */
void free_clist_node_data(clist_node *node);
/** Obsolete. */
void free_clist(clist *list);
/** Obsolete. */
void free_clist_node(clist_node *node);
//

/**
 * Create a new clist.
 *
 * @return a new clist
 */
clist *clist_new();

/**
 * Create a new node.
 *
 * @return a new node
 */
clist_node *clist_node_new();

/**
 * Add a node to the end of the given clist.
 *
 * @param list the clist to add to
 * @param node the node to add
 */
void clist_add(clist *list, clist_node *node);

/**
 * Insert a node at the start of the given clist.
 *
 * @param list the clist to add to
 * @param node the node to add
 */
void clist_insert(clist *list, clist_node *node);

/**
 * Remove the given node from the given clist (the node MUST be in the clist).
 * It then returns the removed node.
 *
 * Note that the node is NOT freed, you are now responsible for it.
 */
clist_node *clist_remove(clist *list, clist_node *node);

/**
 * Get the node at the given position in the list.
 *
 * @param list the list
 * @param index the position
 *
 * @return the node
 */
clist_node *clist_get(clist *list, size_t index);

/**
 * Insert the node at the given position in the list.
 *
 * @return true if it was inserted (it will not be inserted if the node with
 * 	the index before the given one does not exist in the list)
 */
int clist_insert_at(clist *list, size_t index, clist_node *node);

/**
 * Insert a node before another one in the list.
 *
 * @param list the list to work on
 * @param anchor the node that will follow the node to insert, or NULL for head
 * @param node the node to insert
 */
void clist_insert_before(clist *list, clist_node *anchor, clist_node *node);

/**
 * Insert a node after another one in the list.
 *
 * @param list the list to work on
 * @param anchor the node that will precede the node to insert, or NULL for tail
 * @param node the node to insert
 */
void clist_insert_after(clist *list, clist_node *anchor, clist_node *node);

/**
 * Reverse the clist.
 *
 * @param list the clist to reverse
 */
void clist_reverse(clist *list);

/**
 * Free the given clist, including its nodes (recursive).
 */
void clist_free(clist *list);

/**
 * Free the given clist_node, including its data according to clist_node->free_node.
 */
void clist_node_free(clist_node *node);

#endif

#ifdef __cplusplus
}
#endif
