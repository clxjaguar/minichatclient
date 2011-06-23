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

clist *new_clist();
clist_node *new_clist_node();

void clist_add(clist *list, clist_node *node);
/**
 * Remove the given node from the given clist (the node MUST be in the clist).
 * It then returns the removed node.
 *
 * Note that the node is NOT freed, you are now responsible for it.
 */
clist_node *clist_remove(clist *list, clist_node *node);
void clist_insert(clist *list, int index, clist_node *node);
void clist_reverse(clist *list);

void free_clist(clist *list);
void free_clist_node(clist_node *node);
/**
 * Simply free the data field if it is not NULL.
 */
void free_clist_node_data(clist_node *node);

#endif
