// Ce truc est une sorte declient http afin de servir depasserelle vers le minichat rmcgirr83.org pour phpbb.
// pourquoi toujours utiliser les trucs les moins compatibles possibles ?
//
// Avec gcc sous windows, il faut passer -lws2_32 et "c:\Dev-Cpp\lib\libcurses.a" au 
// linker (avec pdcurses-3.2-1mol.DevPak d'installé dans dev-c++ dans cet exemple).
//
// Todo:
// [x] réseau, compatibilité windows, fonctions http de bases (get et post)
// [x] parser les cookies et remplir un tableau en les ajoutant/remplaçant selon le nom du cookie, et les renvoyer au serveur ensuite
// [x] décoder le html de la conversation (on veut le nom du compte de la personne, l'URL de l'icône du compte, l'ID du message, et bien sûr le texte)
// [x] sniffer la partie pour récupérer les personnes dans la conversation
// [x] et programmer le decodage qui va bien (dans la même fonction ça serait bien)
// [_] récuperer les gens qu'il y a dans le canal (sous quel format?)
// [_] ajouter le support de l'IPv6 (un jour viendra où l'on y sera obligé!)
// [x] parser les trucs entites HTML
// [_] gerer les erreurs genre on s'est fait "déconnecté" (sic) par le serveur (et tenter de se relogger)
// [x] parser le HTML présant quand il y a un lien, ou un quote, ou qu'on clique sur le '@'...
// [x] delai de polling adaptatif selon qu'il se passe des trucs ou non
// [x] interface curses (ou pas selon ce qu'on link, c'est modulaire!)
// [x] quand on répond, bien remplacer + par %2B
// [?] implémenter un serveur IRC pour un maximum de compatibilité avec un client d'IM ? ...
// [?] ... voire cette passerelle rendre multi-user pour qu'on puisse le mettre sur le serveur ? ...
// [?] ...ou faire un add-on pour pidgin ? ...

// Licence: Au cas où ça se révélerait indispensable, la GPL ? ou alors CC-BY-NC ?
// Garantie: Aucune. Timmy, si quelqu'un crashe ton serveur avec ce truc, c'est pas notre faute ! ^^

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#if defined (WIN32)
    #include <winsock2.h>
#else
    #include <sys/socket.h>
    #define Sleep(s) usleep(s*1000)
    #define closesocket(s); close(s);
#endif

#include "cookies.h"
#include "network.h"
#include "parsehtml.h"
#include "conf.h"
#include "commons.h"
#include "display_interfaces.h"
#include "strfunctions.h"

#define LOGIN_PAGE "ucp.php?mode=login"
#define MCHAT_PAGE "mchat.php"

// have to be large enough to contain the http headers
#define BUFSIZE 800

// hééé oui, encore une machine à états ! ^^
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
FILE *f;
char *host = NULL; unsigned int port = 0;
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

// ces fonctions sont appelées en retour par parsehtml.c
void minichat_message(char* username, char* message, char *usericonurl, char *userprofileurl){
    char *p = NULL;
        
    // display the message
    //put_timestamp(stdout);
    p = malloc(strlen(username)+strlen(message)+4); // "<> \0"
    strcpy(p, "<"); strcat(p, username); strcat(p, "> "); strcat(p, message);
    display_conversation(p);
    free(p); p = NULL;

	// and put it in the log file    
    //put_timestamp(f); fprintf(f, "<%s> %s\r\n", username, message); fflush(f); TODO
    
    // gere si la user icon est sur le serveur (avec une adresse relative ./)
    // (ne pas oublier d'alouer pour "http://", ":12345" et le \0 de fin de chaine)
    if (usericonurl[0] == '.' && usericonurl[1] == '/') {
        //p = malloc(strlen(host)+strlen(path)+strlen(usericonurl)+20);
        //sprintf(p, "http://%s:%d%s%s", host, port, path, &usericonurl[2]);
        //usericonurl = p;
    }

    
    //fprintf(stderr, "[icon url    = %s ]\n\n", usericonurl);
    
    //fprintf(stderr, "[profile url = http://"HOST""PATH"%s ]\n", &userprofileurl[2]);
	userprofileurl[0] = userprofileurl[0]; // dont show a warning message for unused variable...
    
    if (p) { free(p); p = NULL; } 


}

void minichat_users_at_this_moment(char *string){
    printf("%s\n", string);
}

