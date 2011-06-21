/*
  Name:        clist_p.h
  Copyright:   niki (cc-by-nc) 2011
  Author:      niki
  Date:        2011-06-21
  Description: The 'private' header for clist.c, which you don't need to use clist.c
*/


#ifndef CLIST_P_H
#define CLIST_P_H

typedef struct clist_node_struct clist_node;
struct clist_node_struct {
	clist_node *prev;
	clist_node *next;
	void *data;
};

typedef struct clist_private_struct clist_private;
struct clist_private_struct {
	clist_node *first;
	clist_node *last;
};

#endif
