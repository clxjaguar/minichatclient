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
void clist_remove(clist *list, clist_node *node);
void clist_insert(clist *list, int index, clist_node *node);
void clist_reverse(clist *list);
void free_clist(clist *list);
void free_clist_node(clist_node *node);

#endif
