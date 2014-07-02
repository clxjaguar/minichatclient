/*
	Name: ircserver.c (part of minichatclient)
	Author: cLx
	Date: March 2014
	New IRC miniserver module, rewrite based on niki's CIrc & CUtils but more simple and with less bugs
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>

#include "ircserver.h"
#include "nicklist.h"
#include "display_interfaces.h"
#include "strfunctions.h"

#ifdef WIN32
	#ifndef _WIN32_WINNT
		#define _WIN32_WINNT 0x501
	#endif
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#define close(s) closesocket(s)
	#define ioctl(a,b,c) ioctlsocket(a,b,(unsigned long*)c)
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <netdb.h>
	#include <arpa/inet.h>
	#include <sys/wait.h>
#endif

//#define DBG(x) perror(x)
#define DBG(x) display_debug(x, 0);
#define FREE(x) if (x) { free(x); x=NULL; }
#define COPY(x, y) if (x) { free(x); } x = malloc(strlen(y)+1); if (x) { strcpy(x, y); }

int irc_set_not_blocking(int sockfd){
	int flags;
	/* If they have O_NONBLOCK, use the POSIX way to do it */
#if defined (O_NONBLOCK)
    /* O_NONBLOCK is defined but broken on SunOS 4.1.x and AIX 3.2.5. */
    if ((flags = fcntl(sockfd, F_GETFL, 0)) == -1) {
        flags = 0;
    }
    return fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
#else
    flags = 1;
    return ioctl(sockfd, FIONBIO, &flags);
#endif
}

int irc_listen(const char *local_addr, const char *port){
	int sockfd;
	int yes = 1;
	struct addrinfo hints, *servinfo, *p;
	int rv;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	rv = getaddrinfo(local_addr, port, &hints, &servinfo);

	if (rv != 0) {
		DBG("getaddrinfo");
		//fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			DBG("socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&yes, sizeof(int)) == -1) {
			DBG("setsockopt");
			freeaddrinfo(servinfo);
			return -1;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			DBG("bind");
			continue;
		}
		irc_set_not_blocking(sockfd);
		break;
	}

	if (p == NULL) {
		DBG("failed to bind");
		freeaddrinfo(servinfo);
		return -1;
	}

	// all done with this structure
	freeaddrinfo(servinfo);

	if (listen(sockfd, 2) == -1) {
		DBG("listen");
		return -1;
	}
	/// BLOCKING
	return sockfd;
}

typedef enum {
	DISCONNECTED = 0,
	CONNECTED,
	IDENTIFIED,
	INCHANNEL,
} tclientstate;

typedef struct {
	int sfd;
	int cfd;
	struct sockaddr client_addr;
	socklen_t client_addr_len;
	char *fakehost;
	char *channel_name;
	char *forum_username;
	char *client_nickname;
	char *client_ident;
	char *last_topic_know;
	char *last_thing_i_said;
	int topic_reporting_mode;
	int report_away_to_client;
	time_t autorejoin;
	tclientstate clientstate;
} t_irc;
t_irc irc;

#define SERVER_PREFIX    "minichatclient.sourceforge.net",NULL,NULL
#define NO_PREFIX        NULL,NULL,NULL
#define CLIENT_PREFIX    irc.client_nickname,irc.client_ident,"me"

size_t irc_len(const char *string){
	size_t cnt=0;
	if (!string) { return 0; }
	while(string[cnt++]);
	return cnt;
}

const char* irc_trim(const char* in){
	static char *tmp = NULL;
	unsigned int n=0;

	if (tmp) { free(tmp); tmp=NULL; }
	if (!in) return NULL;
	for(;in[n];n++){
		if (!in[n]) { return in; }
		if (in[n]==' '){ break; }
	}
	tmp = malloc(strlen(in)+1);
	if (!tmp) { return in; }
	strcpy(tmp, in);
	while(tmp[n]){
		if (tmp[n] == ' '){ tmp[n] = '_'; }
		n++;
	}
	return tmp;
}

#define ARGCONCAT(arg) while(*arg!='\0' && *arg!='\r' && *arg!='\n'){ *p++=*arg++; } *p='\0';


