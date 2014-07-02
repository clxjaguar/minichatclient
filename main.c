// Ce truc est une sorte de client http afin de servir de passerelle vers le minichat rmcgirr83.org pour phpbb.
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
#include "ircserver.h"

#define LOGIN_PAGE "ucp.php?mode=login"
#define MCHAT_PAGE "mchat.php"

// have to be large enough to contain the http headers
#define BUFSIZE 800

#define FREE(x) if (x) { free(x); x=NULL; }
#define COPY(x, y) if (x) { free(x); } x = malloc(strlen(y)+1); if (x) { strcpy(x, y); }

// hééééé oui, encore une machine à états ! ^^
typedef enum {
	WAIT_LOADING_LOGIN_PAGE = 0,
	     LOADING_LOGIN_PAGE,
	WAIT_SUBMIT_AUTHENTIFICATION,
	     SUBMIT_AUTHENTIFICATION,
	WAIT_GET_THE_BACKLOG,
	     GET_THE_BACKLOG,
	WAIT_WATCHING_NEW_MESSAGES,
	     WATCHING_NEW_MESSAGES,
	WAIT_RETRIEVING_THE_LIST_OF_USERS,
	     RETRIEVING_THE_LIST_OF_USERS,
	WAIT_POSTING_A_MESSAGE,
	     POSTING_A_MESSAGE
} tstate;

// quelques variables globales
tstate state;
FILE *logfile;
char *host = NULL;
char *port = NULL;
char *path = NULL;

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
	display_conversation(p);
	FREE(p); // mconcatN does malloc()

	// and put it in the log file
	put_timestamp(logfile); fprintf(logfile, "<%s> %s\r\n", username, message); fflush(logfile);

	// update nicklist things...
	if (state != GET_THE_BACKLOG){
		nicklist_msg_update(username, userprofileurl, usericonurl);
	}
	// envoie le message vers l'eventuel client IRC connecté
	p = nicklist_alloc_ident(userprofileurl);
	irc_message(username, p, message);
	FREE(p);
}

// the following routine is showing that "HTTP/1.1 200 OK" message
int check_http_response(char *buf, ssize_t bytes){
	unsigned int i; int response=-1; char tmp, *p=NULL;

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
		case 0:
			return -1;

		case 200:
			return 0; // no error

		case 403:
			display_debug(" [Do not want]", 1);
			return 403;

		case 404:
			display_debug(" [Page not found]", 1);
			return 404;

		default:
			display_debug(" [Error] ", 1);
			return response;
	}
}

