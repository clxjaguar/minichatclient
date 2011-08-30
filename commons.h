// nicklist structures common between irc.c, main.c and gotcurses.c

#ifndef COMMONS_H

// waiting time granolosity for pooling minichat server in milliseconds
#define WAITING_TIME_GRANOLOSITY 250

typedef struct {
	char *nickname;
	char *iconurl;
	char *profileurl;
} tnick;


#define COMMONS_H
#endif
