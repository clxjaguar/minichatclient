/**
 * @file
 * @author niki
 * @date 3 Jan 2012
 *
 * Represent an IRC channel.
 */

#ifndef IRC_CHAN_H_
#define IRC_CHAN_H_

#include "../CUtils/clist.h"
#include "irc_user.h"

/**
 * Represent an IRC channel.
 */
typedef struct {
	char *name; /**< The channel name. */
	char *topic; /**< The channel topic. */
	clist *users; /**< The list of irc_user present in this channel. */
} irc_chan;

/** Constructors and destructors */
/*\@{*/
/**
 * Create a new IRC channel.
 */
irc_chan *irc_chan_new();

/**
 * Free the given IRC channel.
 *
 * @param self the IRC channel to act on
 */
void irc_chan_free(irc_chan *self);
/*\@}*/

/** Channel actions */
/*\@{*/
/**
 * Set the channel name.
 *
 * @param self the IRC channel to act on
 * @param name the new name
 */
void irc_chan_set_name(irc_chan *self, const char name[]);

/**
 * Set the channel topic.
 *
 * @param self the IRC channel to act on
 * @param name the new topic
 */
void irc_chan_set_topic(irc_chan *self, const char topic[]);

/**
 * Add (and copy!) a user to the channel.
 *
 * @param self the IRC channel to act on
 * @param user the user from which a copy will be added to this channel
 */
void irc_chan_add_user(irc_chan *self, irc_user *user);

/**
 * Remove (and free) a user from this channel.
 *
 * @param self the IRC channel to act on
 * @param user the user to remove from this channel
 *
 * @return true if a user was removed
 */
int irc_chan_remove_user(irc_chan *self, irc_user *user);
/*\@}*/

#endif /* IRC_CHAN_H_ */
