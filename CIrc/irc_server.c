/**
 * @file
 * @author niki
 * @date 1 Jan 2012
 */

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include "../CUtils/cstring.h"
#include "../CUtils/net.h"
#include "../CUtils/clist.h"
#include "irc_server.h"
#include "irc_client.h"
#include "irc_chan.h"

////////////////////////////
// structs and prototypes //
////////////////////////////

typedef struct {
	void *callback;
	void *data;
} irc_server_callback;

struct struct_irc_server_callbacks {
	clist *regist; /**< clist<irc_server_callback> */
	clist *join; /**< clist<irc_server_callback> */
	clist *idle; /**< clist<irc_server_callback> */
	clist *mess; /**< clist<irc_server_callback> */
};

typedef struct {
	irc_server *server;
	irc_user *user;
	irc_client *client;
	int socket;
} irc_server_connection;

irc_server_callback *irc_server_callback_new();
irc_server_callbacks *irc_server_callbacks_new();
irc_server_connection *irc_server_connection_new();

void irc_server_callback_free(irc_server_callback *self);
void irc_server_callbacks_free(irc_server_callbacks *self);
void irc_server_connection_free(irc_server_connection *self);

int irc_server_handle_line(irc_server_connection *con);
int irc_server_accept_connection(irc_server *server);

irc_server_connection *irc_server_get_connection(irc_server *self,
		const char username[]);
irc_chan *irc_server_get_chan(irc_server *self, const char channame[]);

void irc_server_privmsg_int(irc_server *self, const char target[], irc_user *user,
		const char message[], int notify_sender);

void irc_server_privmsg_u(irc_server_connection *con, const char target[],
		irc_user *user, const char message[]);

void irc_server_add_callback(clist *callbacks, void *function, void *data);

void irc_server_cstring_add_action(irc_server_connection *con, cstring *string,
		char action[]);

void irc_server_rpl_register(irc_server_connection *con);
void irc_server_rpl_topic(irc_server_connection *con, irc_chan *chan);
void irc_server_rpl_names(irc_server_connection *con, irc_chan *chan);
void irc_server_rpl_who(irc_server_connection *con, irc_chan *chan);

/**
 * Extract (and copy) the text part before the colon (bc).
 */
char *irc_server_extract_bc(const char input[]);

/**
 * Extract (and copy) the text part after the colon (ac).
 */
char *irc_server_extract_ac(const char input[]);

int irc_server_on_all_do(irc_client *client, const char from[],
		const char action[], const char args[], void *data);

//////////////////////////////////
// constructors and destructors //
//////////////////////////////////

irc_server_callback *irc_server_callback_new() {
	irc_server_callback *self;

	self = malloc(sizeof(irc_server_callback));
	self->callback = NULL;
	self->data = NULL;

	return self;
}

irc_server_callbacks *irc_server_callbacks_new() {
	irc_server_callbacks *self;

	self = malloc(sizeof(irc_server_callbacks));
	self->join = clist_new();
	self->regist = clist_new();
	self->idle = clist_new();
	self->mess = clist_new();

	return self;
}

irc_server_connection *irc_server_connection_new() {
	irc_server_connection *self;

	self = malloc(sizeof(irc_server_connection));
	self->socket = -1;
	self->user = irc_user_new();
	self->client = NULL;

	return self;
}

void irc_server_callback_free(irc_server_callback *self) {
	free(self);
}

void irc_server_callbacks_free(irc_server_callbacks *self) {
	if (self->join != NULL)
		clist_free(self->join);
	if (self->regist != NULL)
		clist_free(self->regist);
	if (self->idle != NULL)
		clist_free(self->idle);
	if (self->mess != NULL)
		clist_free(self->mess);
	free(self);
}

void irc_server_connection_free(irc_server_connection *self) {
	// Socket should already be closed.
	
	if (self->client != NULL)
		irc_client_free(self->client);
	if (self->user != NULL)
		irc_user_free(self->user);

	free(self);
}

irc_server *irc_server_new() {
	irc_server *self;

	self = malloc(sizeof(irc_server));
	
	self->server = NULL;
	self->port = -1;
	self->socket = -1;
	self->chans = clist_new();
	self->clients = clist_new();
	self->callbacks = irc_server_callbacks_new();
	self->cont = 1;
	
	return self;
}

