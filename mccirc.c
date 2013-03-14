/**
 * @file
 * @author niki
 * @date 1 Nov 2012
 */

#include <string.h>
#include "mccirc.h"
#include "CUtils/cstring.h"
#include "CUtils/clist.h"
#include "CIrc/irc_server.h"
#include "CIrc/irc_chan.h"

////////////////////////////
// structs and prototypes //
////////////////////////////

void on_server_message(irc_server *serv,
		irc_user *user, const char target[], const char message[], void *data);

void on_server_register(irc_server *serv, irc_user *user, void *data);

void mccirc_force_join(mccirc *self, irc_user *user);

/**
 * Check if the username is from the user who started the mccirc.
 * Note: it also answers TRUE if the user is not set (because if it
 * is not set, we cannot be sure it is not the same user).
 */
int mccirc_is_me(mccirc *self, const char username[]);

/**
 * Create a new user with the given name and add it to the
 * main channel.
 */
void mccirc_create_user(mccirc *self, const char username[]);

/**
 * Get the user with the given name from the main channel
 * (see mccirc_get_chan()) or NULL if no such user exist.
 *
 * Note: the user is the actual user, not a copy. DO NOT FREE.
 */
irc_user *mccirc_get_user(mccirc *self, const char nick[]);

/**
 * Sanitize the username (change the username to make it IRC
 * acceptable) in a reproducible way (same input will always
 * yields the same output).
 *
 * Note: the returned string is malloc'ed, you are responsible for freeing it.
 */
char *mccirc_sanitize_username(const char name[]);

/**
 * Return the main channel of this mccirc.
 * The main channel is the only channel this mccirc is
 * supposed to interact with. It is named after self->channel.
 *
 * Note: it is the actual channel, not a copy: DO NOT FREE.
 */
irc_chan *mccirc_get_chan(mccirc *self);

void mccirc_add_nick(mccirc *self, const char user[]);

void mccirc_remove_nick(mccirc *self, const char user[]);

//////////////////////////////////
// constructors and destructors //
//////////////////////////////////

mccirc *mccirc_new() {
	mccirc *self;
	
	self = malloc(sizeof(mccirc));
	self->server = NULL;
	self->buffer = NULL;
	self->channel = NULL;
	self->topic = NULL;
	self->port = -1;
	self->username = NULL;
	self->ffname = NULL;
	self->jpending = 0;
	self->juser = NULL;
	self->nicklist = NULL;
	
	return self;
}

void mccirc_free(mccirc *self) {
	// should not happen, but just in case:
	if (!self)
		return;
	
	irc_server_free(self->server);
	cstring_free(self->buffer);
	free(self->channel);
	free(self->topic);
	free(self->username);
	free(self->ffname);
	irc_user_free(self->juser);
	clist_free(self->nicklist);
	
	free(self);
}

////////////////////////////
// other public functions //
////////////////////////////

void mccirc_init(mccirc *self, const char ffname[], const char server_name[], 
		const char channel_name[], const char channel_topic[], int port) {
			
	// should not happen, but just in case:
	if (!self)
		return;
		
	self->channel = cstring_sclones(channel_name);
	self->buffer = cstring_new();
	self->ffname = mccirc_sanitize_username(ffname);
	
	self->server = irc_server_new();
	irc_server_set_name(self->server, server_name);
	irc_server_listen(self->server, port, 5);

	irc_server_on_register(self->server, on_server_register, self);
	irc_server_on_message(self->server, on_server_message, self);
	
	// create channel
	irc_server_join(self->server, self->channel, NULL);
	
	// set topic
	mccirc_topic(self, channel_topic);
}

//note: sender is NOT responsible for the memory,
// and the memory MUST be useable UNTIL the next call
// (hard to understand, in short: always free mem from last call or use bufer)
char *mccirc_check_message(mccirc *self) {
	// should not happen, but just in case:
	if (!self)
		return NULL;

	// take care of the optional pending force join
	if (self->juser) {
		self->jpending++;
		if (self->jpending > 20) {
			mccirc_force_join(self, self->juser);
			irc_user_free(self->juser);
			self->juser = NULL;
		}
	}

	if (self->buffer->length > 0)
		cstring_clear(self->buffer);

	// must set buffer if needed:
	irc_server_do_work(self->server);
	
	if (self->buffer->length > 0)
		return self->buffer->string;

	return NULL;
}