int exit_requested=0;
int poll_requested=0;
#ifndef WIN32
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
	int s=0; //socket descriptor
	char buf[BUFSIZE+1]; // rx buffer
	ssize_t bytes;
	int k, response; // flag for any use
	const char *outgoingmsg = NULL;
	unsigned int outgoingmsgretry=0;

	unsigned int t; // timeslots remaining before next polling
	unsigned int nberr = 0;
	unsigned int wait_time = 40; // 10s
	unsigned int wait_time_maxi, wait_time_mini, wait_time_awake;

	tstate oldstate=-1;
	char *useragent = NULL;

	cookie_t cookies[MAXCOOKIES]; // MAXCOOKIES is defined in cookies.h
	message_t msg;

	memset(&cookies[0], 0, sizeof(cookies));
	memset(&msg,        0, sizeof(msg));

	display_init();
	install_sighandlers();
	nicklist_init();
	//exit_requested = 0;
	//poll_requested = 0;

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
		char *username        = read_conf_string("username", NULL, 0); // 0 means: do the malloc if found !
		char *irc_port        = read_conf_string("irc_port", NULL, 0);
		char *irc_host        = read_conf_string("irc_host", NULL, 0);
		char *irc_fakehost    = read_conf_string("irc_fakehost", NULL, 0);
		char *channel_name    = read_conf_string("channel_name", NULL, 0);
		int irc_topic_mode    = read_conf_int   ("irc_topic_mode", 1); // default value : 1 (send once at join, not at changes)
		int irc_report_away   = read_conf_int   ("irc_report_away", 0); //inform the client that we're not active (away)

		if (username) {
			if (irc_port) {
				display_debug("IRC: initialyzing, using channel name: ", 0);
				display_debug(channel_name?channel_name:"#MCC", 1);

				if (!irc_init(irc_host?irc_host:"127.0.0.1", irc_port, irc_fakehost?irc_fakehost:"MCC", channel_name?channel_name:"#MCC", username)){
					display_debug("Warning: irc_init() failed !", 0);
				}
				else {
					irc_set_report_away(irc_report_away);
					irc_set_topic_mode(irc_topic_mode);
				}
			}
			FREE(username);
		}
		FREE(irc_port);
		FREE(irc_host);
		FREE(irc_fakehost);
		FREE(channel_name);
	}

	//{ // do a little pause
	//	unsigned int i;
	//	for (i=0; i<1200; i+=WAITING_TIME_GRANOLOSITY){ display_driver(); }
	//}


	wait_time = 2500/WAITING_TIME_GRANOLOSITY;
	state = WAIT_LOADING_LOGIN_PAGE;

	// MAIN LOOP START HERE //
	while(!exit_requested){
		// on se connecte sur le serveur dans tous les cas, sauf quand on doit juste attendre
		if ((state != WAIT_LOADING_LOGIN_PAGE) &&
		    (state != WAIT_SUBMIT_AUTHENTIFICATION) &&
		    (state != WAIT_GET_THE_BACKLOG) &&
		    (state != WAIT_WATCHING_NEW_MESSAGES) &&
		    (state != WAIT_RETRIEVING_THE_LIST_OF_USERS) &&
		    (state != WAIT_POSTING_A_MESSAGE)) {

			s = maketcpconnexion(host, port);
			if (!s) {
				nberr++;
				if (nberr == 5) {
					put_timestamp(logfile);
					fprintf(logfile, "Unable to connect to the server anymore !\r\n");
					fflush(logfile);
				}
				wait_time = 10 * (1000/WAITING_TIME_GRANOLOSITY);
				state--; // but waiting a bit before to retry in the same state.
			}
			else {
				if (nberr >= 5) {
					put_timestamp(logfile);
					fprintf(logfile, "The server seems to be back now !\r\n");
					fflush(logfile);
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
				// première étape, on se connecte sur la page de login pour aller chercher un sid
				// (attention, il ne va fonctionner qu'avec l'user-agent spécifié, faut plus le changer !)
				{
					char *req = mconcat2(path, LOGIN_PAGE);
					http_get(s, req, host, NULL, NULL, useragent, NULL);
					FREE(req);
				}
				k=1;
				while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
					if(k) {
						check_http_response(buf, bytes);
						parsehttpheadersforgettingcookies(cookies, buf, bytes);
					}
					k=0;
				}
				wait_time = 500/WAITING_TIME_GRANOLOSITY;
				state = WAIT_SUBMIT_AUTHENTIFICATION;
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
						if (password) {
							memset(password, 0, strlen(password)); // overwrite password stored in RAM for security reasons.
						}
						FREE(username); FREE(password);
						display_debug("Username/password informations missing or incomplete, skipping authentification. Tying to switch to the reading states though.", 0);
						state = GET_THE_BACKLOG;
						break;
					}

					req = mconcat2(path, LOGIN_PAGE);
					postdata = mconcat5("username=", username, "&password=", password, "&redirect=index.php&login=Connexion");
					memset(password, 0, strlen(password)); // overwrite password stored in RAM for security reasons.
					FREE(username); FREE(password);

					referer = mconcat4("http://", host, path, LOGIN_PAGE);
					cookiesstr = generate_cookies_string(cookies, NULL, 0);

					http_post(s, req, host, postdata, referer, cookiesstr, useragent, NULL);
					FREE(req); FREE(postdata); FREE(referer); FREE(cookiesstr);
				}
				k=1;
				while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
					if(k) {
						check_http_response(buf, bytes);
						parsehttpheadersforgettingcookies(cookies, buf, bytes);
					}
					k=0;
				}
				wait_time = 1000/WAITING_TIME_GRANOLOSITY;
				state = WAIT_GET_THE_BACKLOG;
				break;

			case GET_THE_BACKLOG:
				// ça, c'est pour récupérer le texte de la conversation déjà écrite comme le fait le navigateur,
				// mais c'est aussi très important pour récupérer les éléments de formulaire 'creation_time' et 'form_token'.
				{
					char *req = mconcat2(path, MCHAT_PAGE);
					char *referer = mconcat4("http://", host, path, LOGIN_PAGE);
					char *cookiesstr = generate_cookies_string(cookies, NULL, 0); // does the malloc

					http_get(s, req, host, referer, cookiesstr, useragent, NULL);
					FREE(req); FREE(referer); FREE(cookiesstr);
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
				if (response == 403){ // error ? retry.
					wait_time = 5000/WAITING_TIME_GRANOLOSITY;
					state = WAIT_LOADING_LOGIN_PAGE;
				}
				else {
					if (outgoingmsg){ // were we trying to send a message ?
						wait_time = 500/WAITING_TIME_GRANOLOSITY;
						state = WAIT_POSTING_A_MESSAGE;
					}
					else { // or just normal, successful login ?
						wait_time = 10*(1000/WAITING_TIME_GRANOLOSITY);
						state = WAIT_WATCHING_NEW_MESSAGES;
					}
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
					FREE(req); FREE(postdata); FREE(referer); FREE(cookiesstr);
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
						wait_time = 10*(1000/WAITING_TIME_GRANOLOSITY);
						state = WAIT_LOADING_LOGIN_PAGE;
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
				{
					static int u = 0;
					if (u++ > 4) {
						u=0;
						state = WAIT_RETRIEVING_THE_LIST_OF_USERS;
					}
					else {
						state = WAIT_WATCHING_NEW_MESSAGES;
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
					FREE(req); FREE(referer); FREE(cookiesstr);
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

					// mode=add&message=MSG&helpbox=Tip%3A+Styles+can+be+applied+quickly+to+selected+text.&addbbcode20=100&addbbcode_custom=%23
					// edit: changed to:
					// mode=add&message=MSG&helpbox=Tip%3A+Styles+can+be+applied+quickly+to+selected+text.&addbbcode20=100&addbbcode_custom=%23&creation_time=1403343659&form_token=6b47dbcb2931a7b070f7a55ba7479fbd66faf1cb

					req = mconcat2(path, MCHAT_PAGE);
					postdata = mconcat6("mode=add&message=", tmp, "&helpbox=Tip%3A+Styles+can+be+applied+quickly+to+selected+text.&addbbcode20=100&addbbcode_custom=%23&creation_time=", get_creation_time(), "&form_token=", get_form_token());
					referer = mconcat4("http://", host, path, MCHAT_PAGE);
					storecookie(cookies, "mChatShowUserList", "yes");
					cookiesstr = generate_cookies_string(cookies, NULL, 0);

					http_post(s, req, host, postdata, referer, cookiesstr, useragent, NULL);
					FREE(req); FREE(referer); FREE(postdata); FREE(cookiesstr); FREE(tmp);
				}
				k=1;
				while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
					if(k) {
						response = check_http_response(buf, bytes); // TODO: buf should be zero terminated!
						//   0 : OK (was 200 in fact)
						// 400 : Posted another line too fast, wait a bit.
						// 403 : Happen when i'm logged in for too long...
						// 501 : When sending strange characters like ISO-8859-1 accents and not UTF-8!
						//  -1 : No error code. Do not try to read the first line of buf.

						if (response) { // got error?
							if (outgoingmsgretry++<2){ // retrying...
								if (response == 400) {
									irc_message("RETRYING_TO_SEND", "minichatclient.sourceforge.net", buf);
									wait_time = 3000/WAITING_TIME_GRANOLOSITY;
									state = WAIT_POSTING_A_MESSAGE;
								}
								else if (response == 403) {
									irc_message("RETRYING_RELOG", "minichatclient.sourceforge.net", buf);
									wait_time = 500/WAITING_TIME_GRANOLOSITY;
									state = LOADING_LOGIN_PAGE;
								}
								else {
									if (response != -1) {
										irc_message("ERROR", "minichatclient.sourceforge.net", buf);
									}
									else {
										irc_message("ERROR", "minichatclient.sourceforge.net", "UNKNOW ERROR");
									}
									outgoingmsg = NULL;
									state = WATCHING_NEW_MESSAGES;
								}
							}
							else { // laissertombering...
								outgoingmsg = NULL;
								outgoingmsgretry=0;
								irc_message("TOO_MUCH_RETRY", "minichatclient.sourceforge.net", buf);
								wait_time = 3000/WAITING_TIME_GRANOLOSITY;
								state = WAIT_WATCHING_NEW_MESSAGES;
							}
						}
						else { // ok!
							outgoingmsg = NULL; // don't be afraid, a copy of this buffer is keep internally in the function providing it
							if (outgoingmsgretry) {
								outgoingmsgretry=0;
								irc_will_reprint_my_message();
							}
							wait_time = wait_time_awake;
							state = WATCHING_NEW_MESSAGES; // le changement d'etat est important ;)
						}
						parsehttpheadersforgettingcookies(cookies, buf, bytes);
					}
					k=0;
				}
				break;

			case WAIT_LOADING_LOGIN_PAGE:
			case WAIT_SUBMIT_AUTHENTIFICATION:
			case WAIT_GET_THE_BACKLOG:
			case WAIT_WATCHING_NEW_MESSAGES:
			case WAIT_RETRIEVING_THE_LIST_OF_USERS:
			case WAIT_POSTING_A_MESSAGE:
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
					state++; // FINISHED TO WAIT NOW, NEXT STATE !
				}

				if (poll_requested){
					poll_requested=0;
					wait_time=7*(1000/WAITING_TIME_GRANOLOSITY);
					state = RETRIEVING_THE_LIST_OF_USERS;
				}

				// the check for keyboard inputs (embedded) does the timebase
				// eg. like "Sleep(WAITING_TIME_GRANOLOSITY)")
				if (!outgoingmsg) { outgoingmsg = display_driver(); }
				else { Sleep(250); }

				// and now we check for a new message in the IRC interface
				if (!outgoingmsg) { outgoingmsg = irc_driver(); }

				// if we have something to send, we change the state of the state machine.
				if (outgoingmsg && state>=WAIT_WATCHING_NEW_MESSAGES && state!=WAIT_POSTING_A_MESSAGE) { 
					state = POSTING_A_MESSAGE;
				}
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
	irc_destroy();
	freecookies(cookies);
	fclose(logfile);
	ws_cleanup();
	FREE(host); FREE(port); FREE(path); FREE(useragent);
	display_end();
	return 0;
}