void irc_server_free(irc_server *self) {
	if (self->server != NULL)
		free(self->server);
	if (self->chans != NULL)
		clist_free(self->chans);
	if (self->clients != NULL)
		clist_free(self->clients);
	if (self->callbacks != NULL)
		irc_server_callbacks_free(self->callbacks);
		
	free(self);
}

////////////////////////////
// other public functions //
////////////////////////////

void irc_server_set_name(irc_server *self, const char name[]) {
	cstring *string;

	string = cstring_new();
	cstring_adds(string, name);
	if (self->server != NULL)
		free(self->server);
	self->server = cstring_convert(string);
}

int irc_server_listen(irc_server *self, int port, int backlog) {
	self->socket = net_listen(port, backlog);
	self->port = port;

	if (self->socket >= 0)
		net_set_non_blocking(self->socket);

	return self->socket >= 0;
}

int irc_server_do_work(irc_server *self) {
	int work, did_work;
	irc_server_connection *con;
	clist_node *node, *to_del;

	did_work = 0;
	for (work = 1 ; work ; ) {
		work = irc_server_accept_connection(self);
		for (node = self->clients->first ; node != NULL ; ) {
			con = (irc_server_connection *)node->data;
			if (con->client && !irc_client_is_alive(con->client)) {
				to_del = node;
				node = node->next;
				clist_remove(self->clients, to_del);
				clist_node_free(to_del);
			} else {
				if (irc_server_handle_line(con))
					did_work = work = 1;
				node = node->next;
			}
		}
	}
	
	irc_server_idle(self);
	
	return did_work;
}

int irc_server_main_loop(irc_server *self) {
	if (self->socket < 0)
		return 0;
	
	while (self->cont)
		irc_server_do_work(self);

	return 1;
}

void irc_server_idle(irc_server *self) {
	clist_node *node;
	irc_server_callback *callback;
	void (*function)(irc_server *self, void *data);
	
	for (node = self->callbacks->idle->first; node != NULL; node
			= node->next) {
		callback = (irc_server_callback *)node->data;
		function = callback->callback;
		function(self, callback->data);
	}
}

void irc_server_join(irc_server *self, const char channame[], irc_user *user) {
	cstring *string;
	irc_chan *chan;
	irc_server_connection *con;
	clist_node *node;
	irc_server_callback *callback;
	void (*function)(irc_server *self, irc_user *user, 
		irc_chan *chan, void *data);

	chan = irc_server_get_chan(self, channame);
	
	// If the user is NULL, just create the chan
	if (!user)
		return;
	
	// Prepare JOIN message
	string = cstring_new();
	cstring_addc(string, ':');
	cstring_adds(string, user->hostmask);
	cstring_addc(string, ' ');
	cstring_adds(string, "JOIN :");
	cstring_adds(string, chan->name);
	
	// Don't answer if already joined (TODO: report error to client?)
	for (node = chan->users->first ; node != NULL ; node = node->next) {
		if (!strcmp(((irc_user *)node->data)->nick, user->nick)) {
			con = irc_server_get_connection(self, ((irc_user *)node->data)->nick);
			if (con != NULL) {
				irc_client_raw(con->client, string->string);
				irc_server_rpl_names(con, chan);
				irc_server_rpl_topic(con, chan);
			}
			
			// Free JOIN message
			cstring_free(string);
			
			return;
		}
	}
	
	irc_chan_add_user(chan, user);

	for (node = chan->users->first ; node != NULL ; node = node->next) {
		con = irc_server_get_connection(self, ((irc_user *)node->data)->nick);
		if (con != NULL)
			irc_client_raw(con->client, string->string);
	}
	
	// Free JOIN message
	cstring_free(string);
	
	con = irc_server_get_connection(self, user->nick);
	if (con != NULL) {		
		irc_server_rpl_names(con, chan);
		irc_server_rpl_topic(con, chan);
	}
	
	for (node = self->callbacks->join->first; node != NULL; node
			= node->next) {
		callback = node->data;
		function = callback->callback;
		function(self, user, chan, callback->data);
	}
}

void irc_server_topic(irc_server *self, const char channame[],
		const char topic[]) {
	irc_chan *chan;
	irc_server_connection *con;
	clist_node *node;
	
	chan = irc_server_get_chan(self, channame);
	irc_chan_set_topic(chan, topic);
	for (node = chan->users->first ; node != NULL ; node = node->next) {
		con = irc_server_get_connection(self, 
			((irc_user *)node->data)->nick);
		if (con != NULL)
			irc_server_rpl_topic(con, chan);
	}
}