void mccirc_chatserver_error(mccirc *self) {
	if (self) {}
}

void mccirc_chatserver_resume(mccirc *self) {
	if (self) {}
}

void mccirc_add_nick(mccirc *self, const char name[]) {
	// should not happen, but just in case:
	if (!name || !self || !self->username)
		return;
	
	// do not add myself
	if (mccirc_is_me(self, name))
		return;
	
	if (!mccirc_get_user(self, name))
		mccirc_create_user(self, name);
}

void mccirc_remove_nick(mccirc *self, const char name[]) {
	irc_user *user;
	
	// should not happen, but just in case:
	if (!name || !self || !self->username)
		return;
	
	user = mccirc_get_user(self, name);
	if (user) {
		irc_server_part(self->server, self->channel, user, NULL);
		irc_chan_remove_user(mccirc_get_chan(self), user);
	}
}

void mccirc_chatserver_message(mccirc *self, const char name[], const char message[]) {
	irc_user *user;
	
	// should not happen, but just in case:
	if (!self || !self->username || !name || !message)
		return;
	
	// do not convey messages for the connected client
	if (mccirc_is_me(self, name))
		return;
	
	user = mccirc_get_user(self, name);
	if (!user) {
		mccirc_create_user(self, name);
		user = mccirc_get_user(self, name);
	}
	
	// only send message if user is still in channel
	if (mccirc_get_user(self, self->username))
		irc_server_privmsg(self->server, self->channel, user, message);
}

void mccirc_topic(mccirc *self, const char topic[]) {
	cstring *string, *tmp;
	char *TOPIC_SUFFIX = " ( basé sur les utilisateurs actifs depuis 10 minutes )";

	// Just in case
	if (!self)
		return;
	
	// Remote the SUFFIX if found
	string = topic ? cstring_clones(topic) : NULL;
	if (string && cstring_ends_withs(string, TOPIC_SUFFIX, 0)) {
		tmp = string;
		string = cstring_substring(tmp, 0, string->length - strlen(TOPIC_SUFFIX));
		cstring_free(tmp);
	}
	
	// Don't do anything if we already have that very same topic
	if (self->topic && topic && !strcmp(self->topic, string->string)) {
		cstring_free(string);
		return;
	}
	
	if (self->topic)
		free(self->topic);
	self->topic = cstring_convert(string);
	
	irc_server_topic(self->server, self->channel, self->topic);
}

void mccirc_nicks_start(mccirc *self) {
	// should not happen, but just in case:
	if (!self || !self->username)
		return;
	
	if (self->nicklist)
		clist_free(self->nicklist);
	
	self->nicklist = clist_new();
}

void mccirc_nicks_add(mccirc *self, const char username[]) {
	clist_node *node;

	// should not happen, but just in case:
	if (!self || !self->username)
		return;
	if (!self->nicklist)
		return;

	// grr!
	if (!username)
		return;
	
	node = clist_node_new();
	node->data = mccirc_sanitize_username(username);
	node->free_content = free;

	clist_add(self->nicklist, node);
}

void mccirc_nicks_stop(mccirc *self) {
	clist_node *node, *unode;
	char *username;
	irc_user *user;
	int isin;

	// should not happen, but just in case:
	if (!self || !self->username)
		return;
	if (!self->nicklist)
		return;
	
	for (node = self->nicklist->first ; node ; node = node->next) {
		username = (char *)node->data;

		if (mccirc_is_me(self, username))
			continue;
		if (mccirc_get_user(self, username))
			continue;

		mccirc_add_nick(self, username);
	}

	for (unode = mccirc_get_chan(self)->users->first ; unode ; ) {
		user = (irc_user *)unode->data;
		unode = unode->next;

		if (!mccirc_is_me(self, user->nick)) {
			isin = 0;
			for (node = self->nicklist->first ; !isin && node ; node = node->next) {
				username = (char *)node->data;
				if (!strcmp(username, user->nick))
					isin = 1;
			}

			if (!isin)
				mccirc_remove_nick(self, user->nick);
		}
	}
}


