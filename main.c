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

// config file
#include "userconfig.h" // toute la config se fait là bas !

// hééé oui, encore une machine à états ! ^^
typedef enum {
    LOADING_LOGIN_PAGE = 0,
    SUBMIT_AUTHENTIFICATION,
    GET_THE_BACKLOG,
    WATCHING_NEW_MESSAGES,
    RETRIEVING_THE_LIST_OF_USERS,
    POSTING_A_MESSAGE,
    WAIT_BEFORE_WATCHING_NEW_MESSAGES
} tstate;

// quelques variables globales
tstate state;
FILE *f;


// ces fonctions sont appelées en retour par parsehtml.c
void minichat_message(char* username, char* message, char *usericonurl, char *userprofileurl){
    struct tm *ptm;
    time_t lt;
    static unsigned int day;
    char *p = NULL;

    lt = time(NULL);
    ptm = localtime(&lt);

    if (ptm->tm_mday != day){
        fprintf(stdout, "*** %04u-%02u-%02u ***\n",   ptm->tm_year+1900, ptm->tm_mon, ptm->tm_mday);
        //fprintf(f     , "*** %04u-%02u-%02u ***\r\n", ptm->tm_year+1900, ptm->tm_mon, ptm->tm_mday);
        day = ptm->tm_mday;
    }

    // gere si la user icon est sur le serveur (avec une adresse relative ./)
    // (ne pas oublier d'alouer pour "http://", ":00000" et \0)
    if (usericonurl[0] == '.' && usericonurl[1] == '/') {
        p = malloc(strlen(HOST)+strlen(PATH)+strlen(usericonurl)+20);
        sprintf(p, "http://%s:%d%s%s", HOST, PORT, PATH, &usericonurl[2]);
        usericonurl = p;
    }
    fprintf(stderr, "[icon url    = %s ]\n", usericonurl);
    fprintf(stderr, "[profile url = http://"HOST""PATH"%s ]\n", &userprofileurl[2]);
    if (state == GET_THE_BACKLOG){
        fprintf(f     , "[    BACK-LOG    ] "); //4+1+2+1+2+1+2+1+2 = 16
        fprintf(stdout, "[BKLOG] "); 
    }
    else {
        fprintf(f     , "[%04u-%02u-%02u", ptm->tm_year+1900, ptm->tm_mon, ptm->tm_mday);
        fprintf(f     , " %02u:%02u] ", ptm->tm_hour, ptm->tm_min);
        fprintf(stdout, "[%02u:%02u] ", ptm->tm_hour, ptm->tm_min);
    }

    // display the message
    fprintf(stdout, "<%s> %s\n\n", username, message);
    fprintf(f     , "<%s> %s\r\n", username, message);
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

#define MAXBUF 2000

// permet de parser un fichier contenant le code html pour tester le parsage 
// plutot que de se connecter sur le serveur a chaque fois !
int test_html_parser(char *filename){
    char buf[MAXBUF+1];
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
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////// ENTRY POINT /////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

int main(void) {
	int s; //socket descriptor
	char buf[MAXBUF+1], buf2[MAXBUF+1];
	int bytes;
	int k; // flag for any use
	int t;
    int b=0;

	tstate oldstate;

    // on garde un peu de place pour stocker les cookies
    cookie_t cookies[MAXCOOKIES];
    memset(cookies, 0, sizeof(cookies));

    // message structure
	message_t msg;
    memset(&msg, 0, sizeof(msg));

    fprintf(stdout, "\n\
    *******************************************************************\n\
    *** Welcome to minichatclient for rmcgirr83.org's phpBB's addon ***\n\
    ***          http://minichatclient.sourceforge.net/             ***\n\
    *******************************************************************\n\
    \n");

	f = fopen("output.txt", "a");
	if (!f){
		fprintf(stderr, "Can't open file for writing !\n");
	}

    //decommenter la ligne qui suit si on veut faire du debug sur le parseur html !
    //test_html_parser("sample.html"); return 0;

    // Si la plateforme est Windows
    ws_init();

    if (USER[0] != '\0') { state = LOADING_LOGIN_PAGE; }
    else { state = GET_THE_BACKLOG; }

    for(;;){
    	switch(state){
    		case LOADING_LOGIN_PAGE:
    			// première étape, on se connecte sur la page de login pour aller chercher un sid
                // (attention, il ne va fonctionner qu'avec l'user-agent spécifié, faut plus le changer !)
    			s = maketcpconnexion(HOST, PORT);
    			if (s) {
        			http_get(s, PATH"ucp.php?mode=login", HOST, NULL, NULL, USERAGENT, NULL);
        			k=1;
        			while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
                        if(k) parsehttpheadersforgettingcookies(cookies, buf);
                        k=0;
        			}
        			state = SUBMIT_AUTHENTIFICATION;
                }
    			close(s);
    			break;

    		case SUBMIT_AUTHENTIFICATION:
                // on s'identifie sur cette même page
    			s = maketcpconnexion(HOST, PORT);

                // génération de ce que l'en va envoyer en POST pour se logger
                strncpy(buf, "username=",  MAXBUF);strncat(buf, USER,     MAXBUF);
                strncat(buf, "&password=", MAXBUF);strncat(buf, PASSWORD, MAXBUF);
                strncat(buf, "&redirect=index.php&login=Connexion",       MAXBUF);

                generate_cookies_string(cookies, buf2, MAXBUF);
                http_post(s, PATH"ucp.php?mode=login", HOST, buf, "http://"HOST""PATH"ucp.php?mode=login", buf2, USERAGENT, NULL);

                k=1;
    			while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
                    if(k) parsehttpheadersforgettingcookies(cookies, buf);
                    k=0;
    			}
    			state = GET_THE_BACKLOG;
    			close(s);
    			break;

    		case GET_THE_BACKLOG:
                // ça, c'est pour récupérer le texte de la conversation déjà écrite comme le fait le navigateur
    			s = maketcpconnexion(HOST, PORT);
    			generate_cookies_string(cookies, buf, MAXBUF);
                http_get(s, PATH"mchat.php", HOST, "http://"HOST""PATH"ucp.php?mode=login", buf, USERAGENT, NULL);
    			k=1;
                while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
                    if(k) { parsehttpheadersforgettingcookies(cookies, buf); }
    				parse_minichat_mess(buf, bytes, &msg, k);
    				k=0;
                }
    			state = RETRIEVING_THE_LIST_OF_USERS;
    			close(s);
    			break;

     	    case WATCHING_NEW_MESSAGES:
                // ... et ça, c'est pour récupérer ce qui se passe en temps réel !
    			s = maketcpconnexion(HOST, PORT);
    			generate_cookies_string(cookies, buf, MAXBUF);
    			// => id dernier message reçu à récupérer et renvoyer
    			strncpy(buf2, "mode=read&message_last_id=", sizeof(buf2));
    			strncat(buf2, msg.msgid, sizeof(buf2));
                http_post(s, PATH"mchat.php", HOST, buf2, "http://"HOST""PATH"mchat.php", buf, USERAGENT, NULL);
    			k=1;
                while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
                    if(k) { parsehttpheadersforgettingcookies(cookies, buf); }
    				parse_minichat_mess(buf, bytes, &msg, k);
                    fwrite(buf, bytes, 1, stderr); // envoi vers console
    				k=0;
    			}
    			state = WAIT_BEFORE_WATCHING_NEW_MESSAGES;
    			close(s);
    			break;

            case RETRIEVING_THE_LIST_OF_USERS:
                // de temps en temps, on peut regarder qui est là.
                s = maketcpconnexion(HOST, PORT);
                generate_cookies_string(cookies, buf, MAXBUF);
                http_post(s, PATH"mchat.php", HOST, "mode=stats", "http://"HOST""PATH"mchat.php", buf, USERAGENT, NULL);
                k=1;
                while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
                    if(k) parsehttpheadersforgettingcookies(cookies, buf);
   				    parse_minichat_mess(buf, bytes, &msg, k);
   				    k=0;
                }
    			state = WAIT_BEFORE_WATCHING_NEW_MESSAGES;
    			close(s);
                break;

    		case POSTING_A_MESSAGE:
                // et enfin, ça, c'est pour y poster quelque chose. faire gaffe de ne pas flooder sinon Timmy va se fâcher.
    			s = maketcpconnexion(HOST, PORT);
                storecookie(cookies, "mChatShowUserList", "yes");
                generate_cookies_string(cookies, buf, MAXBUF);
    			http_post(s, PATH"mchat.php", HOST, "mode=add&message="TESTMSG"&helpbox=Tip%3A+Styles+can+be+applied+quickly+to+selected+text.&addbbcode20=100&addbbcode_custom=%23", "http://"HOST""PATH"mchat.php", buf, USERAGENT, NULL);
                k=1;
                while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
                    if(k) parsehttpheadersforgettingcookies(cookies, buf);
    				fwrite(buf, bytes, 1, stderr); // envoi vers console
    				k=0;
    			}
    			close(s);
    			state = WATCHING_NEW_MESSAGES; // le changement d'état est important ;)
    			break;

   			case WAIT_BEFORE_WATCHING_NEW_MESSAGES:
                // on attends un peu entre chaque refresh pour ne pas saturer le serveur
                if (state != oldstate) {
                    t = 0;
                    fprintf(stderr, "\nWaiting");
                }
                else {
                    t++;
                    if (t>=WAITING_TIME*4){
                        // Niark ! :)
                        if (b++ == 5 && TESTMSG[0] != '\0'){ state = POSTING_A_MESSAGE; }
                        else { state = WATCHING_NEW_MESSAGES; }
                        fprintf(stderr, "\r                                              ");
                    }
                    else {
                        fprintf(stderr, ".");
                    }
                }

                Sleep(250);
                oldstate = state;
                break;
    	}
    }
	fclose(f);
	freecookies(cookies);
    // Si la plateforme est Windows
    ws_cleanup();

	return 0;
}
