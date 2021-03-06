/*
	Ce module doit gérer la liste des personnes présentes sur le minichat.

	<cLx> ça te dirait qu'on se fasse un module qui s'appelle nicklist.c, qui va prendre les appels de parsehtml.c, les requetes de whois de mccirc et qui va aussi fournir la nicklist pour gotcurses.c ?
	<niki> ça serait bien
	<cLx> btw, a chaque message, ça va appeler une fonction dans nicklist.c pour mettre à jour l'url de l'icone, du profil, le time du dernier message dit ...
	<cLx> si une personne est invisible mais parle quand même, on va quand même l'ajouter à la liste d'ailleurs avec un flag pour dire qu'elle est invisible et qu'il faudra compter sur un timeout depuis le dernier message pour la virer de la liste ou pas

	elements de la "base de donnees" :
		- nickname
		- l'url de son icône (récupérée quand la personne parle par msg->usericonurl)
		- l'url du profil (récupérée quand la personne parle par msg->userprofileurl)
		- une petite chaine pour y mettre un ident dedans (pour etre utilisée comme mask aprés)
		- un timestamp qui indique la date et l'heure d'ajout à la nicklist
		- un timestamp qui indique la date et l'heure du dernier message de la personne
		- un timestamp qui indique la date et l'heure de la dernière fois qu'on a vu la personne dans la liste de gens sur le minichat
		- un tag qui indique si la personne a été rajoutée à la nicklist sans qu'elle apparaisse dans la liste (ie: elle est invisible) ? En option, puisqu'il suffit de vérifier si le timestamp précédent est à 0 ou pas en fait...
*/

#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include <sys/queue.h>
#include "queue.h" //local copy as sys/queue.h isn't posix
#include "nicklist.h"
#include "display_interfaces.h" //display_debug()
#include "ircserver.h"
#include "main.h" //malloc_globalise_url()
#include "strfunctions.h"

#define FREE(t);     if(t) { free(t); t=NULL; }
#define COPY(d, s);  if(s) { FREE(d); d=malloc(strlen(s)+1); strcpy(d, s); }

typedef struct t_nicklist {
	char *nickname;
	char *ident;
	char *profile_url;
	char *icon_url;
	time_t added;
	time_t lastseen;
	time_t lastmessage;
	int invisible;
	int enterednow;
	STAILQ_ENTRY(t_nicklist) next;
} t_nicklist;
STAILQ_HEAD(,t_nicklist) head;

// program starting, init the list
void nicklist_init(void) {
	STAILQ_INIT(&head);
}

// when we close the program. bye!
void nicklist_destroy(void) {
	t_nicklist *np;

	// for each elements, delete the contents then the element itself
	while (!STAILQ_EMPTY(&head)) {
		np = STAILQ_FIRST(&head);
		FREE(np->nickname);
		FREE(np->profile_url);
		FREE(np->icon_url);
		FREE(np->ident);
		STAILQ_REMOVE_HEAD(&head, next);
		free(np);
	}

	// helps valgrind to be sad about the memory never freed ;)
	head.stqh_first=NULL; *(head.stqh_last)=NULL;
}

int nicklist_get_infos_for_whois(const char *target, const char *fakehost, char **identinfos, char **realname, char **servinfos, char **urls, char **timesinfos){
	t_nicklist *np;
	unsigned int u;
	char *nick = NULL, *tmp=NULL;
	time_t now = time(NULL);

	// search for the right element.
	STAILQ_FOREACH(np, &head, next) {
		if (np->nickname && !strcasecmp(target, np->nickname)){
			// found. replaces the username's spaces with underscores
			nick = malloc(strlen(np->nickname)+1);
			strcpy(nick, np->nickname);
			u=0;
			while(np->nickname[u]){
				if (np->nickname[u]==' ') { nick[u]='_'; }
				u++;
			}

			if (np->profile_url) { *realname = malloc_globalise_url(np->profile_url); }
			else { *realname = malloc(2); strcpy(*realname, "?"); }
			if (np->icon_url) { tmp = malloc_globalise_url(np->icon_url); }

			*identinfos = mconcat(6, nick, " ", np->ident?np->ident:"?", " ", fakehost, " *");
			*servinfos = mconcat(2, nick, " minichatclient.sourceforge.net");
			*urls = mconcat(4, np->nickname, np->enterednow?" [NEW]":" ", np->invisible?"[INVISIBLE] ":" ", tmp?tmp:"");
			FREE(tmp);

			// "cLx 3009 1395564105"
			{
				char s1[101];
				snprintf(s1, 100, "%ld %ld", np->lastmessage?now-np->lastmessage:now-np->added, np->added);
				*timesinfos = mconcat(3, nick, " ", s1);
			}
			FREE(nick);
			return 1;
		}
	}
	return 0;
}

char *nicklist_list_nicknames(void){
	t_nicklist *np;
	char *dest = NULL, *p, *tmp;
	unsigned int u;
	size_t len = 0;

	STAILQ_FOREACH(np, &head, next) {
		if (np->nickname){
			len+=strlen(np->nickname)+1;
		}
	}

	dest = malloc(len+1);
	if (!dest) { return NULL; }
	dest[0] = '\0';
	p = dest;
	STAILQ_FOREACH(np, &head, next) {
		if (np->nickname){
			tmp = malloc(strlen(np->nickname)+1);
			strcpy(tmp, np->nickname);
			u=0;
			while(np->nickname[u]){
				if (np->nickname[u]==' ') { tmp[u]='_'; }
				u++;
			}
			p = stpcpy(p, tmp);
			p = stpcpy(p, " ");
			free(tmp);
		}
	}
	return dest;
}

