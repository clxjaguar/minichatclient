/**
 * @file
 * @author niki
 * @date 3 Jan 2012
 */

#include <string.h>

#include "../CUtils/cstring.h"
#include "irc_chan.h"

////////////////////////////
// structs and prototypes //
////////////////////////////


//////////////////////////////////
// constructors and destructors //
//////////////////////////////////

irc_chan *irc_chan_new() {
	irc_chan *self;

	self = malloc(sizeof(irc_chan));
	self->name = NULL;
	self->topic = NULL;
	self->users = clist_new();

	return self;
}

void irc_chan_free(irc_chan *self) {
	if (self->name != NULL)
		free(self->name);
	if (self->topic != NULL)
		free(self->topic);
	if (self->users != NULL)
		clist_free(self->users);

	free(self);
}

////////////////////////////
// other public functions //
////////////////////////////

void irc_chan_set_name(irc_chan *self, const char name[]) {
	char *string;
	
	string = name ? cstring_sclones(name) : NULL;
	
	if (self->name != NULL)
		free(self->name);
	
	self->name = string;
}

void irc_chan_set_topic(irc_chan *self, const char topic[]) {
	char *string;
	
	string = topic ? cstring_sclones(topic) : NULL;
	
	if (self->topic != NULL)
		free(self->topic);
	
	self->topic = string;
}

void irc_chan_add_user(irc_chan *self, irc_user *user) {
	clist_node *node;
	irc_user *copy;

	copy = irc_user_new();
	irc_user_set_hostmask(copy, user->hostmask);
	irc_user_set_real_name(copy, user->real_name);
	
	node = clist_node_new();
	node->data = copy;
	node->free_data = irc_user_free;

	clist_add(self->users, node);
}

int irc_chan_remove_user(irc_chan *self, irc_user *user) {
	clist_node *node, *tmp;
	irc_user *other;
	char *nick;
	int removed;
	
	nick = cstring_sclones(user->nick);
	
	removed = 0;
	for (node = self->users->first ; node != NULL ; ) {
		other = (irc_user *)node->data;
		
		// Do "next" BEFORE we remove it
		tmp = node;
		node = node->next;
		
		if (!strcmp(nick, other->nick)) {
			clist_remove(self->users, tmp);
			clist_node_free(tmp);
			removed++;
		}
	}
	
	free(nick);
	return removed;
}
