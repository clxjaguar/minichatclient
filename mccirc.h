/**
 * @file
 * @author niki
 * @date 1 Nov 2012
 *
 * An IRC plugin for minichatclient.
 */

#ifndef MCCIRC_H_
#define MCCIRC_H_

#include "CIrc/irc_server.h"
#include "CUtils/clist.h"

/**
 * Represent the plugin.
 */
typedef struct {
	irc_server* server;
	cstring *buffer;
	char *channel;
	char *topic;
	int port;
	char *username;
	char *ffname;
	int jpending; /*<< force join pending delay (starts at 0) */
	irc_user *juser;
} mccirc;

/** Constructors and destructors */
/*\@{*/
/**
 * Create a new IRC plugin.
 */
mccirc *mccirc_new();

/**
 * Free the plugin.
 *
 * @param self the plugin to act on
 */
void mccirc_free(mccirc *self);
/*\@}*/

/** User actions */
/*\@{*/

void mccirc_init(mccirc *self, const char server_name[], const char ffname[],
		const char channel_name[], const char channel_topic[], int port);
//note: the result must be free()ed by the sender!!
char *mccirc_check_message(mccirc *self);
void mccirc_chatserver_error(mccirc *self);
void mccirc_chatserver_resume(mccirc *self);
void mccirc_chatserver_message(mccirc *self, const char username[], 
		const char message[]);
void mccirc_clear_nicklist(mccirc *self);
void mccirc_add_nick(mccirc *self, const char user[]);
void mccirc_remove_nick(mccirc *self, const char user[]);
void mccirc_topic(mccirc *self, const char topic[]);
/*\@}*/

#endif /* MCCIRC_H_ */