int irc_server_privmsg_s(irc_server *self, const char target[], 
		const char  username[], const char message[]) {
	irc_server_connection *con;
	
	con = irc_server_get_connection(self, username);
	if (con)
		irc_server_privmsg(self, target, con->user, message);
	
	return !!con;
}

void irc_server_privmsg(irc_server *self, const char target[], irc_user *user,
		const char message[]) {
	irc_server_privmsg_int(self, target, user, message, 1);
}

///////////////
// callbacks //
///////////////

void irc_server_on_register(irc_server *self, void( callback)(irc_server *self,
		irc_user *user, void *data), void *data) {
	irc_server_add_callback(self->callbacks->regist, callback, data);
}

void irc_server_on_join(irc_server *self, void( callback)(irc_server *self,
		irc_user *user, irc_chan *chan, void *data), void *data) {
	irc_server_add_callback(self->callbacks->join, callback, data);
}

void irc_server_on_idle(irc_server *self, void( callback)(irc_server *self,
		void *data), void *data) {
	irc_server_add_callback(self->callbacks->idle, callback, data);
}

void irc_server_on_message(irc_server *self, void( callback)(irc_server *self,
		irc_user *user, const char target[], const char message[], void *data), 
		void *data) {
	irc_server_add_callback(self->callbacks->mess, callback, data);
}

///////////////////////
// private functions //
///////////////////////

int irc_server_handle_line(irc_server_connection *con) {
	if (con->socket < 0)
		return 0;	

	if (con->client == NULL) {
		con->client = irc_client_new();
		irc_client_connect_to(con->client, "127.0.0.1", con->server->port,
				con->socket);
		irc_client_on_all(con->client, irc_server_on_all_do, con);
	}

	return irc_client_do_work(con->client);
}

int irc_server_accept_connection(irc_server *self) {
	int work;
	int socket;
	irc_server_connection *con;
	clist_node *node;
	
	work = 0;
	for (socket = net_accept(self->socket) ; socket >= 0 ; 
			socket = net_accept(self->socket)) {
		work = 1;
		net_set_non_blocking(socket);
		
		con = irc_server_connection_new();
		con->socket = socket;
		con->server = self;
		
		node = clist_node_new();
		node->data = con;
		node->free_content = irc_server_connection_free;
		
		clist_add(self->clients, node);
	}
	
	return work;
}

irc_server_connection *irc_server_get_connection(irc_server *self,
		const char username[]) {
	clist_node *node;
	irc_server_connection *con;

	for (node = self->clients->first ; node != NULL ; 
			node = node->next) {
		con = (irc_server_connection *)node->data;
		if (con->user != NULL && !strcmp(con->user->nick, username))
			return con;
	}
	
	return NULL;
}

irc_chan *irc_server_get_chan(irc_server *self, const char channame[]) {
	clist_node *node;
	irc_chan *chan;

	chan = NULL;
	for (node = self->chans->first; node != NULL; node = node->next) {
		if (!strcmp(((irc_chan *) node->data)->name, channame)) {
			chan = (irc_chan *) node->data;
		}
	}

	if (chan == NULL) {
		chan = irc_chan_new();
		irc_chan_set_name(chan, channame);
		node = clist_node_new();
		node->data = chan;
		node->free_content = irc_chan_free;

		clist_add(self->chans, node);
	}
	
	return chan;
}

void irc_server_privmsg_int(irc_server *self, const char target[], irc_user *user,
		const char message[], int notify_sender) {
	irc_server_connection *con;
	irc_chan *chan;
	clist_node *node;
	irc_server_callback *callback;
	void (*function)(irc_server *self, irc_user *user, 
		const char target[], const char message[], void *data);
	
	con = irc_server_get_connection(self, target);
	// Target is a USER
	if (con != NULL) {
		irc_server_privmsg_u(con, target, user, message);
		if (notify_sender)
			irc_server_privmsg_u(con, user->nick, user, message);
	}
	// Target is a CHAN
	else {
		chan = irc_server_get_chan(self, target);
		if (chan != NULL) {
			// Tell all users from CHAN, caller excluded
			for (node = chan->users->first; node != NULL; node = node->next) {
				con = irc_server_get_connection(self,
						((irc_user *) (node->data))->nick);
				if ((con != NULL) && 
						(notify_sender || strcmp(con->user->nick, user->nick)))
					irc_server_privmsg_u(con, target, user, message);
			}
		}
	}
	
	for (node = self->callbacks->mess->first; node != NULL; node = node->next) {
		callback = node->data;
		function = callback->callback;
		function(self, user, target, message, callback->data);
	}
}