// permet de parser un fichier contenant le code html pour tester le parsage 
// plutot que de se connecter sur le serveur a chaque fois !
/*
int test_html_parser(char *filename){
    char buf[1000];
    int bytes;
    int k;
    FILE *ftmp;
    tstate bkstate;
    message_t msg;
    memset(&msg, 0, sizeof(msg));
    
    ftmp = fopen(filename, "r");    
	if (!ftmp){
		fprintf(stderr, "Can't open file for reading !\n");
        return 1;
	}
    bkstate = state;
    state = GET_THE_BACKLOG;
    k=1;
    while ((bytes=fread(buf, 1, sizeof(buf), ftmp)) > 0) {
        parse_minichat_mess(buf, bytes, &msg, k);
        k=0;
    }

    state = bkstate;
    return 0;
}*/

int ishttpresponseok(char *buf, unsigned int bytes){
	unsigned int i; char tmp;
	for(i=0; i<bytes; i++){ 
		if (buf[i] == '\r' || buf[i] == '\n'){ 
			tmp = buf[i];
			buf[i] = '\0'; 
			display_debug(buf, 0); 
			buf[i] = tmp;
			break; 
		}
	}
	return 1; // TODO: analyze the response to know if it is ok !
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////// ENTRY POINT /////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

int main(void) {
	int s; //socket descriptor
	char buf[BUFSIZE+1]; // rx buffer
	int bytes;
	int k; // flag for any use
	char *outgoingmsg = NULL;

	int t; // timeslots remaining before next polling
	unsigned int nberr = 0;
	unsigned short wait_time = 40; // 10s
	unsigned short wait_time_maxi, wait_time_mini, wait_time_awake;
	
	tstate oldstate, futurestate;
	char *useragent = NULL;

	cookie_t cookies[MAXCOOKIES]; // se configure dans cookies.h
	message_t msg;

	memset(&cookies[0], 0, sizeof(cookies));
	memset(&msg,        0, sizeof(msg));

	display_init();
/*
	/// start debug todo remove that section
	{
		FILE *test;
		state = GET_THE_BACKLOG;
		test = fopen("mchat.txt", "r");
		k=1;
		while(fgets(buf, sizeof(buf), test) != NULL){
			bytes = strlen(buf);
			if (!bytes) { break; }
			buf[bytes] = 0;
			parse_minichat_mess(buf, bytes, &msg, k);
			//printf("%s", buf);
			k=0;
		}
		fclose(test);
	}    
//	display_waitforchar("Press any key for exit...");
			for(;;);
	return 0;
	// end
*/

	display_conversation(
	  "********************************************\n"
	  "**     Welcome to minichatclient for      **\n"
	  "**     rmcgirr83.org's phpBB's addon      **\n"
	  "** http://minichatclient.sourceforge.net/ **\n"
	  "********************************************\n\n"
	);

	f = fopen("output.log", "a");
	if (!f){
		display_debug("Can't open output.log for writing !", 0);
		display_waitforchar("Press any key to continue");
		return -1;
	}

    ws_init();
    
    /* reading configuration file */
	host            = read_conf_string("host",      host,      0);
	port            = read_conf_int   ("port",                 80);
	path            = read_conf_string("path",      path,      0);
	useragent       = read_conf_string("useragent", useragent, 0);
	wait_time_maxi  = read_conf_int   ("wait_time_maxi",       15) * (1000/WAITING_TIME_GRANOLOSITY);
	wait_time_mini  = read_conf_int   ("wait_time_mini",       5)  * (1000/WAITING_TIME_GRANOLOSITY);
	wait_time_awake = read_conf_int   ("wait_time_awake",      3)  * (1000/WAITING_TIME_GRANOLOSITY);
	
	//snprintf(stdout, "Server from the configuration file is: http://%s:%u%s", host, port, path);
	//snprintf(stdout, "User-Agent: %s\n", useragent);
	{
		char buf2show[200];
		snprintf(buf2show, 200, "Timmings: maxi=%0.2fs / mini=%0.2fs / awake=%0.2fs\n", (float)wait_time_maxi/(1000/WAITING_TIME_GRANOLOSITY), (float)wait_time_mini/(1000/WAITING_TIME_GRANOLOSITY), (float)wait_time_awake/(1000/WAITING_TIME_GRANOLOSITY));
		display_debug(buf2show, 0);
	}	
	
	if (!host || !path || !useragent){
		display_debug("Error: Server informations missing. Please edit your mchatclient.conf file !", 0);
		display_waitforchar("Press any key to quit");
		return -1;
	}
	
	if (read_conf_int("read_parser_rules", 0)){
		if(parser_loadrules()){
			display_conversation("Warning: Unable to load the parser rules. They will not be used.");
		}
	}
	
	{unsigned int i;
	for (i=0; i<1500; i+=WAITING_TIME_GRANOLOSITY){
		display_driver();
	}}

	
	state = LOADING_LOGIN_PAGE;
    for(;;){
        if (state != WAIT) {
			// on se connecte sur le serveur pour tout les cas sauf attentes
			s = maketcpconnexion(host, port);
			if (!s) { //
                nberr++;
                if (nberr == 5) {
					put_timestamp(f); 
					fprintf(f, "Unable to connect to the server anymore !\r\n"); 
					fflush(f);
                }
                wait_time = 10 * (1000/WAITING_TIME_GRANOLOSITY);
                futurestate = state;
                state = WAIT;
            }
            else { 
                if (nberr >= 5) {
					put_timestamp(f);
                    fprintf(f, "The server seem to be back now !\r\n");
                    fflush(f);
                    if (nberr >= 30) { // 5' ? reconnect from beginning.
                        state = LOADING_LOGIN_PAGE;
                    }
                }
                nberr = 0; 
            }
        }

        // now here is the main finite state machine
    	switch(state){
    		case LOADING_LOGIN_PAGE:
    			// première étape, on se connecte sur la page de login pour aller chercher un sid
                // (attention, il ne va fonctionner qu'avec l'user-agent spécifié, faut plus le changer !)
                {
					char *req = malloc(strlen(path)+strlen(LOGIN_PAGE)+1);
					strcpy(req, path);
					strcat(req, LOGIN_PAGE);
        			http_get(s, req, host, NULL, NULL, useragent, NULL);
					free(req); req=NULL;
				}
        		k=1;
        		while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
                    if(k) { 
						ishttpresponseok(buf, bytes);
						parsehttpheadersforgettingcookies(cookies, buf, bytes);
					}
                    k=0;
        		}
        		state = WAIT;
        		wait_time = 3;
        		futurestate = SUBMIT_AUTHENTIFICATION;
    			break;

    		case SUBMIT_AUTHENTIFICATION:
                // on s'identifie sur cette même page.
                // génération de ce que l'en va envoyer en POST pour se logger
                {
					char *username   = NULL;
					char *password   = NULL;
					
					char *req        = NULL;
					char *postdata   = NULL;
					char *referer    = NULL;
					char *cookiesstr = NULL;
					
					username = read_conf_string("username", username, 0);
					password = read_conf_string("password", password, 0);
					close_conf_file();
					
				    if (!username || !password) { 
						if (username) { free(username); username=NULL; }
						if (password) { free(password); password=NULL; }
						display_debug("Username/password informations missing or incomplete, skipping authentification. Tying to switch to the reading states though.", 0);
						state = GET_THE_BACKLOG; 
						break;
					}
					
					req = malloc(strlen(path)+strlen(LOGIN_PAGE)+1);
					strcpy(req, path);
					strcat(req, LOGIN_PAGE);
				    
				    //username=cLx&password=monpassword&redirect=index.php&login=Connexion
				    postdata = malloc(strlen("username=")+strlen(username)+strlen("&password=")+strlen(password)+strlen("&redirect=index.php&login=Connexion")+1);
	                strcpy(postdata, "username=");  strcat(postdata, username);
	                strcat(postdata, "&password="); strcat(postdata, password);
	                strcat(postdata, "&redirect=index.php&login=Connexion");
	                free(username); username=NULL;
	                free(password); password=NULL;
				    
				    referer = malloc(strlen("http://")+strlen(host)+strlen(path)+strlen(LOGIN_PAGE)+1);
					strcpy(referer, "http://");
				    strcat(referer, host);
				    strcat(referer, path);
				    strcat(referer, LOGIN_PAGE);

	                cookiesstr = generate_cookies_string(cookies, cookiesstr, 0);
					http_post(s, req, host, postdata, referer, cookiesstr, useragent, NULL);
	                
					free(req);        req=NULL;
	                free(postdata);   postdata=NULL;
	                free(cookiesstr); cookiesstr=NULL;
				}
                k=1;
        	    while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
                    if(k) {
						ishttpresponseok(buf, bytes);
						parsehttpheadersforgettingcookies(cookies, buf, bytes);
					}
					k=0;
        		}
        		state = WAIT;
        		wait_time = 3;
        		futurestate = GET_THE_BACKLOG;
    			break;

    		case GET_THE_BACKLOG:
                // ça, c'est pour récupérer le texte de la conversation déjà écrite comme le fait le navigateur
                {
					char *req        = NULL;
					char *referer    = NULL;
					char *cookiesstr = NULL;
					
					req = malloc(strlen(path)+strlen(MCHAT_PAGE)+1);
					strcpy(req, path);
					strcat(req, MCHAT_PAGE);

					referer = malloc(strlen("http://")+strlen(host)+strlen(path)+strlen(LOGIN_PAGE)+1);
					strcpy(referer, "http://");
				    strcat(referer, host);
				    strcat(referer, path);
				    strcat(referer, LOGIN_PAGE);
										
					cookiesstr = generate_cookies_string(cookies, cookiesstr, 0);
					http_get(s, req, host, referer, cookiesstr, useragent, NULL);
					
					free(req);        req=NULL;
					free(referer);    referer=NULL;
					free(cookiesstr); cookiesstr=NULL;
				}
        		k=1;
                while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
                    if(k) { 
						ishttpresponseok(buf, bytes);
						parsehttpheadersforgettingcookies(cookies, buf, bytes);
					}
        			parse_minichat_mess(buf, bytes, &msg, k);
        			k=0;
	            }
        		state = WAIT;
        		wait_time = 10;
        		futurestate = WATCHING_NEW_MESSAGES;
    			break;

     	    case WATCHING_NEW_MESSAGES:
                // ... et ça, c'est pour récupérer ce qui s'y passe !
                {
					char *req        = NULL;
					char *postdata   = NULL;
					char *referer    = NULL;
					char *cookiesstr = NULL;
					
					req = malloc(strlen(path)+strlen(MCHAT_PAGE)+1);
					strcpy(req, path);
					strcat(req, MCHAT_PAGE);
	        		
	        		// => donner l'id du dernier message reçu
	        		postdata = malloc(strlen("mode=read&message_last_id=")+strlen(msg.msgid)+1);
	        		strcpy(postdata, "mode=read&message_last_id=");
	        		strcat(postdata, msg.msgid);
	        		
	        		referer = malloc(strlen("http://")+strlen(host)+strlen(path)+strlen(MCHAT_PAGE)+1);
					strcpy(referer, "http://");
					strcat(referer, host);
					strcat(referer, path);
					strcat(referer, MCHAT_PAGE);
					
					cookiesstr = generate_cookies_string(cookies, cookiesstr, 0);
	                http_post(s, req, host, postdata, referer, cookiesstr, useragent, NULL);
	                
					free(req);        req=NULL;
					free(postdata);   postdata=NULL;
					free(referer);    referer=NULL;
					free(cookiesstr); cookiesstr=NULL;
				}
				{
					unsigned short nbmessages = 0, old_wait_time;
					
	        		k=1;
	                while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
	                    if(k) { 
							ishttpresponseok(buf, bytes);
							parsehttpheadersforgettingcookies(cookies, buf, bytes); 
						}
	        			nbmessages = parse_minichat_mess(buf, bytes, &msg, k);
	        			k=0;
	        		}   
					
					old_wait_time = wait_time;
	        		if (nbmessages == 0) {
						wait_time*=1.5;
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
					char *req        = NULL;
					char *referer    = NULL;
					char *cookiesstr = NULL;
					
					req = malloc(strlen(path)+strlen(MCHAT_PAGE)+1);
					strcpy(req, path);
					strcat(req, MCHAT_PAGE);
					
					referer = malloc(strlen("http://")+strlen(host)+strlen(path)+strlen(MCHAT_PAGE)+1);
					strcpy(referer, "http://");
					strcat(referer, host);
					strcat(referer, path);
					strcat(referer, MCHAT_PAGE);
					
	                cookiesstr = generate_cookies_string(cookies, cookiesstr, 0);
	                http_post(s, req, host, "mode=stats", referer, cookiesstr, useragent, NULL);
	                
	                free(req);        req=NULL;
					free(referer);    referer=NULL;
					free(cookiesstr); cookiesstr=NULL;
				}
                k=1;
                {
					FILE *test;
                	test = fopen("stats.txt", "w");
				
                while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
                    if(k) {
						ishttpresponseok(buf, bytes);
						parsehttpheadersforgettingcookies(cookies, buf, bytes);
					}
       				parse_minichat_mess(buf, bytes, &msg, k);
       				fwrite(buf, bytes, 1, test);
       				k=0;
                }
                fclose(test); }
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
					char *tmp = NULL; // please don't free() outgoingmsg! else it will be double freed.

					// replace some charactersdisturbing the POST string...
					strrep(outgoingmsg, &tmp, "%", "%25");
					strrep(NULL,        &tmp, "+", "%2B");
					strrep(NULL,        &tmp, "&", "%26");
					strrep(NULL,        &tmp, "=", "%3D");
					strrep(NULL,        &tmp, " ", "+");


					req = malloc(strlen(path)+strlen(MCHAT_PAGE)+1);
					strcpy(req, path);
					strcat(req, MCHAT_PAGE);

					// mode=add&message=TESTMSG&helpbox=Tip%3A+Styles+can+be+applied+quickly+to+selected+text.&addbbcode20=100&addbbcode_custom=%23
					postdata = malloc(strlen(POSTDATALEFT) + strlen(tmp) + strlen(POSTDATARIGHT) + 1);
					strcpy(postdata, POSTDATALEFT);
					strcat(postdata, tmp);
					strcat(postdata, POSTDATARIGHT);

					referer = malloc(strlen("http://")+strlen(host)+strlen(path)+strlen(MCHAT_PAGE)+1);
					strcpy(referer, "http://");
					strcat(referer, host);
					strcat(referer, path);
					strcat(referer, MCHAT_PAGE);
					
					storecookie(cookies, "mChatShowUserList", "yes");
					cookiesstr = generate_cookies_string(cookies, cookiesstr, 0);
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
						ishttpresponseok(buf, bytes);
						parsehttpheadersforgettingcookies(cookies, buf, bytes);

					}
        			//fwrite(buf, bytes, 1, stderr); // envoi vers console
					k=0;
        		}
        		wait_time = wait_time_awake;
    			state = WATCHING_NEW_MESSAGES; // le changement d'état est important ;)
    			break;

   			case WAIT:
                // on attends un peu entre chaque refresh pour ne pas saturer le serveur
                if (state != oldstate) {
                    t = wait_time;
                    //display_status(stderr, "\n[Waiting     ]\b");
                    display_debug("Waiting......", 0);
                    //oldstate = state;
                }
				if (t){{
					const char anim[4] = {'\\', '-', '/', '|'};
					char buf2show[15];
					t--; 
					//fprintf(stderr, "\b\b\b\b%c%3.0d", anim[t%4], (int)(t/(1000/WAITING_TIME_GRANOLOSITY))); 
					//snprintf(buf, 15, "Waiting%3us %c", (unsigned int)(t/(1000/WAITING_TIME_GRANOLOSITY)), anim[t%4]); 
					//display_statusbar(buf);
					snprintf(buf2show, 15, "\b\b\b\b\b\b%3us %c", (unsigned int)(t/(1000/WAITING_TIME_GRANOLOSITY)), anim[t%4]); 
					display_debug(buf2show, 1);
					//oldstate = state;
				}}
				else {
					state = futurestate;
					//fprintf(stderr, "\r              \r");
					//display_statusbar(NULL);
					display_debug("\b\b\b\b\b\b\b\b\b\b\b\b\b             \b\b\b\b\b\b\b\b\b\b\b\b\b", 1);
				}

				// timebase and checks for keyboard inputs. eg, like "Sleep(WAITING_TIME_GRANOLOSITY);"
                outgoingmsg = display_driver(); // please don't free(), it will happen it the next time.
                if (outgoingmsg) {
					state = POSTING_A_MESSAGE;
				}
				oldstate = state; // bug if not here ? why ?
                break;
    	}
     
        // if a TCP connexion to the server is present, terminate it !
        if (s) { closesocket(s); s = 0; }
    }
	fclose(f);
	freecookies(cookies);
	parser_freerules();
    ws_cleanup();
    display_end();
	return 0;
}