char* nicklist_alloc_ident(const char *profile_url){
	// extract a ident (mostly for IRC)
	char *p, *ident;
	p = strstr(profile_url, "&u=")+3;
	if (p) {
		ident=malloc(strlen(p)+1);
		strcpy(ident, p);
	}
	else {
		ident=malloc(2);
		strcpy(ident, "?");
	}
	return ident;
}

// 'nickname' talked. and now we know his profile and icon URLs.
void nicklist_msg_update(const char *nickname, const char *profile_url, const char *icon_url) {
	t_nicklist *np;
	time_t now = time(NULL);

	if (!nickname) { return; }
	STAILQ_FOREACH(np, &head, next) {
		if (!strcmp(nickname, np->nickname)) { // ok, found it
			np->enterednow = 0;
			np->lastseen = now;
			np->lastmessage = now;

			// copy the profile URL is it wasn't already memorized
			if (profile_url) {
				if (!np->profile_url) {
					np->profile_url = malloc(strlen(profile_url)+1);
					strcpy(np->profile_url, profile_url);
				}
			}

			// refresh icon URL
			if (icon_url) {
				if (np->icon_url){
					if (!strcmp(icon_url, np->icon_url)){
						return; // no need to update
					}
					free(np->icon_url);
				}
				np->icon_url = malloc(strlen(icon_url)+1);
				strcpy(np->icon_url, icon_url);
			}
			return;
		}
	}

	// ok, so it's a new entry !
	np = malloc(sizeof(t_nicklist));
	memset(np, 0, sizeof(t_nicklist));
	np->enterednow=1;
	np->invisible=1;
	np->added = now;
	np->lastseen = now;
	np->lastmessage = now;

	// copy nickname
	np->nickname = malloc(strlen(nickname)+1);
	strcpy(np->nickname, nickname);

	// copy profile URL
	if (profile_url) {
		np->profile_url = malloc(strlen(profile_url)+1);
		strcpy(np->profile_url, profile_url);
	}

	// copy ident
	np->ident = nicklist_alloc_ident(profile_url);

	// copy icon URL
	if (icon_url) {
		np->icon_url = malloc(strlen(icon_url)+1);
		strcpy(np->icon_url, icon_url);
	}
	STAILQ_INSERT_TAIL(&head, np, next);

	// update nicklists display (IRC and output plugin)
	display_nicklist(np->nickname);
	irc_join(np->nickname, np->ident);
}

// called when polling the list of people from the server
void nicklist_recup_start(void) {
	// some time ago we were clearing the display now
	// with a call of"display_nicklist(NULL);"
}

// called for each nickname found when polling the list from the server
void nicklist_recup_name(const char* nickname, const char* profile_url) {
	t_nicklist *np;
	time_t now = time(NULL);

	if (!nickname) { return; }
	STAILQ_FOREACH(np, &head, next) {
		if (!strcmp(nickname, np->nickname)) { // ok, found it
			np->enterednow=0;
			np->invisible=0;
			np->lastseen = now;

			// add profile URL ?
			if (profile_url && !np->profile_url) {
				np->profile_url = malloc(strlen(profile_url)+1);
				strcpy(np->profile_url, profile_url);
			}
			return;
		}
	}

	// ok, so it's a new entry !
	np = malloc(sizeof(t_nicklist));
	memset(np, 0, sizeof(t_nicklist));
	np->enterednow=1;
	np->invisible=0;
	np->added = now;
	np->lastseen = now;

	// copy nickname
	np->nickname = malloc(strlen(nickname)+1);
	strcpy(np->nickname, nickname);

	// copy profile URL
	if (profile_url) {
		np->profile_url = malloc(strlen(profile_url)+1);
		strcpy(np->profile_url, profile_url);
	}

	np->ident = nicklist_alloc_ident(profile_url);
	STAILQ_INSERT_TAIL(&head, np, next);
	irc_join(np->nickname, np->ident);
}

// called after we got the list from the server
void nicklist_recup_end(void) {
	t_nicklist *np, *np_temp;
	display_nicklist(NULL);
	time_t now = time(NULL);

	STAILQ_FOREACH_SAFE(np, &head, next, np_temp) {
		if ((np->invisible && now-np->lastseen > 20*60)
		 ||(!np->invisible && now-np->lastseen > 60)){
			irc_part(np->nickname, np->ident, np->invisible?"invisible and 20mn inactive":NULL);
			// remove nicklist element
			FREE(np->nickname);
			FREE(np->profile_url);
			FREE(np->icon_url);
			FREE(np->ident);
			STAILQ_REMOVE(&head, np, t_nicklist, next);
			free(np);
		}
		else {
			if (np->invisible) { np->enterednow=0; }
			display_nicklist(np->nickname);
		}
	}
	irc_topic_mode3_showtime();
}

// called when we get the topic from the server
void nicklist_topic(const char *string){
	irc_topic(string);
	display_statusbar(string);
}

void nicklist_showlist(void){
	t_nicklist *np;
	time_t now = time(NULL);
	char *tmp;

	display_conversation("");
	STAILQ_FOREACH(np, &head, next) {
		tmp = malloc(1000);
		snprintf(tmp, 1000, "* %s (%s)%s%s seen:%ds talked:%d%s added:%ds", np->nickname, np->ident?np->ident:"", np->invisible?" [INVISIBLE]":"", np->enterednow?" [NEW]":"", (int)(now-np->lastseen), np->lastmessage?(int)(now-np->lastmessage):0, np->lastmessage?"s":"", (int)(now-np->added));
		display_conversation(tmp);
		free(tmp);
		if (np->profile_url){
			tmp = malloc_globalise_url(np->profile_url);
			display_conversation(tmp);
			free(tmp);
		}
		if (np->icon_url){
			tmp = malloc_globalise_url(np->icon_url);
			display_conversation(tmp);
			free(tmp);
		}
	}
}