void irc_sendtoclient(const char *prefix, const char *ident, const char *host, unsigned int lastarg, const char *arg1, const char *arg2, const char *arg3, const char *arg4, const char *arg5){
	char *string, *p;
	p = string = malloc(1+irc_len(prefix)+1+irc_len(ident)+1+irc_len(host)+2+irc_len(arg1)+1+irc_len(arg2)+1+irc_len(arg3)+1+irc_len(arg4)+1+irc_len(arg5)+3);
	if (!string) { return; }
	if (prefix){
		p = stpcpy(p, ":");
		p = stpcpy(p, prefix);
		if (ident&&host){
			p = stpcpy(p, "!");
			p = stpcpy(p, ident);
			p = stpcpy(p, "@");
			p = stpcpy(p, host);
		}
		p = stpcpy(p, " ");
	}
	if (arg1) {
		if (lastarg == 1) { p = stpcpy(p, ":"); }
		ARGCONCAT(arg1);
	}
	if (arg2) {
		p = stpcpy(p, " ");
		if (lastarg == 2) { p = stpcpy(p, ":"); }
		ARGCONCAT(arg2);
	}
	if (arg3) {
		p = stpcpy(p, " ");
		if (lastarg == 3) { p = stpcpy(p, ":"); }
		ARGCONCAT(arg3);
	}
	if (arg4) {
		p = stpcpy(p, " ");
		if (lastarg == 4) { p = stpcpy(p, ":"); }
		ARGCONCAT(arg4);
	}
	if (arg5) {
		p = stpcpy(p, " ");
		if (lastarg == 5) { p = stpcpy(p, ":"); }
		ARGCONCAT(arg5);
	}
	p = stpcpy(p, "\r\n");
	send(irc.cfd, string, strlen(string), 0);
	free(string);
}

void irc_send_conn_response_ok(void){
	irc_sendtoclient(SERVER_PREFIX, 3, "001", irc.client_nickname, "Welcome on the IRC side of MiniChatClient", NULL, NULL);
	irc_sendtoclient(SERVER_PREFIX, 3, "002", irc.client_nickname, "Your host is ... you!", NULL, NULL);
	irc_sendtoclient(SERVER_PREFIX, 3, "003", irc.client_nickname, "This server was compiled on ", __DATE__", "__TIME__, NULL);
	irc_sendtoclient(SERVER_PREFIX, 0, "004", irc.client_nickname, "minichatclient.sourceforge.net", NULL, NULL);
	irc_sendtoclient(SERVER_PREFIX, 3, "375", irc.client_nickname, "Message of the Day", NULL, NULL);
	irc_sendtoclient(SERVER_PREFIX, 3, "372", irc.client_nickname, "Less code, less bugs !", NULL, NULL);
	irc_sendtoclient(SERVER_PREFIX, 3, "376", irc.client_nickname, "End of MOTD", NULL, NULL);
}

