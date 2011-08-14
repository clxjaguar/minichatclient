// Ce truc est une sorte declient http afin de servir depasserelle vers le minichat rmcgirr83.org pour phpbb.
// pourquoi toujours utiliser les trucs les moins compatibles possibles ?
//
// Sous windows, il faut passer -lws2_32 au linker (nous utilisons aussi gcc).
//
// Changelog:
// 05/04/11 : cLx       Début du programme (connexions vers un serveur HTTP), fonctionne sous Debian
// 05/04/11 : Nejaa     Portabilisation du code existant vers Windows
// 06/04/11 : cLx       Test sous XP => OK!
// 07/04/11 : cLx       Grosses modifications dans plusieurs fonctions du programme. Le programme peut se logger et envoyer du texte
// 08/04/11 : cLx       Découpage en differents objets. Traitement des cookies de façon automatique.
// 08/04/11 : Nejaa     Minimes modification pour compiler sans warnings et fonction d'attente (provisoire) portabilité a tester
// 15/04/11 : cLx       Une machines à états pour faire un semblant de parsage sur l'HTML
// 17/04/11 : cLx       La fonction de decodage fait maintenant quelque chose de propre ! :D
// 18/04/11 : Nejaa     Ajout du marquage de l'heure
// 19/04/11 : cLx       Corrections sur l'horodatage / entites html / un peu plein de petits trucs partout
// 11/05/11 : cLx       Correction d'un bug a la con dans le parseur html (en cas de usericon sur d'autres serveurs que le forum)
// 29/05/11 : cLx       Amelioration du comportement en cas de problemes de connexion 
// 23/06/11 : cLx       Modifications pour récupérer la config depuis un fichier de conf.
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
// [_] parser le HTML présant quand il y a un lien, ou un quote, ou qu'on clique sur le '@'...
// [_] delai de polling adaptatif selon qu'il se passe des trucs ou non
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
    #define close(s) closesocket(s)
#else
    #include <sys/socket.h>
    #define Sleep(s) usleep(s*1000)
#endif

#include "cookies.h"
#include "network.h"
#include "parsehtml.h"
#include "conf.h"

#define LOGIN_PAGE "ucp.php?mode=login"
#define MCHAT_PAGE "mchat.php"
#define TESTMSG    "{Meeowwwss!}"

// waiting time granolosity for pooling minichat server in milliseconds
#define WAITING_TIME_GRANOLOSITY 250

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
    //static unsigned int day;

    lt = time(NULL);
    ptm = localtime(&lt);
    
    if (f == NULL) { return; }

    if (state == GET_THE_BACKLOG){
		if (f != stdout) {
			fprintf(f     , "[    BACK-LOG    ] "); //4+1+2+1+2+1+2+1+2 = 16
		}
		else {
			fprintf(f, "[BKLOG] "); 
		}
	}
	else {
		if (f != stdout) {
			fprintf(f     , "[%04u-%02u-%02u", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday);
			fprintf(f     , " %02u:%02u] ", ptm->tm_hour, ptm->tm_min);
		}
		else {
			fprintf(f, "[%02u:%02u] ", ptm->tm_hour, ptm->tm_min);
		}
    }
}

// ces fonctions sont appelées en retour par parsehtml.c
void minichat_message(char* username, char* message, char *usericonurl, char *userprofileurl){
    char *p = NULL;
    
    // gere si la user icon est sur le serveur (avec une adresse relative ./)
    // (ne pas oublier d'alouer pour "http://", ":00000" et \0)
    if (usericonurl[0] == '.' && usericonurl[1] == '/') {
        p = malloc(strlen(host)+strlen(path)+strlen(usericonurl)+20);
        sprintf(p, "http://%s:%d%s%s", host, port, path, &usericonurl[2]);
        usericonurl = p;
    }
    
    // display the message
    put_timestamp(stdout);
    fprintf(stdout, "<%s> %s\n", username, message);

	// and put it in the log file    
    put_timestamp(f);
    fprintf(f     , "<%s> %s\r\n", username, message);
    
    fprintf(stderr, "[icon url    = %s ]\n\n", usericonurl);
    
    //fprintf(stderr, "[profile url = http://"HOST""PATH"%s ]\n", &userprofileurl[2]);
	userprofileurl[0] = userprofileurl[0]; // dont show a warning message for unused variable...
    
    fflush(f);

    if (p) { free(p); p = NULL; }


}

