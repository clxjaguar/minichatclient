// Ce truc est une sorte de client http afin de servir depasserelle vers le minichat rmcgirr83.org pour phpbb.
// Rha, Mais pourquoi toujours utiliser les trucs les moins compatibles possibles ? Parce que c'est 'web'.
//
// Avec gcc sous windows, il faut passer -lws2_32 et "c:\Dev-Cpp\lib\libcurses.a" au
// linker (avec pdcurses-3.2-1mol.DevPak d'installé dans dev-c++ dans cet exemple).

// Licence: Au cas où ça se révélerait indispensable, la GPL ? ou alors CC-BY-NC ?
// Garantie: Aucune. Timmy, si quelqu'un crashe ton serveur avec ce truc, c'est pas notre faute ! ^^

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#ifdef WIN32
	#include <winsock2.h>
#else
	#include <sys/socket.h>
	#define Sleep(s) usleep(s*1000)
	#define closesocket(s); close(s);
#endif

#include "main.h"
#include "entities.h"
#include "cookies.h"
#include "network.h"
#include "parsehtml.h"
#include "conf.h"
#include "nicklist.h" //nicklist_msg_update(), nicklist_init(), nicklist_destroy()
#include "commons.h"
#include "display_interfaces.h"
#include "strfunctions.h"
#include "mccirc.h"

#define LOGIN_PAGE "ucp.php?mode=login"
#define MCHAT_PAGE "mchat.php"

// have to be large enough to contain the http headers
#define BUFSIZE 800

// hééééé oui, encore une machine à états ! ^^
typedef enum {
	LOADING_LOGIN_PAGE = 0,
	SUBMIT_AUTHENTIFICATION,
	GET_THE_BACKLOG,
	WATCHING_NEW_MESSAGES,
	RETRIEVING_THE_LIST_OF_USERS,
	POSTING_A_MESSAGE,
	WAIT
} tstate;

// quelques variables globales
tstate state;
FILE *logfile;
char *host = NULL;
char *port = NULL;
char *path = NULL;

mccirc *irc = NULL;

mccirc *get_mccirc(void){
	return irc;
}

void put_timestamp(FILE *f){
	struct tm *ptm;
	time_t lt;

	lt = time(NULL);
	ptm = localtime(&lt);

	if (f == NULL) { return; }

	if (state == GET_THE_BACKLOG){
		if (f != stdout) {
			fprintf(f, "[    BACK-LOG    ] "); //4+1+2+1+2+1+2+1+2 = 16
		}
		else {
			fprintf(f, "[BKLOG] ");
		}
	}
	else {
		if (f != stdout) {
			fprintf(f, "[%04u-%02u-%02u", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday);
			fprintf(f, " %02u:%02u] ", ptm->tm_hour, ptm->tm_min);
		}
		else {
			fprintf(f, "[%02u:%02u] ", ptm->tm_hour, ptm->tm_min);
		}
	}
}

char *malloc_globalise_url(const char *url){
	char *output;
	if (!url) { return NULL; }
	if (url[0] == '.' && url[1] == '/' && host && path) {
		if (port && port[0] && strcmp(port, "80")) {
			return mconcat6("http://", host, ":", port, path, &url[2]);
		}
		else {
			return mconcat4("http://", host, path, &url[2]);
		}
	}
	else {
		output = malloc(strlen(url)+1);
		if (output) { strcpy(output, url); }
		return output;
	}
}

void minichat_message(const char *username, const char *message, const char *usericonurl, const char *userprofileurl) {
	char *p = NULL;

	// display the message
	p = mconcat4("<", username, "> ", message);
	if (p) {
		display_conversation(p);
		free(p); p = NULL; // mconcatN does malloc()
	}

	// and put it in the log file
	put_timestamp(logfile); fprintf(logfile, "<%s> %s\r\n", username, message); fflush(logfile);

	// update nicklist things...
	if (state != GET_THE_BACKLOG){
		nicklist_msg_update(username, userprofileurl, usericonurl);
	}

	// envoie le message vers le client IRC (s'il y en a un) via "mccirc".
	mccirc_chatserver_message(irc, username, message);
}