void irc_server_privmsg_u(irc_server_connection *con, const char target[],
		irc_user *user, const char message[]) {
	cstring *string;
	
	string = cstring_new();
	cstring_addc(string, ':');
	cstring_adds(string, user->hostmask);
	cstring_addc(string, ' ');
	cstring_adds(string, "PRIVMSG ");
	cstring_adds(string, target);
	cstring_adds(string, " :");
	cstring_adds(string, message);

	irc_client_raw(con->client, string->string);
	cstring_free(string);
}

void irc_server_add_callback(clist *callback_list, void *function, void *data) {
	irc_server_callback *callback;
	clist_node *node;

	callback = irc_server_callback_new();
	callback->callback = function;
	callback->data = data;

	node = clist_node_new();
	node->data = callback;
	node->free_content = irc_server_callback_free;

	clist_add(callback_list, node);
}

void irc_server_cstring_add_action(irc_server_connection *con, cstring *string,
		char action[]) {
	cstring_addc(string, ':');
	cstring_adds(string, con->server->server);
	cstring_addc(string, ' ');
	cstring_adds(string, action);
	cstring_addc(string, ' ');
	cstring_adds(string, con->user->nick);
	cstring_addc(string, ' ');
}

void irc_server_rpl_register(irc_server_connection *con) {
	cstring *string;

	clist_node *node;
	irc_server_callback *callback;
	void (*function)(irc_server *self, irc_user *target, void *data);
	
	string = cstring_new();

	irc_server_cstring_add_action(con, string, "001");
	cstring_adds(string, ":Welcome on CIrc");
	irc_client_raw(con->client, string->string);

	cstring_clear(string);
	irc_server_cstring_add_action(con, string, "002");
	cstring_adds(string, ":Your host is ");
	cstring_adds(string, con->server->server);
	cstring_adds(string, ", running version CIrc-0.1");
	irc_client_raw(con->client, string->string);

	cstring_clear(string);
	irc_server_cstring_add_action(con, string, "003");
	cstring_adds(string, ":This server was created 02:10:00 Jan  1 1987");
	irc_client_raw(con->client, string->string);

	cstring_clear(string);
	irc_server_cstring_add_action(con, string, "004");
	cstring_adds(string, con->server->server);
	cstring_adds(string, " CIrc-0.1");
	cstring_adds(string, " Rdikorsw MRabiklmnopqrstv abkloqv"); //TODO
	irc_client_raw(con->client, string->string);

	//TODO
	cstring_clear(string);
	irc_server_cstring_add_action(con, string, "005");
	cstring_adds(
			string,
			"AWAYLEN=201 CASEMAPPING=rfc1459 CHANMODES=b,k,l,MRimnprst CHANTYPES=# CHARSET=ascii ELIST=MU EXTBAN=,R FNC KICKLEN=256 MAP MAXBANS=60 MAXCHANNELS=20 MAXPARA=32 :are supported by this server");
	irc_client_raw(con->client, string->string);

	//TODO
	cstring_clear(string);
	irc_server_cstring_add_action(con, string, "005");
	cstring_adds(string, "MAXTARGETS=20 MODES=20 NAMESX NETWORK=");
	cstring_adds(string, con->server->server); // TODO should be a name, not a server name
	cstring_adds(
			string,
			" NICKLEN=32 PREFIX=(qaov)~&@+ SSL=[::]:6697 STARTTLS STATUSMSG=~&@+ TOPICLEN=308 UHNAMES VBANLIST WALLCHOPS :are supported by this server");
	irc_client_raw(con->client, string->string);

	//375, 372++, 376
	cstring_clear(string);
	irc_server_cstring_add_action(con, string, "375");
	cstring_adds(string, ":MOTD");
	irc_client_raw(con->client, string->string);

	cstring_clear(string);
	irc_server_cstring_add_action(con, string, "372");
	cstring_adds(string, ":My message of the day");
	irc_client_raw(con->client, string->string);

	cstring_clear(string);
	irc_server_cstring_add_action(con, string, "376");
	cstring_adds(string, ":End of MOTD");
	irc_client_raw(con->client, string->string);

	// 251
	cstring_clear(string);
	irc_server_cstring_add_action(con, string, "251");
	cstring_adds(string, ":There are 1 users and 1 invisible on 1 servers"); //TODO
	irc_client_raw(con->client, string->string);

	// 252
	cstring_clear(string);
	irc_server_cstring_add_action(con, string, "252");
	cstring_adds(string, "0 :operator(s) online"); //TODO
	irc_client_raw(con->client, string->string);

	// 254
	cstring_clear(string);
	irc_server_cstring_add_action(con, string, "254");
	cstring_adds(string, "0 :channels formed"); //TODO
	irc_client_raw(con->client, string->string);

	// 255
	cstring_clear(string);
	irc_server_cstring_add_action(con, string, "254");
	cstring_adds(string, ":I have 0 clients and 1 servers"); //TODO
	irc_client_raw(con->client, string->string);

	// 265, 266
	cstring_clear(string);
	irc_server_cstring_add_action(con, string, "265");
	cstring_adds(string, ":Current Local Users: 1  Max: 10"); //TODO
	irc_client_raw(con->client, string->string);

	cstring_clear(string);
	irc_server_cstring_add_action(con, string, "266");
	cstring_adds(string, ":Current Global Users: 1  Max: 10"); //TODO
	irc_client_raw(con->client, string->string);

	cstring_free(string);

	for (node = con->server->callbacks->regist->first; node != NULL;
			node = node->next) {
		callback = node->data;
		function = callback->callback;
		function(con->server, con->user, callback->data);
	}
}