///////////////////////
// private functions //
///////////////////////

void on_server_message(irc_server *serv,
		irc_user *user, const char target[], const char message[], void *data) {
	mccirc *self = (mccirc *)data;
	
	if (serv) {}
	if (target) {}
	
	if (self->username && !strcmp(user->nick, self->username)) {
		cstring_clear(self->buffer);
		cstring_adds(self->buffer, message);
	}
}

void on_server_register(irc_server *serv, irc_user *user, void *data) {	
	mccirc *self = (mccirc *)data;
	clist_node *node;
	irc_user *u;
	char *tmp;
	
	// Check for dead clients, and remove them
	irc_server_ping_all(serv);
	
	// Clear and change the username for mccirc
	tmp = self->username;
	self->username = cstring_sclones(user->nick);
	if (tmp)
		free(tmp);
	
	// check that user is known by the server,
	// remove it if it is before /JOIN and /TOPIC
	u = NULL;
	for (node = mccirc_get_chan(self)->users->first ; node != NULL ; node = node->next) {
		if (!strcmp(((irc_user *)node->data)->nick, user->nick))
			u = (irc_user *)node->data;
	}
	
	if (u) {
		// remove user, make sure chan still exists
		irc_chan_remove_user(mccirc_get_chan(self), u);
		irc_server_join(self->server, self->channel, NULL);
	}
	
	// Force join later on if the client did not already do it
	self->jpending = 0;
	self->juser = irc_user_new();
	irc_user_set_hostmask(self->juser, user->hostmask);
	irc_user_set_real_name(self->juser, user->real_name);
	//
}

void mccirc_force_join(mccirc *self, irc_user *user) {
	// Check for dead clients, and remove them
	irc_server_ping_all(self->server);
	
	// only force if not already present
	if (!mccirc_get_user(self, user->nick)) {
		irc_server_topic(self->server, self->channel, self->topic);
		irc_server_join(self->server, self->channel, user);
	}
}

int mccirc_is_me(mccirc *self, const char username[]) {
	char *name;
	int me;
	
	name = mccirc_sanitize_username(username);
	
	me = 0;
	if (!self->username || !self->ffname
			|| !strcmp(name, self->ffname)
			|| !strcmp(name, self->username))
		me = 1;
	
	return me;
}

irc_chan *mccirc_get_chan(mccirc *self) {
	// create the channel if needed
	irc_server_join(self->server, self->channel, NULL);
	return (irc_chan *)self->server->chans->first->data;
}

irc_user *mccirc_get_user(mccirc *self, const char username[]) {
	clist_node *node;
	irc_user *user;
	irc_chan *chan;
	char *nick;
	
	nick = mccirc_sanitize_username(username);
	chan = mccirc_get_chan(self);

	user = NULL;
	for (node = chan->users->first; node != NULL; node = node->next) {
		if (!strcmp(((irc_user *)node->data)->nick, nick)) {
			user = (irc_user *)node->data;
		}
	}
	
	free(nick);
	return user;
}

void mccirc_create_user(mccirc *self, const char username[]) {
	cstring *mask;
	irc_user *user;
	char *sane_username;
	
	mask = cstring_new();
	sane_username = mccirc_sanitize_username(username);
	cstring_adds(mask, sane_username);
	cstring_addc(mask, '!');
	cstring_adds(mask, sane_username);
	cstring_addc(mask, '@');
	cstring_adds(mask, self->channel);
	free(sane_username);
	
	user = irc_user_new();
	irc_user_set_hostmask(user, mask->string);
	
	irc_server_join(self->server, self->channel, user);
	
	cstring_free(mask);
	irc_user_free(user);
}

char *mccirc_sanitize_username(const char name[]) {
	cstring *mask;
	
	// remove " " in the username
	mask = cstring_new();
	cstring_adds(mask, name);
	cstring_replaces(mask, " ", "_");
	cstring_replaces(mask, "@", "_");
	cstring_replaces(mask, "!", "_");
	
	return cstring_convert(mask);
}