// the following routine is showing that "HTTP/1.1 200 OK" message
int check_http_response(char *buf, ssize_t bytes){
	unsigned int i; int response=0; char tmp, *p=NULL;

	if (bytes <= 0) {
		display_debug("No data ?", 0);
		return -1;
	}
	for(i=0; i<(unsigned int)bytes; i++){
		if (!p && buf[i] == ' '){
			p = &buf[i+1];
		}
		else if (buf[i] == '\r' || buf[i] == '\n'){
			tmp = buf[i];
			buf[i] = '\0';
			if (p) {
				response = atoi(p);
			}
			display_debug(buf, 0);
			buf[i] = tmp;
			break;
		}
	}
	switch(response){
		case 200:
			display_debug("", 1);
			return 0; // no error

		case 403:
			display_debug(" [Do not want]", 1);
			return 403;

		case 404:
			display_debug(" [Page not found]", 1);
			return 404;

		default:
			display_debug(" [error] ", 1);
			return -2;
	}
}

int exit_requested;
int poll_requested;
#ifdef __linux__
#include <signal.h>

static void sigkilled(int sig){
	switch(sig){
		case SIGUSR1:
			display_statusbar("Polling forced by SIGUSR1 signal...");
			poll_requested = 1;
			return;
			break;

		case SIGINT:
			if (!exit_requested) {
				display_statusbar("Got SIGINT (^C), exiting...");
				exit_requested = 1;
				return;
			}
			display_statusbar("Got SIGINT twice, dying now :(");
			break;

		case SIGTERM:
			display_statusbar("Got SIGTERM, exiting...");
			exit_requested = 1;
			return;
			break;

		case SIGSEGV:
			display_statusbar("Got SIGSEGV, dying..");
			exit_requested = 1;
			break;

		case SIGABRT:
			display_statusbar("Got SIGABRT, dying..");
			exit_requested = 1;
			break;

		case SIGQUIT:
			display_statusbar("Got SIGQUIT, dying..");
			exit_requested = 1;
			break;

		default:
			break;
	}

	signal(sig, SIG_DFL);
	raise(sig);
}

int install_sighandlers(void){
	signal(SIGUSR1, sigkilled);
	signal(SIGTERM, sigkilled);
	signal(SIGINT, sigkilled);
	//signal(SIGSEGV, sigkilled); //segfault!
	signal(SIGQUIT, sigkilled);
	signal(SIGABRT, sigkilled);
	return 0;
}

#else
int install_sighandlers(){
	return -1;
}
#endif

