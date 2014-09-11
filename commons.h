// nicklist structures common between irc.c, main.c and gotcurses.c

#ifndef COMMONS_H

// waiting time granolosity for pooling minichat server in milliseconds
#define WAITING_TIME_GRANOLOSITY 250

typedef struct {
	char *nickname;
	char *iconurl;
	char *profileurl;
} tnick;

typedef struct {
	char *username;
	char *message;
	char *usericonurl;
	char *userprofileurl;
	char msgid[30];
} message_t;

#define COMMONS_H
#endif
