/**
 * @file
 * @author niki
 * @date 30 Dec 2011
 *
 * Represent an IRC client.
 */

#ifndef IRC_CLIENT_H_
#define IRC_CLIENT_H_

#include "irc_user.h"

/**
 * A structure containing all the information related to a client instance.
 */
typedef struct struct_irc_client irc_client;

/** Constructors and destructors */
/*\@{*/
/**
 * Create a new irc_client.
 */
irc_client *irc_client_new();

/**
 * Free all the resources allocated to this irc_client.
 */
void irc_client_free(irc_client *self);

/**
 * Set the debug mode (if enabled, all IN and OUT operations will be printed to
 * stderr with "IN :" or "OUT:" before them.
 *
 * @param debug true or false
 */
void irc_client_set_debug_mode(irc_client *self, int debug);
/*\@}*/

/** Client actions */
/*\@{*/
/**
 * Check if the client is in auto_pong mode.
 * Use true if you want this client to automatically send a PONG when the server
 * requests it with a PING -- if not, you have to do this manually or risk being
 * disconnected forcefully
 *
 * @param self the irc_client instance
 * 
 * @return the auto_pong current mode
 */
int irc_client_is_auto_pong(irc_client *self);

/**
 * Switch auto-pong on or off.
 * Use true if you want this client to automatically send a PONG when the server
 * requests it with a PING -- if not, you have to do this manually or risk being
 * disconnected forcefully
 *
 * @param self the irc_client instance
 * @param auto_pong true if you want this client to automatically send
 * 		a PONG when the server requests it with a PING -- if not, you
 * 		have to do this manually or risk being disconnected forcefully
 */
void irc_client_set_auto_pong(irc_client *self, int auto_pong);

/**
 * Check if the client is still alive (conection still working).
 *
 * @param self the irc_client instance
 *
 * @return true it if is
 */
int irc_client_is_alive(irc_client *self);

/**
 * Check if the client is registered.
 *
 * @param self the irc_client instance
 *
 * @return true it if is
 */
int irc_client_is_registered(irc_client *self);

/**
 * Connect to an IRC server.
 *
 * @param self the irc_client instance
 * @param server the server to connect to
 * @param port the port to connect to
 * @param blocking the connection blocking state (if true, all calls to do_work
 * 		  will be blocking if no data is immediatly available)
 *
 * @return true if success
 */
int irc_client_connect(irc_client *self, const char server[], int port,
		int blocking);

/**
 * "Connect" to an IRC server with an already connected socket.
 *
 * @param self the irc_client instance
 * @param server the server you connected to
 * @param port the port you connected to
 * @param socket the socket you already have
 *
 * @return true if success
 */
int irc_client_connect_to(irc_client *self, const char server[], int port,
		int socket);

/**
 * Stop the client.
 * 
 * @param self the irc_client to stop.
 */
void irc_client_stop(irc_client *self);

/**
 * Issue the NICK command.
 *
 * @param self the irc_client instance
 * @param nick the nickname you request
 *
 * @return true if success
 */
int irc_client_nick(irc_client *self, const char nick[]);

/**
 * Issue the USER command.
 *
 * @param self the irc_client instance
 * @param user your Unix user name
 * @param hostname your hostname
 * @param server the server you connect from
 * @param real_name your "real" name
 *
 * @return true if success
 */
int irc_client_user(irc_client *self, const char user[], const char hostname[],
		const char server[], const char real_name[]);

/**
 * Issue a raw command to the IRC server.
 *
 * @param self the irc_client instance
 * @param message the command to send (e.g. : "NICK my_nick")
 *
 * @return true if success
 */
int irc_client_raw(irc_client *self, const char message[]);
/*\@}*/

/** Work methods */
/*\@{*/
/**
 * Check if work needs to be done, and do it if needed.
 * Note that we only handle one request, then return.
 * 
 * If you want to drain the work queue, you can loop until there is no
 * work to do anymore (until we return false).
 * In this case, make sure you know if you are in blocking mode or not.
 *
 * @param self the client to act on
 * 
 * @return true if work was done
 */
int irc_client_do_work(irc_client *self);

/**
 * Starts the main loop of this IRC client
 * (getting and dispatching the messages received from the IRC server).
 *
 * @param self the irc_client instance
 * @param auto_pong true if you want this client to automatically send
 * 		a PONG when the server requests it with a PING -- if not, you
 * 		have to do this manually or risk being disconnected forcefully
 *
 * @return true if work has been handled during the session
 */
