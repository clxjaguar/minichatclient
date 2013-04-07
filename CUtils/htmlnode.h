#ifdef __cplusplus
extern "C" {
#endif

#ifndef HTMLNODE_H
#define HTMLNODE_H

#include <stdio.h>
#include "attribute.h"
#include "clist.h"

enum HTMLNODE_ELEM_TYPE {
	HTMLNODE_ELEM_TEXT,
	HTMLNODE_ELEM_TAG,
	HTMLNODE_ELEM_ATTRIBUTE,
};

typedef struct htmlnode_struct htmlnode;
struct htmlnode_struct {
	htmlnode *parent;
	char *name; // NULL means text

	char *text; // [ either

	clist *attributes; // [ or
	clist *nodes;      // [
};

typedef struct {
	enum HTMLNODE_ELEM_TYPE type;
	//TODO: union
	char *text;

	char *name;

	char *tag;
	char *key;
	char *value;
} htmlnode_elem;

htmlnode *htmlnode_new();

void htmlnode_free(htmlnode *self);

htmlnode_elem *htmlnode_elem_new();

void htmlnode_elem_free(htmlnode_elem *self);

int htmlnode_parse_dom_file(htmlnode *self, FILE *file);
int htmlnode_parse_dom_str(htmlnode *self, const char input[]);
int htmlnode_parse_dom_net(htmlnode *self, int fd);
int htmlnode_parse_sax_file(FILE *file, void (*callback)(htmlnode_elem *elem, void *data), void *data);
int htmlnode_parse_sax_net(int fd, void (*callbac)(htmlnode_elem *elem, void *data), void *data);
int htmlnode_parse_sax_str(const char iput[], void (*callbac)(htmlnode_elem *elem, void *data), void *data);

#endif

#ifdef __cplusplus
}
#endif