const char* parse_buffer(char *string){
#define MAXARGS 50
	char *arg[MAXARGS+1];
	int cutting=1, n, nextarg=0;

	// lets begin with the cutting of the differents parts of the client's line
	n=0;
	arg[0] = NULL;
	while(string[n]){
		if (string[n] == ' '){
			if (!cutting){
				cutting=1;
				string[n] = '\0';
			}
		}
		else {
			if (cutting){
				cutting=0;
				arg[nextarg] = &string[n]; arg[nextarg+1] = NULL;
				if (string[n] == ':' && nextarg) { arg[nextarg]++; break; }
				if (string[n] != ':' || nextarg) { nextarg++; }
				if (nextarg>=MAXARGS){ break; }
			}
			if (nextarg==1){
				if (string[n] >= 'a' && string[n]<='z') {
					//string[n]-=((char)'a'-(char)'A');
					string[n]&=(char)0xDF; // capitalyze without -Wconversion warning !
				}
			}
		}
		n++;
	}

	// now, we'll have to check for recognized commands from the first part
	if (!arg[0]) { return NULL; }
	if (!strcmp(arg[0], "USER")) {
		if (!arg[1]) {
			irc_sendtoclient(SERVER_PREFIX, 3, "461", irc.client_nickname, "Not enough parameters", NULL, NULL);
		}
		else if (irc.clientstate == CONNECTED){
			COPY(irc.client_ident, arg[1]);
			if (irc.client_nickname){
				irc_send_conn_response_ok();
				irc.autorejoin = time(NULL) + 5;
				irc.clientstate = IDENTIFIED;
			}
		}
		else {
			irc_sendtoclient(SERVER_PREFIX, 3, "462", irc.client_nickname, "You may not reregister", NULL, NULL);
		}
		return NULL;
	}
	else if (!strcmp(arg[0], "NICK")) {
		if (!arg[1]) {
			irc_sendtoclient(SERVER_PREFIX, 3, "431", irc.client_nickname, "No nickname given", NULL, NULL);
			return NULL;
		}
		if (irc.clientstate == CONNECTED){
			COPY(irc.client_nickname, arg[1]);
			if (irc.client_ident){
				irc_send_conn_response_ok();
				irc.autorejoin = time(NULL) + 5;
				irc.clientstate = IDENTIFIED;
			}
		}
		else {
			irc_sendtoclient(CLIENT_PREFIX, 2, "NICK", arg[1], NULL, NULL, NULL);
			COPY(irc.client_nickname, arg[1]);
		}
		return NULL;
	}

	if (irc.clientstate < IDENTIFIED){
		irc_sendtoclient(SERVER_PREFIX, 3, "451", arg[0], "You have not registered", NULL, NULL);
	}
	else if (!strcmp(arg[0], "PRIVMSG")) {
		if (!arg[1] || !arg[2]) { return NULL; }
		if (!irc.channel_name) { return NULL; }
		if (!strcasecmp(arg[1], irc.channel_name)){
			if (irc.clientstate == INCHANNEL){
				if (arg[2][0]==0x01) { // ctcp to channel ?
					if (!strncmp(arg[2]+1, "ACTION", 6)){
						arg[2][7] = '*';
						arg[2][strlen(arg[2])-1] = '*';
						return &arg[2][7];
					}
				}
				return arg[2];
			}
		}
	}

	else if (!strcmp(arg[0], "ISON")) {{
		// okay, normally we have to use the people in argument.. i'll not
		// check two lists, i'm giving them all, the irc client will sort'em.
		char *tmp;
		tmp = nicklist_list_nicknames();
		if (tmp){
			irc_sendtoclient(SERVER_PREFIX, 3, "303", irc.client_nickname, tmp, NULL, NULL);
			free(tmp);
		}
	}}

	else if (!strcmp(arg[0], "PING")) {
		irc_sendtoclient(NO_PREFIX, 2, "PONG", arg[1], NULL, NULL, NULL);
	}
	else if (!strcmp(arg[0], "JOIN")) {
		if (!arg[1]) { return NULL; }
		if (!irc.channel_name) { return NULL; }
		if (!strcasecmp(arg[1], irc.channel_name)){
			if (irc.clientstate == IDENTIFIED) {{
				char *tmp;
				irc_sendtoclient(CLIENT_PREFIX, 2, "JOIN", irc.channel_name, NULL, NULL, NULL);
				irc.clientstate = INCHANNEL;
				tmp = nicklist_list_nicknames();
				if (tmp){
					irc_sendtoclient(SERVER_PREFIX, 5, "353", irc.client_nickname, "=", irc.channel_name, tmp);
					irc_sendtoclient(SERVER_PREFIX, 4, "366", irc.client_nickname, irc.channel_name, "End of /NAMES list", NULL);
					free(tmp);
				}
				if (irc.last_topic_know && irc.topic_reporting_mode){
					irc_sendtoclient(SERVER_PREFIX, 4, "332", irc.client_nickname, irc.channel_name, irc.last_topic_know, NULL);
				}
			}}
		}
	}
	else if (!strcmp(arg[0], "TOPIC")) {
		if (irc.last_topic_know){
			irc_sendtoclient(SERVER_PREFIX, 4, "332", irc.client_nickname, irc.channel_name, irc.last_topic_know, NULL);
		}
	}
	else if (!strcmp(arg[0], "WHOIS")) {{
		char *target;
		char *identinfos=NULL, *realname=NULL, *servinfos=NULL, *urls=NULL, *timesinfos=NULL; // <- will need to be freed later
		if (!arg[1]) {
			irc_sendtoclient(SERVER_PREFIX, 3, "431", irc.client_nickname, "No nickname given", NULL, NULL);
			return NULL;
		}
		if (arg[2]) { target = arg[2]; }
		else { target = arg[1]; }

		if (nicklist_get_infos_for_whois(target, irc.fakehost, &identinfos, &realname, &servinfos, &urls, &timesinfos)){
			irc_sendtoclient(SERVER_PREFIX, 4, "311", irc.client_nickname, identinfos, realname, NULL);
			irc_sendtoclient(SERVER_PREFIX, 4, "312", irc.client_nickname, servinfos, urls, NULL);
			irc_sendtoclient(SERVER_PREFIX, 4, "317", irc.client_nickname, timesinfos, "seconds idle, signon time", NULL);
			FREE(identinfos); FREE(realname); FREE(servinfos); FREE(urls); FREE(timesinfos);
		}
		else {
			irc_sendtoclient(SERVER_PREFIX, 4, "401", irc.client_nickname, target, "No such nick/channel", NULL);
		}
		irc_sendtoclient(SERVER_PREFIX, 4, "318", irc.client_nickname, target, "End of /WHOIS list.", NULL);
	}}
	else if (!strcmp(arg[0], "WHO")) {{
		// TODO
	}}
	else if (!strcmp(arg[0], "USERHOST")) { //QUIET PLZ
	}
	else if (!strcmp(arg[0], "MODE")) { //TODO ?
	}
	else if (!strcmp(arg[0], "NAMES")) {{
		char *tmp = nicklist_list_nicknames();
		if (tmp){
			irc_sendtoclient(SERVER_PREFIX, 4, "353", irc.client_nickname, irc.channel_name, tmp, NULL);
			free(tmp);
		}
		irc_sendtoclient(SERVER_PREFIX, 4, "366", irc.client_nickname, irc.channel_name, "End of /NAMES list", NULL);
	}}

	else if (!strcmp(arg[0], "PART")) {
		if (!arg[1]) { return NULL; }
		if (!irc.channel_name) { return NULL; }
		if (!strcasecmp(arg[1], irc.channel_name)){
			if (irc.clientstate == INCHANNEL) {
				irc_sendtoclient(CLIENT_PREFIX, 2, "PART", irc.channel_name, NULL, NULL, NULL);
				irc.clientstate = IDENTIFIED;
			}
		}
	}
	else if (!strcmp(arg[0], "AWAY")) {
		if (arg[1]){
			irc_sendtoclient(SERVER_PREFIX, 3, "306", irc.client_nickname, "You have been marked as being away", NULL, NULL);
		}
		else {
			irc_sendtoclient(SERVER_PREFIX, 3, "305", irc.client_nickname, "You are no longer marked as being away", NULL, NULL);
		}
	}
	else if (!strcmp(arg[0], "QUIT")) {
		irc_sendtoclient(NO_PREFIX, 2, "ERROR", "Closing Link: (Quit:)", NULL, NULL, NULL);
		close(irc.cfd); irc.cfd=0;
		irc.clientstate = DISCONNECTED;
	}
	else {
		irc_sendtoclient(SERVER_PREFIX, 4, "421", irc.client_nickname, arg[0], "Unknown command", NULL);
	}
	return NULL;
}