void force_polling(void){
	poll_requested=1;
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////// ENTRY POINT /////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

int main(void) {
	int s; //socket descriptor
	char buf[BUFSIZE+1]; // rx buffer
	ssize_t bytes;
	int k, response; // flag for any use
	const char *outgoingmsg = NULL;

	unsigned int t; // timeslots remaining before next polling
	unsigned int nberr = 0;
	unsigned int wait_time = 40; // 10s
	unsigned int wait_time_maxi, wait_time_mini, wait_time_awake;

	tstate oldstate=LOADING_LOGIN_PAGE, futurestate=WAIT;
	char *useragent = NULL;

	cookie_t cookies[MAXCOOKIES]; // se configure dans cookies.h
	message_t msg;

	memset(&cookies[0], 0, sizeof(cookies));
	memset(&msg,        0, sizeof(msg));

	display_init();
	install_sighandlers();
	nicklist_init();
	exit_requested = 0;
	poll_requested = 0;

	display_conversation(
	  "********************************************\n"
	  "**     Welcome to minichatclient for      **\n"
	  "**     rmcgirr83.org's phpBB's addon      **\n"
	  "** http://minichatclient.sourceforge.net/ **\n"
	  "********************************************\n\n"
	);

	logfile = fopen("output.log", "a");
	if (!logfile){
		display_debug("Can't open output.log for writing !", 0);
		display_waitforchar("Press any key to continue");
		return -1;
	}

	ws_init();

	/* reading configuration file */
	host            =               read_conf_string("host",      host,      0);
	port            =               read_conf_string("port",      port,      0);
	path            =               read_conf_string("path",      path,      0);
	useragent       =               read_conf_string("useragent", useragent, 0);
	wait_time_maxi  = (unsigned int)read_conf_int   ("wait_time_maxi",       15) * (1000/WAITING_TIME_GRANOLOSITY);
	wait_time_mini  = (unsigned int)read_conf_int   ("wait_time_mini",       5)  * (1000/WAITING_TIME_GRANOLOSITY);
	wait_time_awake = (unsigned int)read_conf_int   ("wait_time_awake",      3)  * (1000/WAITING_TIME_GRANOLOSITY);

	{
		char buf2show[200];
		snprintf(buf2show, 200, "Timmings: maxi=%0.2fs / mini=%0.2fs / awake=%0.2fs\n", (float)wait_time_maxi/(1000/WAITING_TIME_GRANOLOSITY), (float)wait_time_mini/(1000/WAITING_TIME_GRANOLOSITY), (float)wait_time_awake/(1000/WAITING_TIME_GRANOLOSITY));
		display_debug(buf2show, 0);
	}

	if (!host || !path || !useragent){
		display_debug("Error: Server informations missing. Please edit your mchatclient.conf file !", 0);
		display_waitforchar("Press any key to quit");
		fclose(logfile);
		ws_cleanup();
		display_end();
		return -1;
	}

	if (read_conf_int("read_parser_rules", 0)){
		if(parser_loadrules()){
			display_conversation("Warning: Unable to load the parser rules. They will not be used.");
		}
	}

	{ // initialize the mini IRC server
		char *username     = read_conf_string("username", NULL, 0); // 0 means: do the malloc if found !
		int irc_port       = read_conf_int   ("irc_port", 0); // default value : 0
		char *channel_name = read_conf_string("channel_name", NULL, 0); // 0 means: do the malloc if found !
		int irc_topic_mode = read_conf_int   ("irc_topic_mode", 1); // default value : 1 (send once at join, not at changes)

		if (username) {
			if (irc_port) {
				display_debug("IRC: initialyzing, using channel name: ", 0);
				display_debug(channel_name?channel_name:"#MiniChatDefaultName", 1);
				irc = mccirc_new();
				if (!irc) { display_debug("IRC: ERROR! mccirc_new() returned NULL. IRC support not compiled in ?", 0); }
				mccirc_init(irc, username, "minichatclient.sourceforge.net", channel_name?channel_name:"#MiniChatDefaultName", NULL, irc_port);
				mccirc_set_topic_mode(irc, irc_topic_mode);
			}
			free(username); username = NULL;
		}
		if (channel_name) { free(channel_name); channel_name = NULL; }
	}

	{ // do a little pause
		unsigned int i;
		for (i=0; i<1200; i+=WAITING_TIME_GRANOLOSITY){ display_driver(); }
	}


	state = LOADING_LOGIN_PAGE;
	while(!exit_requested){
		if (state != WAIT) {
			// on se connecte sur le serveur pour tout les cas sauf attentes
			s = maketcpconnexion(host, port);
			if (!s) { //
				nberr++;
				if (nberr == 5) {
					put_timestamp(logfile);
					fprintf(logfile, "Unable to connect to the server anymore !\r\n");
					fflush(logfile);
					mccirc_chatserver_error(irc);
				}
				wait_time = 10 * (1000/WAITING_TIME_GRANOLOSITY);
				futurestate = state;
				state = WAIT;
			}
			else {
				if (nberr >= 5) {
					put_timestamp(logfile);
					fprintf(logfile, "The server seems to be back now !\r\n");
					fflush(logfile);
					mccirc_chatserver_resume(irc);
					if (nberr >= 30) { // 5' ? reconnect from beginning.
						state = LOADING_LOGIN_PAGE;
					}
				}
				nberr = 0;
			}
		}

		// now here is the main finite state machine
		switch(state){
			default:
			case LOADING_LOGIN_PAGE:
				// premi�re �tape, on se connecte sur la page de login pour aller chercher un sid
				// (attention, il ne va fonctionner qu'avec l'user-agent sp�cifi�, faut plus le changer !)
				{
					char *req = mconcat2(path, LOGIN_PAGE);
					http_get(s, req, host, NULL, NULL, useragent, NULL);
					free(req); req=NULL;
				}
				k=1;
				while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
					if(k) {
						check_http_response(buf, bytes);
						parsehttpheadersforgettingcookies(cookies, buf, bytes);
					}
					k=0;
				}
				state = WAIT;
				wait_time = 1;
				futurestate = SUBMIT_AUTHENTIFICATION;
				break;

			case SUBMIT_AUTHENTIFICATION:
				// on s'identifie sur cette même page.
				// génération de ce que l'en va envoyer en POST pour se logger
				{
					char *req        = NULL;
					char *postdata   = NULL;
					char *referer    = NULL;
					char *cookiesstr = NULL;
					char *username = read_conf_string("username", NULL, 0);
					char *password = read_conf_string("password", NULL, 0);
					close_conf_file();

					if (!username || !password) {
						if (username) { free(username); username=NULL; }
						if (password) {
							memset(password, 0, strlen(password)); // overwrite password stored in RAM for security reasons.
							free(password); password=NULL;
						}
						display_debug("Username/password informations missing or incomplete, skipping authentification. Tying to switch to the reading states though.", 0);
						state = GET_THE_BACKLOG;
						break;
					}

					req = mconcat2(path, LOGIN_PAGE);
					postdata = mconcat5("username=", username, "&password=", password, "&redirect=index.php&login=Connexion");
					memset(password, 0, strlen(password)); // overwrite password stored in RAM for security reasons.
					free(username); username=NULL;
					free(password); password=NULL;

					referer = mconcat4("http://", host, path, LOGIN_PAGE);
					cookiesstr = generate_cookies_string(cookies, NULL, 0);

					http_post(s, req, host, postdata, referer, cookiesstr, useragent, NULL);

					free(req);        req=NULL;
					free(postdata);   postdata=NULL;
					free(referer);    referer=NULL;
					free(cookiesstr); cookiesstr=NULL;
				}
				k=1;
				while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
					if(k) {
						check_http_response(buf, bytes);
						parsehttpheadersforgettingcookies(cookies, buf, bytes);
					}
					k=0;
				}
				state = WAIT;
				wait_time = 1;
				futurestate = GET_THE_BACKLOG;
				break;

			case GET_THE_BACKLOG:
				// ça, c'est pour récupérer le texte de la conversation déjà écrite comme le fait le navigateur
				{
					char *req = mconcat2(path, MCHAT_PAGE);
					char *referer = mconcat4("http://", host, path, LOGIN_PAGE);
					char *cookiesstr = generate_cookies_string(cookies, NULL, 0); // does the malloc

					http_get(s, req, host, referer, cookiesstr, useragent, NULL);

					free(req);        req=NULL;
					free(referer);    referer=NULL;
					free(cookiesstr); cookiesstr=NULL;
				}
				k=1;
				while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
					if(k) {
						response = check_http_response(buf, bytes);
						parsehttpheadersforgettingcookies(cookies, buf, bytes);
					}
					parse_minichat_mess(buf, bytes, &msg, k);
					k=0;
				}
				futurestate = WATCHING_NEW_MESSAGES;
				wait_time = 10*(1000/WAITING_TIME_GRANOLOSITY); state = WAIT;
				if (response==403){
					futurestate = LOADING_LOGIN_PAGE;
				}
				break;

			case WATCHING_NEW_MESSAGES:
				// ... et ça, c'est pour récupérer ce qui s'y passe !
				// => donner l'id du dernier message reçu
				{
					char *req = mconcat2(path, MCHAT_PAGE);
					char *postdata = mconcat2("mode=read&message_last_id=", msg.msgid);
					char *referer = mconcat4("http://", host, path, MCHAT_PAGE);
					char *cookiesstr = generate_cookies_string(cookies, NULL, 0);

					http_post(s, req, host, postdata, referer, cookiesstr, useragent, NULL);

					free(req);        req=NULL;
					free(postdata);   postdata=NULL;
					free(referer);    referer=NULL;
					free(cookiesstr); cookiesstr=NULL;
				}
				{
					unsigned int nbmessages = 0, old_wait_time;

					k=1;
					while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
						if(k) {
							response = check_http_response(buf, bytes);
							parsehttpheadersforgettingcookies(cookies, buf, bytes);
						}
						nbmessages = parse_minichat_mess(buf, bytes, &msg, k);
						k=0;
					}
					if (response==403){
						wait_time = 10*(1000/WAITING_TIME_GRANOLOSITY); state = WAIT;
						futurestate = LOADING_LOGIN_PAGE;
						break;
					}

					old_wait_time = wait_time;
					if (nbmessages == 0) {
						wait_time = wait_time + (wait_time>>1);
						if (wait_time>wait_time_maxi) { wait_time = wait_time_maxi; }
					}
					else {
						wait_time/=(nbmessages+1);
						if (wait_time<wait_time_mini) {
							wait_time = wait_time_mini;
							if (old_wait_time < wait_time) { wait_time = old_wait_time; }
						}
					}
				}
				futurestate = WATCHING_NEW_MESSAGES;
				state = WAIT;
				{
					static int u = 0;
					if (u++ > 4) {
						u=0;
						futurestate = RETRIEVING_THE_LIST_OF_USERS;
					}
				}
				break;

			case RETRIEVING_THE_LIST_OF_USERS:
				// de temps en temps, on peut regarder qui est là.
				{
					char *req = mconcat2(path, MCHAT_PAGE);
					char *referer = mconcat4("http://", host, path, MCHAT_PAGE);
					char *cookiesstr = generate_cookies_string(cookies, NULL, 0);

					http_post(s, req, host, "mode=stats", referer, cookiesstr, useragent, NULL);

					free(req);        req=NULL;
					free(referer);    referer=NULL;
					free(cookiesstr); cookiesstr=NULL;
				}
				k=1;
				{
					while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
						if(k) {
							check_http_response(buf, bytes);
							parsehttpheadersforgettingcookies(cookies, buf, bytes);
						}
						parse_minichat_mess(buf, bytes, &msg, k);
						k=0;
					}
				}
				state = WATCHING_NEW_MESSAGES;
				break;

