/**
 * @file
 * @author niki
 * @date 3 Jan 2012
 */
 
#include "irc_user.h"
#include "../CUtils/cstring.h"

////////////////////////////
// structs and prototypes //
////////////////////////////


//////////////////////////////////
// constructors and destructors //
//////////////////////////////////

irc_user *irc_user_new() {
	irc_user * self;

	self = malloc(sizeof(irc_user));
	self->nick = NULL;
	self->hostname = NULL;
	self->server = NULL;
	self->hostmask = NULL;
	self->real_name = NULL;

	return self;
}

void irc_user_free(irc_user *self) {
	if (!self)
		return;
	
	free(self->nick);
	free(self->hostname);
	free(self->server);
	free(self->hostmask);
	free(self->real_name);

	free(self);
}

////////////////////////////
// other public functions //
////////////////////////////

void irc_user_set_hostmask(irc_user *self, const char hostmask[]) {
	char *string;
	cstring *tmp, *tmp2;
	int i;
	
	// hostmask -> string
	string = hostmask ? cstring_sclones(hostmask) : NULL;
	
	free(self->nick);
	free(self->hostname);
	free(self->server);
	free(self->hostmask);
			
	if (string) {
		// hostmask
		self->hostmask = string;

		// user -> tmp
		tmp = cstring_new();
		cstring_adds(tmp, hostmask);
		i = cstring_finds(tmp, "!", 0);
		cstring_cut_at(tmp, i);
		self->nick = cstring_convert(tmp);

		// hostname@server -> tmp
		tmp2 = cstring_new();
		cstring_adds(tmp2, hostmask);
		tmp = cstring_substring(tmp2, i + 1, 0);
		cstring_free(tmp2);

		// server -> tmp2
		i = cstring_finds(tmp, "@", 0);
		tmp2 = cstring_substring(tmp, i + 1, 0);
		self->server = cstring_convert(tmp2);

		// hostname -> tmp
		cstring_cut_at(tmp, i);
		self->hostname = cstring_convert(tmp);
	} else {
		self->nick = NULL;
		self->hostname = NULL;
		self->server = NULL;
		self->hostmask = NULL;
	}
}

void irc_user_set_user(irc_user *self, const char nick[], const char hostname[], const char server[]) {
	cstring *string;

	if (nick && hostname && server) {
		string = cstring_new();
		cstring_adds(string, nick);
		cstring_addc(string, '!');
		cstring_adds(string, hostname);
		cstring_addc(string, '@');
		cstring_adds(string, server);
	} else {
		string = NULL;
	}
	
	irc_user_set_hostmask(self, string ? string->string : NULL);
	
	cstring_free(string);
}

void irc_user_set_real_name(irc_user *self, const char real_name[]) {
	char *string;
	
	// real_name -> string
	string = (real_name ? cstring_sclones(real_name) : NULL);
	
	free(self->real_name);
	
	self->real_name = string;
}