int irc_init(const char *host, const char *port, const char *fakehost, const char *channel_name, const char *forum_username){
#if defined (WIN32)
	WSADATA WSAData;
#endif

	memset(&irc, 0, sizeof(irc));
	irc.topic_reporting_mode = 1;
	if (!channel_name   || !channel_name[0])   { return 0; }
	if (!forum_username || !forum_username[0]) { return 0; }
	if (!fakehost       || !fakehost[0])       { return 0; }

#if defined (WIN32)
	if (WSAStartup(MAKEWORD(2,2), &WSAData))   { return 0; }
#endif

	irc.sfd = irc_listen(host, port);
	if (irc.sfd == -1){
		irc.sfd = 0;
		return 0;
	}
	irc.client_addr_len = sizeof(irc.client_addr);
	COPY(irc.channel_name, channel_name);
	COPY(irc.forum_username, forum_username);
	COPY(irc.fakehost, fakehost);
	return irc.sfd;
}

int irc_destroy(void){
	irc_trim(NULL);
	if (irc.clientstate >= CONNECTED){
		irc_sendtoclient(NO_PREFIX, 2, "ERROR", "Closing Link: MiniChatClient is exiting, bye bye !", NULL, NULL, NULL);
	}
	if (irc.cfd){ close(irc.cfd); irc.cfd=0; }
	if (irc.sfd){ close(irc.sfd); irc.sfd=0; }
	FREE(irc.fakehost);
	FREE(irc.channel_name);
	FREE(irc.forum_username);
	FREE(irc.client_nickname);
	FREE(irc.client_ident);
	FREE(irc.last_topic_know);
	FREE(irc.last_thing_i_said);
#if defined (WIN32)
    WSACleanup();
#endif
	memset(&irc, 0, sizeof(irc)); // helps Valgrind to be mad at us !
	return 0;
}

