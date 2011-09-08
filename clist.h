#ifndef CLIST_H
#define CLIST_H

typedef struct clist_node_struct clist_node;
struct clist_node_struct {
	clist_node *prev;
	clist_node *next;
	void *data;
	void(*free_node)(clist_node *self);
};

typedef struct clist_struct clist;
struct clist_struct {
	clist_node *first;
	clist_node *last;
	size_t size;
};

/**
 * Create a new clist.
 *
 * @return a new clist
 */
clist *new_clist();

/**
 * Create a new node.
 *
 * @return a new node
 */
clist_node *new_clist_node();

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

void free_clist(clist *list);
void free_clist_node(clist_node *node);
/**
 * Simply free() the data field if it is not NULL.
 */
void free_clist_node_data(clist_node *node);

#endif