void irc_server_rpl_topic(irc_server_connection *con, irc_chan *chan) {
	cstring *string;

	string = cstring_new();	
	if (chan->topic != NULL) {
		irc_server_cstring_add_action(con, string, "332"); //RPL_TOPIC
		cstring_addc(string, ' ');
		cstring_adds(string, chan->name);
		cstring_adds(string, " :");
		cstring_adds(string, chan->topic);
	} else {
		irc_server_cstring_add_action(con, string, "331"); //RPL_NOTOPIC
		cstring_addc(string, ' ');
		cstring_adds(string, chan->name);
		cstring_adds(string, " :");
		cstring_adds(string, "No topic is set");
	}

	irc_client_raw(con->client, string->string);
	cstring_free(string);
}

// chan: channel from which the /NAMES was asked
// target: the target user who asked the /NAMES
void irc_server_rpl_names(irc_server_connection *con, irc_chan *chan) {
	cstring *string;
	clist_node *node;

	string = cstring_new();
	irc_server_cstring_add_action(con, string, "353"); // RPL_NAMREPLY
	cstring_adds(string, "= ");
	cstring_adds(string, chan->name);
	for (node = chan->users->first; node != NULL; node = node->next) {
		cstring_addc(string, ' ');
		cstring_adds(string, ((irc_user *) node->data)->nick);
	}

	irc_client_raw(con->client, string->string);

	cstring_clear(string);
	irc_server_cstring_add_action(con, string, "366");
	cstring_adds(string, chan->name);
	cstring_adds(string, " :End of /NAMES list.");

	irc_client_raw(con->client, string->string);

	cstring_free(string);
}

// chan: channel from which the /NAMES was asked
// target: the target user who asked the /NAMES
void irc_server_rpl_who(irc_server_connection *con, irc_chan *chan) {
	cstring *string;
	clist_node *node;

	string = cstring_new();
	for (node = chan->users->first; node != NULL; node = node->next) {
		irc_server_cstring_add_action(con, string, "352");
		cstring_adds(string, chan->name);
		cstring_addc(string, ' ');
		cstring_adds(string, ((irc_user *) node->data)->nick);
		cstring_addc(string, ' ');
		cstring_adds(string, ((irc_user *) node->data)->hostname);
		cstring_addc(string, ' ');
		cstring_adds(string, ((irc_user *) node->data)->server);
		cstring_addc(string, ' ');
		cstring_adds(string, ((irc_user *) node->data)->nick); // TODO??
		cstring_adds(string, " H :0 ");
		cstring_adds(string, ((irc_user *) node->data)->real_name);

		irc_client_raw(con->client, string->string);
		cstring_clear(string);
	}

	irc_server_cstring_add_action(con, string, "315");
	cstring_adds(string, chan->name);
	cstring_adds(string, " :End of /WHO list.");

	irc_client_raw(con->client, string->string);
	cstring_free(string);

	/*
	 2 real examples :
	 ":irc.elynx.fr 352 user_nick #chanchan METAh ip-149.net-82-216-7.versailles2.rev.numericable.fr irc.elynx.fr metah H :0 bluhbluhbluh"
	 ":irc.elynx.fr 352 user_nick #chanchan Snarl ip-149.net-82-216-7.versailles2.rev.numericable.fr irc.elynx.fr Snarl H :0 Snarl"
	 */
}