void irc_set_report_away(int value){
	irc.report_away_to_client = value;
}

void irc_join(const char *nickname, const char *ident){
	if (irc.forum_username && !strcasecmp(nickname, irc.forum_username)){
		if (irc.report_away_to_client) {
			irc_sendtoclient(SERVER_PREFIX, 3, "305", irc.client_nickname, "You are no longer marked as being away", NULL, NULL);
		}
		return;
	}
	if (irc.clientstate == INCHANNEL){
		irc_sendtoclient(irc_trim(nickname), ident, irc.fakehost, 2, "JOIN", irc.channel_name, NULL, NULL, NULL);
	}
}

void irc_part(const char *nickname, const char *ident, const char *partmsg){
	if (irc.forum_username && !strcasecmp(nickname, irc.forum_username)){
		if (irc.report_away_to_client) {
			irc_sendtoclient(SERVER_PREFIX, 3, "306", irc.client_nickname, "You have been marked as being away", NULL, NULL);
		}
		return;
	}
	if (irc.clientstate == INCHANNEL){
		irc_sendtoclient(irc_trim(nickname), ident, irc.fakehost, 3, "PART", irc.channel_name, partmsg, NULL, NULL);
	}
}

void irc_will_reprint_my_message(void){
	FREE(irc.last_thing_i_said);
}

void irc_message(const char *nickname, const char *ident, const char *message){
	if (irc.forum_username && nickname && !strcasecmp(nickname, irc.forum_username)){
		if (irc.last_thing_i_said && message){{
			char *str1=NULL, *str2=NULL;
			strrep(message, &str1, " ", "");
			strrep(irc.last_thing_i_said, &str2, " ", "");
			if (!strcasecmp(str1, str2)){
				FREE(str1); FREE(str2);
				FREE(irc.last_thing_i_said);
				return;
			}
			FREE(str1); FREE(str2);
		}}
	}

	if (irc.clientstate == INCHANNEL){
		irc_sendtoclient(irc_trim(nickname), ident, irc.fakehost, 3, "PRIVMSG", irc.channel_name, message, NULL, NULL);
	}
}

char topic_changed = 0;
void irc_topic(const char *topic){
	// si c'est pareil on fait rien du tout.
	if (irc.last_topic_know) {
		if (!strcasecmp(topic, irc.last_topic_know)){
			topic_changed=0;
			return;
		}
	}

	// pour les tordus qui activent cet insupportable "mode 2"
	if (irc.clientstate == INCHANNEL){
		if (irc.topic_reporting_mode == 2){
			irc_sendtoclient(irc.fakehost, NULL, NULL, 3, "TOPIC", irc.channel_name, topic, NULL, NULL);
		}
	}

	// dans tous les cas on memorise.
	COPY(irc.last_topic_know, topic);
	topic_changed=1;
}

void irc_topic_mode3_showtime(void){
	// le "mode 3", pour un compromis sur le topic, et affiché en dernier
	if (topic_changed && irc.clientstate == INCHANNEL){
		if (irc.topic_reporting_mode == 3){
			irc_sendtoclient(SERVER_PREFIX, 4, "332", irc.client_nickname, irc.channel_name, irc.last_topic_know, NULL);
		}
	}
	topic_changed=0;
}

