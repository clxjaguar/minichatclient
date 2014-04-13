/**
 * @file
 * @author niki
 * @date 22 Sep 2012
 */

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "irc_client.h"
#include "irc_server.h"
#include "irc_user.h"
#include "../CUtils/cstring.h"
#include "../CUtils/net.h"

void on_client_register(irc_client *self, const char from[], const char args[],
		void *data) {
	clist_node *node;
	clist *list;
	
	if (from) {}
	if (args) {}
	if (data) {}

	//DEBUG:
	list = cstring_splitc((cstring*) data, '\n', '\0');
	for (node = list->first; node != NULL; node = node->next)
		irc_client_raw(self, ((cstring*) node->data)->string);
	clist_free(list);

	usleep(1000000);
	irc_client_stop(self);
}

int irc_send(char *from, char*to, char *server, int port, cstring*mess) {
	irc_client *client;
	cstring *full_mess;
	clist_node *node;
	clist *list;

	full_mess = cstring_new();
	list = cstring_splitc(mess, '\n', '\b');
	for (node = list->first; node != NULL; node = node->next) {
		//The last line is always empty..
		if (node->next != NULL) {
			if (full_mess->length > 0)
				cstring_addc(full_mess, '\n');
			cstring_adds(full_mess, "PRIVMSG ");
			cstring_adds(full_mess, to);
			cstring_adds(full_mess, " :");
			cstring_add(full_mess, ((cstring*) node->data));
			if (((cstring*) node->data)->length == 0)
				cstring_addc(full_mess, ' ');
		}
	}
	clist_free(list);

	client = irc_client_new();
	irc_client_set_auto_pong(client, 1);
	
	irc_client_on_register(client, on_client_register, full_mess);
	
	irc_client_connect(client, server, port, 0);
	irc_client_nick(client, from);
	irc_client_user(client, from, from, from, from);

	// loop:
	while (irc_client_is_alive(client)) {
		while (irc_client_do_work(client));
		usleep(10);
	}
	
	irc_client_free(client);
	cstring_free(full_mess);

	return 0;
}

int main(int argc, char *argv[]) {
	int i;
	cstring *string, *full;

	char *from = "irc-send";
	char *to = NULL;
	char *server = NULL;
	char *port = "6667";
	int iport = 6667;

	for (i = 0; i < argc; i++) {
		if (!strcmp(argv[i], "--from"))
			from = argv[++i];
		else if (!strcmp(argv[i], "--to"))
			to = argv[++i];
		else if (!strcmp(argv[i], "--server"))
			server = argv[++i];
		else if (!strcmp(argv[i], "--port"))
			port = argv[++i];
	}

	iport = atoi(port);

	string = cstring_new();
	full = cstring_new();
	while (cstring_readline(string, stdin)) {
		if (full->length > 0)
			cstring_addc(full, '\n');
		cstring_add(full, string);
	}
	cstring_free(string);

	int rep = 0;

	net_init();

	rep = irc_send(from, to, server, iport, full);

	cstring_free(full);
	net_cleanup();

	return rep;
}
