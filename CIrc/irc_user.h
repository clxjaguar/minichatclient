/**
 * @file
 * @author niki
 * @date 3 Jan 2012
 *
 * Represent an IRC user.
 */

#ifndef IRC_USER_H_
#define IRC_USER_H_

/**
 * Represent an IRC user.
 */
typedef struct {
	char *nick; /**< The user nickname. */
	char *hostname; /**< The user hostname. */
	char *server; /**< The user server. */
	char *hostmask; /**< The user hostmask, which contains the hostname and server. */
	char *real_name; /**< The user real name. */

} irc_user;

/** Constructors and destructors */
/*\@{*/
/**
 * Create a new, empty IRC user.
 */
irc_user *irc_user_new();

/**
 * Free the given IRC user.
 *
 * @param self the irc_user to act on
 */
void irc_user_free(irc_user *self);
/*\@}*/

/** User actions */
/*\@{*/
/**
 * Change the hostmask (and the related hostname and server)
 * of the given IRC user.
 *
 * @param self the irc_user to act on
 * @param hostmask the new hostmask (which contains the hostname and server)
 */
void irc_user_set_hostmask(irc_user *self, const char hostmask[]);

/**
 * Change the nick, the hostname and the server of this IRC user (the related
 * hostmask will change, too).
 *
 * @param self the irc_user to act on
 * @param nick the new nick
 * @param hostname the new hostname
 * @param server the new server
 */
void irc_user_set_user(irc_user *self, const char nick[],
		const char hostname[], const char server[]);

/**
 * Change the real name of this IRC user.
 *
 * @param self the irc_user to act on
 * @param real_name the new real name
 */
void irc_user_set_real_name(irc_user *self, const char real_name[]);
/*\@}*/

#endif /* IRC_USER_H_ */