char *irc_server_extract_bc(const char input[]) {
	cstring *string;
	int i;
	
	string = cstring_clones(input);
	i = cstring_finds(string, ":", 0);
	
	if (i >= 0) {
		// Remove the trailing space if any
		if (i > 0 && string->string[i - 1] == ' ')
			i--;
		
		cstring_cut_at(string, i);
		return cstring_convert(string);
	} else {
		return cstring_convert(string);
	}
}

char *irc_server_extract_ac(const char input[]) {
	cstring *string, *tmp;
	size_t i;
	
	string = cstring_clones(input);
	i = cstring_finds(string, ":", 0);
	
	if (i > 0 && (string->length - 1) > i) {
		tmp = cstring_substring(string, i + 1, 0);
		cstring_free(string);
		return cstring_convert(tmp);
	} else if (i == 0) {
		cstring_free(string);
		return cstring_sclones("");
	} else {
		cstring_free(string);
		return NULL;
	}
}

int irc_server_on_all_do(irc_client *client, const char from[],
		const char action[], const char args[], void *data) {
	
	char *ff = NULL;
	if (ff == from)
		ff = NULL;
	client = NULL;

	irc_server_connection *con;
	cstring *string;
	char *s1, *s2;

	con = (irc_server_connection *) data;

	if (!strcmp(action, "PING")) {
		// TODO: return PONG servername -or- PONG "ping request" ?
		string = cstring_new();
		cstring_adds(string, "PONG ");
		cstring_adds(string, con->server->server);
		irc_client_raw(con->client, string->string);
		cstring_free(string);
	} else if (!strcmp(action, "NICK")) {
		if (con->user->hostname != NULL && con->user->server != NULL) {
			irc_user_set_user(con->user, args, con->user->hostname,
					con->user->server);
			irc_server_rpl_register(con);
		} else {
			irc_user_set_user(con->user, args, "", "");
		}
	} else if (!strcmp(action, "USER")) {
		cstring *uargs = cstring_new();
		cstring *uhost = cstring_new();
		cstring *userver = cstring_new();
		cstring *uname = cstring_new();
		cstring *tmp;
		clist *list;

		cstring_adds(userver, con->server->server);

		cstring_adds(uargs, args);
		int i = cstring_finds(uargs, ":", 0);
		if (i >= 0) {
			if (i > 0) {
				cstring_addf(uname, uargs, i);
				tmp = cstring_substring(uargs, 0, i);
				list = cstring_splitc(tmp, ' ', '"');
				if (list->size >= 1)
					cstring_add(uhost, ((cstring *) clist_get(list, 0)->data));
				clist_free(list);
				cstring_free(tmp);
			}
		}
		cstring_free(uargs);

		irc_user_set_real_name(con->user, uname->string);
		if (con->user->nick != NULL) {
			irc_user_set_user(con->user, con->user->nick, uhost->string,
					userver->string);
			irc_server_rpl_register(con);
		} else {
			irc_user_set_user(con->user, "", uhost->string, userver->string);
		}

		cstring_free(uhost);
		cstring_free(userver);
		cstring_free(uname);
	} else if (!strcmp(action, "JOIN")) {
		irc_server_join(con->server, args, con->user);
	} else if (!strcmp(action, "NAMES")) {
		irc_server_rpl_names(con, irc_server_get_chan(con->server, args));
	} else if (!strcmp(action, "WHO")) {
		irc_server_rpl_who(con, irc_server_get_chan(con->server, args));
	} else if (!strcmp(action, "PRIVMSG")) {
		s1 = irc_server_extract_bc(args);
		s2 = irc_server_extract_ac(args);
		irc_server_privmsg_int(con->server, s1, con->user, s2, 0);
	}  else if (!strcmp(action, "TOPIC")) {
		s1 = irc_server_extract_bc(args);
		s2 = irc_server_extract_ac(args);
		
		// No arg means ListTopic, arg means ChangeTopic
		if (s2 != NULL)
			irc_server_topic(con->server, s1, s2);
		else
			irc_server_rpl_topic(con, irc_server_get_chan(con->server, s1));
		
		free(s1);
		free(s2);
	}

	return 1;
}