int irc_client_main_loop(irc_client *self, int auto_pong);
/*\@}*/

/** Callbacks */
/*\@{*/
/**
 * Add a callback which is called every time a message is received
 * from the server.
 *
 * @param self the irc_client instance
 * @param callback this callback will receive the following parameters:
 * 		- self: the irc_client instance
 * 		- from: whom (or what) the message comes from
 * 		- action: the action (e.g. "PRIVMSG" or "042" or...)
 * 		- args: the rest of the parameters
 * 		- data: the data that is passed as-is
 * 		and it should return true if the event if handled, false if not
 * 		(if it is handled, nothing more is done with this event, even if
 * 		other callback are waiting).
 * @param data the data that will be passed as-is to the callback, every time
 */
void irc_client_on_all(irc_client *self, int(*callback)(irc_client *self,
		const char from[], const char action[], const char args[], void *data),
		void *data);

/**
 * Add a callback which is called when a PING is received from the server.
 *
 * @param self the irc_client instance
 * @param callback this callback will receive the following parameters:
 * 		- self: the irc_client instance
 * 		- ping: the ping code to return in a PONG request
 * 		- data: the data that is passed as-is
 * @param data the data that will be passed as-is to the callback, every time
 */
void irc_client_on_ping(irc_client *self, void(*callback)(irc_client *self,
		const char ping[], void *data), void *data);

/**
 * Add a callback which is called when a PRIVMSG is received from the server.
 *
 * @param self the irc_client instance
 * @param callback this callback will receive the following parameters:
 * 		- self: the irc_client instance
 * 		- from: whom the message comes from
 * 		- to: the message recipient
 * 		- message: the message itself
 * 		- data: the data that is passed as-is
 * @param data the data that will be passed as-is to the callback, every time
 */
void irc_client_on_privmsg(irc_client *self, void(*callback)(irc_client *self,
		irc_user *from, const char to[], const char message[], void *data),
		void *data);

/**
 * Add a callback which is called when a NOTICE is received from the server.
 *
 * @param self the irc_client instance
 * @param callback this callback will receive the following parameters:
 * 		- self: the irc_client instance
 * 		- from: whom (or what) the message comes from
 * 		- message: the message itself
 * 		- data: the data that is passed as-is
 * @param data the data that will be passed as-is to the callback, every time
 */
void irc_client_on_notice(irc_client *self, void(*callback)(irc_client *self,
		const char from[], const char message[], void *data), void *data);

/**
 * Add a callback which is called when a numerical message is received
 * from the server.
 *
 * @param self the irc_client instance
 * @param callback this callback will receive the following parameters:
 * 		- self: the irc_client instance
 * 		- from: whom (or what) the message comes from
 * 		- action: the action number (42 or 1 or...)
 * 		- args: the rest of the parameters
 * 		- data: the data that is passed as-is
 * @param data the data that will be passed as-is to the callback, every time
 */
void irc_client_on_num(irc_client *self, void(*callback)(irc_client *self,
		const char from[], int action, const char args[], void *data),
		void *data);

/**
 * Add a callback which is called when a client register on this server.
 *
 * @param self the irc_client instance
 * @param callback this callback will receive the following parameters:
 * 		- self: the irc_client instance
 * 		- from: whom (or what) the message comes from
 * 		- args: the rest of the parameters
 * 		- data: the data that is passed as-is
 * @param data the data that will be passed as-is to the callback, every time
 */
void irc_client_on_register(irc_client *self, void(*callback)(irc_client *self,
		const char from[], const char args[], void *data), void *data);

/**
 * Add a callback which is called when a message is received from the server,
 * but is not handled by another callback (except "all").
 *
 * @param self the irc_client instance
 * @param callback this callback will receive the following parameters:
 * 		- self: the irc_client instance
 * 		- from: whom (or what) the message comes from
 * 		- action: the action (e.g. "PRIVMSG" or "042" or...)
 * 		- args: the rest of the parameters
 * 		- data: the data that is passed as-is
 * @param data the data that will be passed as-is to the callback, every time
 */
void irc_client_on_other(irc_client *self, void(*callback)(irc_client *self,
		const char from[], const char action[], const char args[], void *data),
		void *data);
/*\@}*/

#endif /* IRC_CLIENT_H_ */