#define POSTDATALEFT   "mode=add&message="
#define POSTDATARIGHT  "&helpbox=Tip%3A+Styles+can+be+applied+quickly+to+selected+text.&addbbcode20=100&addbbcode_custom=%23"

			case POSTING_A_MESSAGE:
				// et enfin, ça, c'est pour y poster quelque chose. faire gaffe de ne pas flooder sinon Timmy va se fâcher.
				if (!outgoingmsg){
					display_debug("Error: doing a posting cycle but there is no outgoing message to send.", 0);
					state = WATCHING_NEW_MESSAGES;
					break;
				}

				{
					char *req        = NULL;
					char *postdata   = NULL;
					char *referer    = NULL;
					char *cookiesstr = NULL;
					char *tmp = NULL; // outgoingmsg don't have to be free()'d.

					// replace some charactersdisturbing the POST string...
					strrep(outgoingmsg, &tmp, "%", "%25");
					strrep(NULL,        &tmp, "+", "%2B");
					strrep(NULL,        &tmp, "&", "%26");
					strrep(NULL,        &tmp, "=", "%3D");
					strrep(NULL,        &tmp, " ", "+");

					// mode=add&message=TESTMSG&helpbox=Tip%3A+Styles+can+be+applied+quickly+to+selected+text.&addbbcode20=100&addbbcode_custom=%23

					req = mconcat2(path, MCHAT_PAGE);
					postdata = mconcat3(POSTDATALEFT, tmp, POSTDATARIGHT);
					referer = mconcat4("http://", host, path, MCHAT_PAGE);
					storecookie(cookies, "mChatShowUserList", "yes");
					cookiesstr = generate_cookies_string(cookies, NULL, 0);

					http_post(s, req, host, postdata, referer, cookiesstr, useragent, NULL);

					outgoingmsg = NULL; // don't be afraid, a copy of this buffer is keep internally in the function providing it

					free(req);        req=NULL;
					free(referer);    referer=NULL;
					free(postdata);	  postdata=NULL;
					free(cookiesstr); cookiesstr=NULL;
					free(tmp);        tmp=NULL;
				}
				k=1;
				while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
					if(k) {
						response = check_http_response(buf, bytes);
						parsehttpheadersforgettingcookies(cookies, buf, bytes);
					}
					k=0;
				}
				wait_time = wait_time_awake;
				state = WATCHING_NEW_MESSAGES; // le changement d'etat est important ;)
				break;

			case WAIT:
				// on attends un peu entre chaque refresh pour ne pas saturer le serveur
				if (state != oldstate) { // timer init.
					t = wait_time;
					display_debug("Waiting......", 0);
					oldstate = state;
				}
				if (t){{ // timer avec du temps restant
					const char anim[4] = {'\\', '-', '/', '|'};
					char buf2show[15];
					t--;
					snprintf(buf2show, 15, "\b\b\b\b\b\b%3us %c", (unsigned int)(t/(1000/WAITING_TIME_GRANOLOSITY)), anim[t%4]); 
					display_debug(buf2show, 1);
				}}
				else { // temps d'attente termine
					display_debug("\b\b\b\b\b\b\b\b\b\b\b\b\b             \b\b\b\b\b\b\b\b\b\b\b\b\b", 1);
					state = futurestate;
				}

				if (poll_requested){
					poll_requested=0;
					wait_time=7*(1000/WAITING_TIME_GRANOLOSITY);
					state = RETRIEVING_THE_LIST_OF_USERS;
				}

				// the check for keyboard inputs (embedded) does the timebase
				// eg. like "Sleep(WAITING_TIME_GRANOLOSITY)")
				outgoingmsg = display_driver();

				// and now we check for a new message in the IRC interface
				if (!outgoingmsg) { outgoingmsg = mccirc_check_message(irc); }

				// if we have something to send, we change the state of the state machine.
				if (outgoingmsg) { state = POSTING_A_MESSAGE; }
				oldstate = state; // this is important to know when reset the waiting time.
				break;
		}

		// if a TCP connexion to the server is present, terminate it !
		if (s) { closesocket(s); s = 0; }
	} // MAIN LOOP END

	// end of program requested. closing everything now.
	parser_freerules();
	parse_minichat_mess(NULL, 0, &msg, 1);
	nicklist_destroy();
	mccirc_free(irc);
	freecookies(cookies);
	fclose(logfile);
	ws_cleanup();
	if (host)      { free(host);      host=NULL; }
	if (path)      { free(path);      path=NULL; }
	if (useragent) { free(useragent); useragent=NULL; }
	display_end();
	return 0;
}
