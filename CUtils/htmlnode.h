#ifdef __cplusplus
extern "C" {
#endif

#ifndef HTMLNODE_H
#define HTMLNODE_H

#include <stdio.h>
#include "attribute.h"
#include "clist.h"

typedef struct htmlnode_struct htmlnode;
struct htmlnode_struct {
	char *name;
	clist *attributes;
	clist *nodes;
	htmlnode *parent;
};

htmlnode *htmlnode_new();

void htmlnode_free(htmlnode *self);

int htmlnode_parse(htmlnode *self, FILE *file);

#endif

#ifdef __cplusplus
}
#endif
