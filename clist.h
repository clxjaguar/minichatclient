#ifndef CLIST_H
#define CLIST_H

#include "clist_p.h"

typedef struct clist_struct clist;
struct clist_struct {
	clist_private *private;
	size_t size;
};

clist *new_clist();
void clist_add(void *data);
void *clist_get(int index);
void clist_set(void *data);
void *clist_del(int index);
void clist_insert(int index, void *data);

#endif