void minichat_users_at_this_moment(char *string){
    printf("%s\n", string);
    /* il faudrait décoder ça dans parcehtml.c :
        <div class="mChatStats" id="mChatStats"><a href="#" onclick="mChat.toggle('UserList'); return false;">In total there are <strong>5</strong>
users chatting  sur le Chien, ouch, le pauvre !</a>&nbsp;( based on users active over the past 10&nbsp;minutes )<br /><span id="mChatUserLis
t" style="display: none; float: left;"><a href="./memberlist.php?mode=viewprofile&amp;u=1201">Fluffy</a>, <a href="./memberlist.php?mode=vie
wprofile&amp;u=1027">Rey</a>, <a href="./memberlist.php?mode=viewprofile&amp;u=1042">Shalinka</a>, <a href="./memberlist.php?mode=viewprofil
e&amp;u=1184">Teobryn</a>, <a href="./memberlist.php?mode=viewprofile&amp;u=5">cLx</a></span></div>"*/
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

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////// ENTRY POINT /////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

int main(void) {
	int s; //socket descriptor
	char buf[BUFSIZE+1]; // rx buffer
	int bytes;
	int k; // flag for any use

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

    fprintf(stdout, "\n\
    *******************************************************************\n\
    *** Welcome to minichatclient for rmcgirr83.org's phpBB's addon ***\n\
    ***          http://minichatclient.sourceforge.net/             ***\n\
    *******************************************************************\n\
    \n");

	f = fopen("output.log", "a");
	if (!f){
		fprintf(stderr, "Can't open output.log for writing !\n");
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
	
	fprintf(stdout, "Server from the configuration file is: http://%s:%u%s\n", host, port, path);
	fprintf(stdout, "User-Agent: %s\n", useragent);
	fprintf(stdout, "Timmings: maxi=%0.2fs / mini=%0.2fs / awake=%0.2fs\n", (float)wait_time_maxi/(1000/WAITING_TIME_GRANOLOSITY), (float)wait_time_mini/(1000/WAITING_TIME_GRANOLOSITY), (float)wait_time_awake/(1000/WAITING_TIME_GRANOLOSITY));
	
	if (!host || !path || !useragent){
		fprintf(stdout, "Error: Server informations missing. Please edit your mchatclient.conf file ! Exiting now...\n");
		Sleep(2000);
		return -1;
	}
	
	if (read_conf_int("read_parser_rules", 0)){
		if(parser_loadrules()){
			fprintf(stdout, "Warning: Unable to load the parser rules. They will not be used.\n");
			Sleep(2000);
		}
	}
	
	state = LOADING_LOGIN_PAGE;
    for(;;){
        if (state != WAIT) {
			// on se connecte sur le serveur pour tout les cas sauf attentes
			s = maketcpconnexion(host, port);
			if (!s) { //
                nberr++;
                if (nberr == 2) {
					put_timestamp(f);
                    fprintf(f, "Unable to connect to the server anymore !\r\n");
                    fflush(f);
                }
                wait_time = 10 * (1000/WAITING_TIME_GRANOLOSITY);
                futurestate = state;
                state = WAIT;
            }
            else { 
                if (nberr >= 2) {
					put_timestamp(f);
                    fprintf(f, "The server seem to be back now !\r\n");
                    if (nberr >= 30) { // 5' ? reconnect from beginning.
                        state = LOADING_LOGIN_PAGE;
                    }
                    fflush(f);
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
                    if(k) parsehttpheadersforgettingcookies(cookies, buf);
                    k=0;
        		}
        		state = SUBMIT_AUTHENTIFICATION;
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
					
				    if (!username || !password) { 
						if (!username) { free(username); username=NULL; }
						if (!password) { free(password); password=NULL; }
						fprintf(stderr, "Username/password informations missing or incomplete, skipping authentification. Tying to switch to the reading states though.");
						close_conf_file();
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
				    close_conf_file();
				    
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
                    if(k) parsehttpheadersforgettingcookies(cookies, buf);
                    k=0;
        		}
        		state = GET_THE_BACKLOG;
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
                    if(k) { parsehttpheadersforgettingcookies(cookies, buf); }
        			parse_minichat_mess(buf, bytes, &msg, k);
        			k=0;
                }
        		state = RETRIEVING_THE_LIST_OF_USERS;
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
	                    if(k) { parsehttpheadersforgettingcookies(cookies, buf); }
	        			nbmessages = parse_minichat_mess(buf, bytes, &msg, k);
	                    if(k) {{ int i; for(i=0; i<bytes; i++){ fprintf(stderr, "%c", buf[i]); if (buf[i] == '\n'){break;} }}}
						////fwrite(buf, bytes, 1, stderr); } // envoi vers console
	        			k=0;
	        		}   

					old_wait_time = wait_time;
	        		if (nbmessages == 0) {
						wait_time*=1.5; //*(1000/WAITING_TIME_GRANOLOSITY);
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
                while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
                    if(k) parsehttpheadersforgettingcookies(cookies, buf);
       				parse_minichat_mess(buf, bytes, &msg, k);
       				k=0;
                }
                //wait_time = wait_time_mini; // TODOTODOTODO
                //futurestate = WATCHING_NEW_MESSAGES;
        		//state = WAIT;
        		state = WATCHING_NEW_MESSAGES;
                break;

    		case POSTING_A_MESSAGE:
                // et enfin, ça, c'est pour y poster quelque chose. faire gaffe de ne pas flooder sinon Timmy va se fâcher.
                {
					char *req        = NULL;
					char *postdata   = NULL;
					char *referer    = NULL;
					char *cookiesstr = NULL;
					
					req = malloc(strlen(path)+strlen(MCHAT_PAGE)+1);
					strcpy(req, path);
					strcat(req, MCHAT_PAGE);
					
					//"mode=add&message="TESTMSG"&helpbox=Tip%3A+Styles+can+be+applied+quickly+to+selected+text.&addbbcode20=100&addbbcode_custom=%23"
					
					referer = malloc(strlen("http://")+strlen(host)+strlen(path)+strlen(MCHAT_PAGE)+1);
					strcpy(referer, "http://");
					strcat(referer, host);
					strcat(referer, path);
					strcat(referer, MCHAT_PAGE);
					
					storecookie(cookies, "mChatShowUserList", "yes");
					cookiesstr = generate_cookies_string(cookies, cookiesstr, 0);
					http_post(s, req, host, postdata, referer, cookiesstr, useragent, NULL);
	        	    
					free(req);        req=NULL;
					free(postdata);   postdata=NULL;
					free(referer);    referer=NULL;
					free(cookiesstr); cookiesstr=NULL;
				}
                k=1;
                while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
                    if(k) parsehttpheadersforgettingcookies(cookies, buf);
        			fwrite(buf, bytes, 1, stderr); // envoi vers console
        			k=0;
        		}
        		wait_time = wait_time_awake;
    			state = WATCHING_NEW_MESSAGES; // le changement d'état est important ;)
    			break;

   			case WAIT:
                // on attends un peu entre chaque refresh pour ne pas saturer le serveur
                if (state != oldstate) {
                    t = wait_time;
                    fprintf(stderr, "\n[Waiting     ]\b");
                }
				if (t){{
					const char anim[4] = {'\\', '-', '/', '|'};
					t--; fprintf(stderr, "\b\b\b\b%c%3.0d", anim[t%4], (int)(t/(1000/WAITING_TIME_GRANOLOSITY))); 
				}}
				else {
					state = futurestate;
					fprintf(stderr, "\r              \r");
				}

                Sleep(WAITING_TIME_GRANOLOSITY);
                oldstate = state;
                break;
    	}
     
        // if a TCP connexion to the server is present, terminate it !
        if (s) { close(s); s = 0; }
    }
	fclose(f);
	freecookies(cookies);
	parser_freerules();
    ws_cleanup();
	return 0;
}
