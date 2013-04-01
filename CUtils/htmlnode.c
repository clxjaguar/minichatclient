#include "htmlnode.h"

htmlnode *htmlnode_new() {
	htmlnode *self;

	self = (htmlnode *)malloc(sizeof(htmlnode));
	self->name = NULL;
	self->attributes = clist_new();
	self->nodes = clist_new();
	self->parent = NULL;

	return self;
}

void htmlnode_free(htmlnode *self) {
	if (!self)
		return;

	free(self->name);
	clist_free(self->attributes);
	clist_free(self->nodes);
	// note: we DO NOT free the parent

	free(self);
}

int htmlnode_parse(htmlnode *self, FILE *file) {
	//TODO
	return 0;
}

