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
	cstring *last_message;
	cstring *buffer;
	char *channel;
	char *topic;
	int port;
	char *username;
	char *ffname;
	int jpending; /*<< force join pending delay (starts at 0) */
	irc_user *juser;
	clist *nicklist;
	int topic_mode;
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
void mccirc_set_topic_mode(mccirc *self, int topic_mode);
//note: the result must be free()ed by the sender!!
char *mccirc_check_message(mccirc *self);
void mccirc_chatserver_error(mccirc *self);
void mccirc_chatserver_resume(mccirc *self);
void mccirc_chatserver_message(mccirc *self, const char username[], 
		const char message[]);
void mccirc_topic(mccirc *self, const char topic[]);
/*\@}*/

/** Nicklist actions */
/*\@{*/
void mccirc_nicks_start(mccirc *self);
void mccirc_nicks_stop(mccirc *self);
void mccirc_nicks_add(mccirc *self, const char username[]);
/*\@}*/

#endif /* MCCIRC_H_ */
