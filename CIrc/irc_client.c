/*
 * @file
 * @author niki
 * @date 30 Dec 2011
 *
 *
 */
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>

#include "irc_client.h"
#include "irc_user.h"
#include "../CUtils/net.h"
#include "../CUtils/cstring.h"
#include "../CUtils/clist.h"

////////////////////////////
// structs and prototypes //
////////////////////////////

typedef struct {
	void *callback;
	void *data;
} irc_client_callback;

typedef struct {
	clist *all;
	clist *ping;
	clist *privmsg;
	clist *notice;
	clist *num;
	clist *other;
	clist *regist;
} irc_client_callbacks;

struct struct_irc_client {
	irc_user *user;
	irc_client_callbacks *callbacks;
	char *server;
	int auto_pong; /**< the index of the auto_pong callback, or -1 if none */
	int port;
	int socket;
	FILE *file;
	cstring *buffer;
	int registered;
	int debug;
	int cont;
};

irc_client_callback *irc_client_new_callback();
void irc_client_free_callback(irc_client_callback *self);

irc_client_callbacks *irc_client_new_callbacks();
void irc_client_free_callbacks(irc_client_callbacks *self);

/**
 * Simple "ping auto-answer" function to be used as a pong callback.
 */
void irc_client_on_ping_pong(irc_client *self, const char ping[], void *data);

/**
 * Simply add a callback to the given list.
 */
void irc_client_add_callback(clist *callbacks, void *callback, void *data);

// The following functions simply forward the call the the list of
// callbacks ("all" is special, it returns true as soon as a callback
// returns true, without checking the next ones).
int irc_client_on_all_do(irc_client *self, const char from[],
		const char action[], const char args[]);
void irc_client_on_ping_do(irc_client *self, const char ping[]);
void irc_client_on_privmsg_do(irc_client *self, const char from[],
		const char to[], const char message[]);
void irc_client_on_notice_do(irc_client *self, const char from[],
		const char message[]);
void irc_client_on_num_do(irc_client *self, const char from[], int action,
		const char args[]);
void irc_client_on_other_do(irc_client *self, const char from[],
		const char action[], const char args[]);
//

/**
 * Handles the given line of text and work on it.
 *
 * @param self the irc_client instance
 * @param line the line to process
 *
 * @return true if the line was understood by the client
 */
int irc_client_handle_line(irc_client *self, const char line[]);

//////////////////////////////////
// constructors and destructors //
//////////////////////////////////

irc_client_callback *irc_client_new_callback() {
	irc_client_callback *self;

	self = malloc(sizeof(irc_client_callback));
	self->callback = NULL;
	self->data = NULL;

	return self;
}

void irc_client_free_callback(irc_client_callback *self) {
	free(self);
}

irc_client_callbacks *irc_client_new_callbacks() {
	irc_client_callbacks *self;

	self = malloc(sizeof(irc_client_callbacks));
	self->all = clist_new();
	self->ping = clist_new();
	self->privmsg = clist_new();
	self->notice = clist_new();
	self->num = clist_new();
	self->other = clist_new();
	self->regist = clist_new();

	return self;
}

void irc_client_free_callbacks(irc_client_callbacks *self) {
	clist_free(self->all);
	clist_free(self->ping);
	clist_free(self->privmsg);
	clist_free(self->notice);
	clist_free(self->num);
	clist_free(self->other);
	clist_free(self->regist);

	free(self);
}

irc_client *irc_client_new() {
	irc_client * self;

	self = malloc(sizeof(irc_client));
	self->user = irc_user_new();
	self->callbacks = irc_client_new_callbacks();
	self->server = NULL;
	self->auto_pong = -1;
	self->port = 0;
	self->socket = -1;
	self->file = NULL;
	self->buffer = cstring_new();
	self->debug = 0;
	self->cont = 1;

	return self;
}

void irc_client_free(irc_client *self) {
	if (self->file != NULL)
		fclose(self->file);
	if (self->user != NULL)
		irc_user_free(self->user);
	if (self->server != NULL)
		free(self->server);
	if (self->buffer != NULL)
		cstring_free(self->buffer);
	if (self->callbacks != NULL)
		irc_client_free_callbacks(self->callbacks);

	free(self);
}

////////////////////////////
// other public functions //
////////////////////////////