void irc_set_topic_mode(int mode){
	if (mode>=0 && mode<=3) {
		irc.topic_reporting_mode = mode;
	}
}

const char* irc_driver(void){
	static char linebuffer[2000], inputbuf[500];
	int newcfd = 0;
	int i;
	static unsigned int o;
	ssize_t bytes = 0;
	const char *ret = NULL;

	if (!irc.sfd) { return NULL; }

	newcfd = accept(irc.sfd, &irc.client_addr, &irc.client_addr_len);
	if (newcfd != -1) {
		display_debug("Got new client connection to the IRC miniserver !", 0);
		if (irc.cfd){
			irc_sendtoclient(NO_PREFIX, 2, "ERROR", "Closing Link: Another incomming connection, ghosting that one...", NULL, NULL, NULL);
			close(irc.cfd); irc.cfd=0;
			irc.clientstate = DISCONNECTED;
		}
		FREE(irc.client_nickname);
		FREE(irc.client_ident);
		irc_set_not_blocking(newcfd);
		irc.cfd = newcfd;
		irc.clientstate = CONNECTED;
		o=0;
	}
	if (irc.clientstate == DISCONNECTED) { return NULL; }

	bytes = recv(irc.cfd, inputbuf, sizeof inputbuf, 0);
	if (bytes != -1){
		if (bytes == 0) {
			display_debug("IRC client closed connection.", 0);
			irc.clientstate = DISCONNECTED;
			close(irc.cfd); irc.cfd=0;
			return NULL;
		}
		for (i=0; i<bytes; i++){
			if (inputbuf[i] == '\r') { continue; }
			if (inputbuf[i] == '\n') {
				linebuffer[o] = '\0'; o=0;
				ret = parse_buffer(linebuffer);
				if (ret){
					COPY(irc.last_thing_i_said, ret);
				}
			}
			else {
                if (o < sizeof(linebuffer)){ linebuffer[o++] = inputbuf[i]; }
			}
		}
	}
	if (irc.autorejoin != 0) {
		if (time(NULL) >= irc.autorejoin) {
			if (irc.clientstate == IDENTIFIED && irc.channel_name) {{
				char *tmp;
				irc_sendtoclient(CLIENT_PREFIX, 2, "JOIN", irc.channel_name, NULL, NULL, NULL);
				irc.clientstate = INCHANNEL;
				tmp = nicklist_list_nicknames();
				if (tmp){
					irc_sendtoclient(SERVER_PREFIX, 5, "353", irc.client_nickname, "=", irc.channel_name, tmp);
					irc_sendtoclient(SERVER_PREFIX, 4, "366", irc.client_nickname, irc.channel_name, "End of /NAMES list", NULL);
					free(tmp);
				}
				if (irc.last_topic_know && irc.topic_reporting_mode){
					irc_sendtoclient(SERVER_PREFIX, 4, "332", irc.client_nickname, irc.channel_name, irc.last_topic_know, NULL);
				}
			}}
			irc.autorejoin = 0;
		}
	}
	return ret;
}


/*

#define IRCPORTTEST "6667"

#ifdef WIN32
#include <windows.h>
#define usleep(x) Sleep(x/1000)
#endif

int main(void){
	const char *p;
	int n=0;
	irc_init("0.0.0.0", IRCPORTTEST, "mcc", "#test", "MyNickname");
	system("netstat -an | grep :"IRCPORTTEST);
	if (!irc.sfd) { printf("init failed !\n"); }
	for(;;){
		//printf("."); fflush(stdout);
		p = irc_driver();
		if (p) { printf("[%s]\n", p); }
		if (n==10) { irc_join("Test", "fakeuser1"); }
		if (n==20) { irc_message("Test", "test", "This is a test message. I don't even exists."); }
		if (n==30) { irc_part("Test", "fakeuser1", NULL); }
		usleep(250000);
		n++;
		if (n>=40){n=0;}
	}
	irc_destroy();
	return 0;
}

*/
