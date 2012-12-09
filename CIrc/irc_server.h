/**
 * @file
 * @author niki
 * @date 1 Jan 2012
 *
 * Represent an IRC server.
 */

#ifndef IRC_SERVER_H_
#define IRC_SERVER_H_

#include "../CUtils/cstring.h"
#include "irc_client.h"
#include "irc_chan.h"

typedef struct struct_irc_server_callbacks irc_server_callbacks;

/**
 * A structure containing all the information related to a server instance.
 */
typedef struct {
	char *server;
	int port;
	int socket;
	clist *chans; /**< clist<irc_chan> */
	clist *clients; /**< clist<irc_server_connection> */
	irc_server_callbacks *callbacks;
	int cont;
} irc_server;

/** Constructors and destructors */
/*\@{*/
/**
 * Create a new IRC server.
 */
irc_server *irc_server_new();

/**
 * Free the given irc_server.
 *
 * @param self the server to act on
 */
void irc_server_free(irc_server *self);
/*\@}*/

/** Server actions */
/*\@{*/
/**
 * Change the IRC server name.
 *
 * @param self the server to act on
 * @param name the new name
 */
void irc_server_set_name(irc_server *self, const char name[]);

/**
 * Listen for network connections.
 *
 * @param self the server to act on
 * @param port the network port to listen on
 * @param backlog the number of client connections to buffer
 * 
 * @return true if we are listening
 */
int irc_server_listen(irc_server *self, int port, int backlog);

/**
 * Check if work needs to be done, and do it if needed.
 *
 * @param self the server to act on
 */
int irc_server_do_work(irc_server *self);

/**
 * Start the main server loop, which will continuously check if work
 * need to be done, and do it.
 * It will only exit when the server is stopped.
 *
 * @param self the server to act on
 */
int irc_server_main_loop(irc_server *self);

/**
 * Force a call to the idle callbacks.
 *
 * @param self the server to act on
 */
void irc_server_idle(irc_server *self);

/**
 * Force a user to join the given channel.
 * If the user is NULL, it will just create the channel.
 *
 * @param self the server to act on
 * @param channame the channel name, which will be created if needed
 * @param user the user to force-join or NULL
 */
void irc_server_join(irc_server *self, const char channame[], irc_user *user);

/**
 * Change the topic of the given channel.
 *
 * @param self the server to act on
 * @param channame the channel name, which will be created if needed
 * @param topic the new topic, or NULL to remove it
 */
void irc_server_topic(irc_server *self, const char channame[],
		const char topic[]);

/**
 * Send a message from the given user to the given target, be it another user
 * or a channel.
 *
 * @param self the server to act on
 * @param target the target user or target channel name
 * @param user the message sender
 * @param message the actual message to send
 */
void irc_server_privmsg(irc_server *self, const char target[], irc_user *user,
		const char message[]);

/**
 * Send a message from the given user to the given target, be it another user
 * or a channel.
 *
 * @param self the server to act on
 * @param target the target user or target channel name
 * @param username the message sender
 * @param message the actual message to send
 * 
 * @return TRUE if the sender user was found
 */
int irc_server_privmsg_s(irc_server *self, const char target[], 
		const char username[], const char message[]);

/** 
 * Ping all connected clients and call do_work once.
 *
 * @param self the server to act on
 */
void irc_server_ping_all(irc_server *self);
/*\@}*/

/** Callbacks */
/*\@{*/
/**
 * This callback will be called after a new user has been registered.
 *
 * @param self the server to act on
 * @param callback the method to call back, which will be called with:
 *  	- self: the server to act on
 *  	- user: the user who registered
 *   	- data: the data that was passed when the callback was set
 * @param data the data to pass to the callback
 */
void irc_server_on_register(irc_server *self, void( callback)(irc_server *self,
		irc_user *user, void *data), void *data);

/**
 * This callback will be called after a user has joined a channel.
 *
 * @param self the server to act on
 * @param callback the method to call back, which will be called with:
 *  	- self: the server to act on
 *  	- user: the user who joined
 *  	- chan: the channel which was joined
 *   	- data: the data that was passed when the callback was set
 * @param data the data to pass to the callback
 */
void irc_server_on_join(irc_server *self, void( callback)(irc_server *self,
		irc_user *user, irc_chan *chan, void *data), void *data);
		
/**
 * This callback will be called when the server is idle.
 * It is also called at the end of a do_work call.
 *
 * @param self the server to act on
 * @param callback the method to call back, which will be called with:
 *  	- self: the server to act on
 *   	- data: the data that was passed when the callback was set
 * @param data the data to pass to the callback
 */
void irc_server_on_idle(irc_server *self, void( callback)(irc_server *self,
		void *data), void *data);

/**
 * This callback will be called when the server sees a message.
 *
 * @param self the server to act on
 * @param callback the method to call back, which will be called with:
 *  	- self: the server to act on
 * 		- user: the message recipient
 * 		- target: the message target
 * 		- message: the message
 *   	- data: the data that was passed when the callback was set
 * @param data the data to pass to the callback
 */	
void irc_server_on_message(irc_server *self, void( callback)(irc_server *self,
		irc_user *user, const char target[], const char message[], void *data), 
		void *data);
/*\@}*/

#endif /* IRC_SERVER_H_ */