int irc_client_is_auto_pong(irc_client *self) {
	return self->auto_pong > -1;
}

void irc_client_set_auto_pong(irc_client *self, int auto_pong) {
	clist_node *node;
	
	if (irc_client_is_auto_pong(self) && !auto_pong) {
		node = clist_get(self->callbacks->ping, self->auto_pong);
		node = clist_remove(self->callbacks->ping, node);
		self->auto_pong = -1;
		if (node != NULL)
			clist_node_free(node);
	}
	else if (!irc_client_is_auto_pong(self) && auto_pong) {
		self->auto_pong = self->callbacks->ping->size;
		irc_client_on_ping(self, irc_client_on_ping_pong, NULL);
	}
}

void irc_client_set_debug_mode(irc_client *self, int debug) {
	self->debug = debug;
}

void irc_client_stop(irc_client *self) {
	self->cont = 0;
}

int irc_client_is_registered(irc_client *self) {
	return self->registered;
}

int irc_client_is_alive(irc_client *self) {
	return self->cont;
}

int irc_client_connect(irc_client *self, const char server[], int port,
		int blocking) {
	int socket;
	
	socket = net_connect(server, port);
	if (!blocking)
		net_set_non_blocking(socket);
	
	return irc_client_connect_to(self, server, port, socket);
}

int irc_client_connect_to(irc_client *self, const char server[], int port,
		int socket) {

	if (self->server != NULL)
		free(self->server);
		
	self->server = cstring_sclones(server);
	self->port = port;
	self->socket = socket;

	return self->socket;
}

int irc_client_nick(irc_client *self, const char nick[]) {
	cstring *mess;
	size_t bytes;
	int ok;

	mess = cstring_new();
	cstring_adds(mess, nick);
	irc_user_set_user(self->user, cstring_convert(mess),
			self->user->hostname, self->user->server);

	mess = cstring_new();
	cstring_adds(mess, "NICK ");
	cstring_adds(mess, nick);
	cstring_adds(mess, "\r\n");
	bytes = (size_t) net_write(self->socket, mess->string, mess->length);
	ok = bytes == mess->length;

	if (self->debug)
		fprintf(stderr, "OUT: %s\n", mess->string);

	cstring_free(mess);

	return ok;
}

int irc_client_user(irc_client *self, const char user[], const char hostname[],
		const char server[], const char real_name[]) {
	cstring *mess;
	size_t bytes;
	int ok;

	irc_user_set_user(self->user, user, hostname, server);
	irc_user_set_real_name(self->user, real_name);

	mess = cstring_new();
	cstring_adds(mess, "USER ");
	cstring_adds(mess, user);
	cstring_adds(mess, " ");
	cstring_adds(mess, hostname);
	cstring_adds(mess, " ");
	cstring_adds(mess, server);
	cstring_adds(mess, " :");
	cstring_adds(mess, real_name);
	cstring_adds(mess, "\r\n");
	bytes = (size_t) net_write(self->socket, mess->string, mess->length);
	ok = bytes == mess->length;

	if (self->debug)
		fprintf(stderr, "OUT: %s\n", mess->string);

	cstring_free(mess);

	return ok;
}

int irc_client_raw(irc_client *self, const char message[]) {
	cstring *mess;
	ssize_t sbytes;
	int ok;

	mess = cstring_new();
	cstring_adds(mess, message);
	cstring_adds(mess, "\r\n");
	sbytes = net_write(self->socket, mess->string, mess->length);
	ok = (sbytes >= 0 && ((size_t) sbytes) == mess->length);

	if (self->debug)
		fprintf(stderr, ok ? "OUT: %s" : "O!!: %s", mess->string);

	cstring_free(mess);
	
	if (!ok && errno == SIGPIPE)
		irc_client_stop(self);
	
	return ok;
}

int irc_client_do_work(irc_client *self) {
	int work;
	
	if (!self->cont)
		return 0;
	
	if (self->file == NULL)
		self->file = fdopen(self->socket, "r");
	if (self->file == NULL)
		return 0;
	
	//TODO: support partial lines in buffer
	cstring_readline(self->buffer, self->file);
	work = 0;
	if (self->buffer->length > 0) {
		irc_client_handle_line(self, self->buffer->string);
		work = 1;
		cstring_clear(self->buffer);
	}
	
	return work;
}

void irc_client_on_all(irc_client *self, int( callback)(irc_client *self,
		const char from[], const char action[], const char args[], void *data),
		void *data) {
	irc_client_add_callback(self->callbacks->all, callback, data);
}

void irc_client_on_ping(irc_client *self, void(*callback)(irc_client *self,
		const char ping[], void *data), void *data) {
	irc_client_add_callback(self->callbacks->ping, callback, data);
}

void irc_client_on_privmsg(irc_client *self, void(*callback)(irc_client *self,
		irc_user *from, const char to[], const char message[], void *data),
		void *data) {
	irc_client_add_callback(self->callbacks->privmsg, callback, data);
}

void irc_client_on_notice(irc_client *self, void(*callback)(irc_client *self,
		const char from[], const char message[], void *data), void *data) {
	irc_client_add_callback(self->callbacks->notice, callback, data);
}

void irc_client_on_num(irc_client *self, void(*callback)(irc_client *self,
		const char from[], int action, const char args[], void *data),
		void *data) {
	irc_client_add_callback(self->callbacks->num, callback, data);
}

void irc_client_on_register(irc_client *self, void(*callback)(irc_client *self,
		const char from[], const char args[], void *data), void *data) {
	irc_client_add_callback(self->callbacks->regist, callback, data);
}

void irc_client_on_other(irc_client *self, void(*callback)(irc_client *self,
		const char from[], const char action[], const char args[], void *data),
		void *data) {
	irc_client_add_callback(self->callbacks->other, callback, data);
}

///////////////////////
// private functions //
///////////////////////

void irc_client_add_callback(clist *callbacks, void *function, void *data) {
	irc_client_callback *callback;
	clist_node *node;

	callback = irc_client_new_callback();
	callback->callback = function;
	callback->data = data;

	node = clist_node_new();
	node->data = callback;
	node->free_data = irc_client_free_callback;

	clist_add(callbacks, node);
}

void irc_client_on_ping_pong(irc_client *self, const char ping[], void *data) {
	cstring *pong;

	if (data != NULL)
		data = NULL;

	pong = cstring_new();
	cstring_adds(pong, "PONG ");
	cstring_adds(pong, ping);

	irc_client_raw(self, pong->string);

	cstring_free(pong);
}

int irc_client_on_all_do(irc_client *self, const char from[],
		const char action[], const char args[]) {
	clist_node *node;
	int handled;
	irc_client_callback *callback;
	int (*function)(irc_client *self, const char from[], const char action[],
			const char args[], void *data);

	handled = 0;
	for (node = self->callbacks->all->first; !handled && node != NULL; node
			= node->next) {
		callback = node->data;
		function = callback->callback;
		handled = function(self, from, action, args, callback->data);
	}

	return handled;
}

void irc_client_on_ping_do(irc_client *self, const char ping[]) {
	clist_node *node;
	irc_client_callback *callback;
	void (*function)(irc_client *self, const char ping[], void *data);

	for (node = self->callbacks->ping->first; node != NULL; node = node->next) {
		callback = node->data;
		function = callback->callback;
		function(self, ping, callback->data);
	}
}

void irc_client_on_privmsg_do(irc_client *self, const char from[],
		const char to[], const char message[]) {
	clist_node *node;
	irc_user *ufrom;
	irc_client_callback *callback;
	void (*function)(irc_client *self, irc_user *from, const char to[],
			const char message[], void *data);

	if (self->callbacks->privmsg->first != NULL) {
		ufrom = irc_user_new();
		irc_user_set_hostmask(ufrom, from);
		for (node = self->callbacks->privmsg->first; node != NULL; node
				= node->next) {
			callback = node->data;
			function = callback->callback;
			function(self, ufrom, to, message, callback->data);
		}
	}
}

void irc_client_on_notice_do(irc_client *self, const char from[],
		const char message[]) {
	clist_node *node;
	irc_client_callback *callback;
	void (*function)(irc_client *self, const char from[], const char message[],
			void *data);

	for (node = self->callbacks->notice->first; node != NULL; node = node->next) {
		callback = node->data;
		function = callback->callback;
		function(self, from, message, callback->data);
	}
}

void irc_client_on_num_do(irc_client *self, const char from[], int action,
		const char args[]) {
	clist_node *node;
	irc_client_callback *callback;
	int (*function)(irc_client *self, const char from[], int action,
			const char args[], void *data);
	int (*function_r)(irc_client *self, const char from[], const char args[],
			void *data);

	if (action > 5) {
		if (!self->registered) {
			self->registered = 1;
			for (node = self->callbacks->regist->first; node != NULL; node
					= node->next) {
				callback = node->data;
				function_r = callback->callback;
				function_r(self, from, args, callback->data);
			}
		}
	}

	for (node = self->callbacks->num->first; node != NULL; node = node->next) {
		callback = node->data;
		function = callback->callback;
		function(self, from, action, args, callback->data);
	}
}

void irc_client_on_other_do(irc_client *self, const char from[],
		const char action[], const char args[]) {
	clist_node *node;
	irc_client_callback *callback;
	int (*function)(irc_client *self, const char from[], const char action[],
			const char args[], void *data);

	for (node = self->callbacks->other->first; node != NULL; node = node->next) {
		callback = node->data;
		function = callback->callback;
		function(self, from, action, args, callback->data);
	}
}

int irc_client_handle_line(irc_client *self, const char line[]) {
	cstring *string;
	long long first_space;
	size_t i;
	cstring *action, *from, *args, *ping, *message, *tmp;
	int num;
	char *sfrom;
	char *sargs;
	int ok;

	if (self->debug)
		fprintf(stderr, " IN: %s\n", line);

	string = cstring_new();
	cstring_adds(string, line);
	ok = 1;
	action = NULL;
	from = NULL;

	for (i = 0; i < string->length && string->string[i] == ' '; i++)
		;
	first_space = cstring_finds(string, " ", i);

	if (string->length > i && first_space > 0) {
		action = cstring_substring(string, i, first_space - i);
		from = NULL;

		if (action->length > 0) {
			if (action->string[0] == ':') {
				from = cstring_substring(action, 1, 0);
				cstring_free(action);
				action = NULL;

				for (i = first_space; i < string->length && string->string[i]
						== ' '; i++)
					;
				first_space = cstring_finds(string, " ", i);

				if (string->length > i && first_space > 0) {
					action = cstring_substring(string, i, first_space - i);
				}
			}

			if (action != NULL && action->length > 0) {
				args = cstring_substring(string, i + action->length + 1, 0);

				sfrom = from == NULL ? NULL : from->string;
				sargs = args->string;

				// apply the global catch, and see if we need to continue
				if (!irc_client_on_all_do(self, sfrom, action->string, sargs)) {
					if (!strcmp(action->string, "PING")) {
						if (args != NULL && args->length > 0) {
							ping = cstring_substring(args, 1, 0);
							irc_client_on_ping_do(self, ping->string);
							cstring_free(ping);
						}
					} else if (!strcmp(action->string, "NOTICE")) {
						irc_client_on_notice_do(self, sfrom, sargs);
					} else if (!strcmp(action->string, "PRIVMSG")) {
						for (i = 0; i < args->length && args->string[i] == ' '; i++)
							;
						first_space = cstring_finds(args, " ", i);
						if (((size_t) first_space + 1 < args->length)
								&& args->string[first_space + 1] == ':')
							first_space++;
						message = cstring_substring(args, first_space + 1, 0);
						cstring_cut_at(args, first_space);
						irc_client_on_privmsg_do(self, sfrom, sargs,
								message->string);
					} else {
						// check if we have a numerical action (always 3 numbers)
						num = atoi(action->string);
						tmp = cstring_new();
						if (num < 100)
							cstring_adds(tmp, "0");
						if (num < 10)
							cstring_adds(tmp, "0");
						cstring_addi(tmp, num);
						if (!strcmp(tmp->string, action->string)) {
							// numerical action
							irc_client_on_num_do(self, sfrom, num, sargs);
						} else {
							// other action
							irc_client_on_other_do(self, sfrom, action->string,
									sargs);
						}
						cstring_free(tmp);
					}
				}

				cstring_free(args);
			}
		}

		if (action != NULL)
			cstring_free(action);
		if (from != NULL)
			cstring_free(from);
	}

	cstring_free(string);
	return ok;
}

