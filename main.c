// Ce truc est une sorte declient http afin de servir depasserelle vers le minichat rmcgirr83.org pour phpbb.
// pourquoi toujours utiliser les trucs les moins compatibles possibles ?
//
// pour compiler utiliser "gcc client-chat-FF.c". Sous windows, il faut passer
// -lws2_32 au linker (nous utilisons aussi gcc).
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
//
// Todo:
// [x] réseau, compatibilité windows, fonctions http de bases (get et post)
// [x] parser les cookies et remplir un tableau en les ajoutant/remplaçant selon le nom du cookie, et les renvoyer au serveur ensuite
// [x] décoder le html de la conversation (on veut le nom du compte de la personne, l'URL de l'icône du compte, l'ID du message, et bien sûr le texte)
// [x] sniffer la partie pour récupérer les personnes dans la conversation
// [x] et programmer le decodage qui va bien (dans la même fonction ça serait bien)
// [_] récuperer les gens qu'il y a dans le canal (sous quel format?)
// [?] implémenter un serveur IRC pour un maximum de compatibilité avec un client d'IM ? ou faire un add-on ? ou faire du multi-user et gérer ça coté serveur ?

// Licence: Au cas où ça se révélerait indispensable, la GPL ?
// Garantie: Aucune. Timmy, si quelqu'un crashe ton serveur avec ce truc, c'est pas notre faute ! ^^

///// CONFIG
#define USERAGENT "Pas Firefox, ni IE, ni Chrome, ni Opera et encore moins Safari !"
#define HOST "forum.francefurs.org"
#define PORT 80
#define ATTENTE 4 // temps d'attente, en secondes (granulosité : 0.25s)
//#define TESTMSG "%7Bmessage+sended+with+mchatclient.exe+(cc-by-nc+cLx+2011)&7D"
#define TESTMSG "%7Bmeeeooow%7D"
#include "userconfig.h" // le login est mot de passe sont là bas !

///// FIN de la config

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

#if defined (linux)
    #include <sys/socket.h>
#elif defined (WIN32)
    #include <winsock2.h>
//    #include <time.h>
    #define close(s) closesocket(s)
#endif

#include "cookies.h"
#include "network.h"
#include "parsehtml.h"


/* Doit pouvoir décoder :
        <div class="mChatStats" id="mChatStats"><a href="#" onclick="mChat.toggle('UserList'); return false;">In total there are <strong>5</strong>
users chatting  sur le Chien, ouch, le pauvre !</a>&nbsp;( based on users active over the past 10&nbsp;minutes )<br /><span id="mChatUserLis
t" style="display: none; float: left;"><a href="./memberlist.php?mode=viewprofile&amp;u=1201">Fluffy</a>, <a href="./memberlist.php?mode=vie
wprofile&amp;u=1027">Rey</a>, <a href="./memberlist.php?mode=viewprofile&amp;u=1042">Shalinka</a>, <a href="./memberlist.php?mode=viewprofil
e&amp;u=1184">Teobryn</a>, <a href="./memberlist.php?mode=viewprofile&amp;u=5">cLx</a></span></div>"

*/

FILE *f;
/*
void parse_minichat_mess(char *buf, unsigned int bytes){
     fwrite(buf, bytes, 1, stdout); // debug vers console
     fwrite(buf, bytes, 1, f); // envoi vers fichier
}
*/

// ces fonctions sont appelées en retour par parsehtml.c



void minichat_message(char* username, char* message, char *usericonurl, char *userprofileurl){
    char h[3], m[3], s[3];
    time_t secondes=time(NULL);
    struct tm temps;
    temps=*localtime(&secondes);

    fprintf(stdout, "%d:%d:%d <%s> %s\n", temps.tm_hour, temps.tm_min, temps.tm_sec, username, message);

    itoa(temps.tm_hour, h, 10);
    itoa(temps.tm_min, m, 10);
    itoa(temps.tm_sec, s, 10);

    fwrite(&h, strlen(h), 1, f);
    fwrite(":", 1, 1, f);
    fwrite(&m, strlen(m), 1, f);
    fwrite(":", 1, 1, f);
    fwrite(&s, strlen(s), 1, f);
    fwrite(" ", 1, 1, f);
    fwrite("<", 1, 1, f);
    fwrite(username, strlen(username), 1, f);
    fwrite("> ", 2, 1, f);
    fwrite(message, strlen(message), 1, f);
    fwrite("\r\n", 2, 1, f);
    //printf("[icon url] %s\n[profile url] %s\n", usericonurl, userprofileurl);
    fflush(f);
}

void minichat_users_at_this_moment(char *string){
    printf("%s\n", string);
}

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////// ENTRY POINT /////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#define MAXBUF 1000

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

int main(void) {
	int s; //socket descriptor
	char buf[MAXBUF+1], buf2[MAXBUF+1];
	int bytes;
	int k; // flag for any use
	int t;


	tstate oldstate, state = LOADING_LOGIN_PAGE;

    // on garde un peu de place pour stocker les cookies
    cookie_t cookies[MAXCOOKIES];
    memset(cookies, 0, sizeof(cookies));

	message_t msg;
    memset(&msg, 0, sizeof(msg));


    //structures de récuperation de message
    //Messaget current_message, previous_message;
    //memset(&current_message, 0, sizeof(Messaget));
    //memset(&previous_message, 0, sizeof(Messaget));

	f = fopen("output.txt", "w");
	if (!f){
		fprintf(stderr, "Can't open file for writing !\n");
	}

    // Si la plateforme est Windows
    ws_init();


    for(;;){
    	switch(state){
    		case LOADING_LOGIN_PAGE:
    			// première étape, on se connecte sur la page de login pour aller chercher un sid
                // (attention, il ne va fonctionner qu'avec l'user-agent spécifié, faut plus le changer !)
    			s = maketcpconnexion(HOST, PORT);
    			if (s) {
        			http_get(s, "/ucp.php?mode=login", HOST, NULL, NULL, USERAGENT, NULL);
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
                http_post(s, "/ucp.php?mode=login", HOST, buf, "http://"HOST"/ucp.php?mode=login", buf2, USERAGENT, NULL);

                k=1;
    			while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
                    if(k) parsehttpheadersforgettingcookies(cookies, buf);
                    k=0;
    			}
    			state = GET_THE_BACKLOG;
    			close(s);
    			break;

    		case GET_THE_BACKLOG:
                // ça, c'est pour récupérer le texte de la conversation déjà écrite...
    			s = maketcpconnexion(HOST, PORT);
    			generate_cookies_string(cookies, buf, MAXBUF);
                //http_post(s, "/mchat.php", HOST, "mode=read", "http://"HOST"/mchat.php", buf, USERAGENT, NULL);
                http_get(s, "/mchat.php", HOST, "http://"HOST"/ucp.php?mode=login", buf, USERAGENT, NULL); // plus comme l'application !
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
                http_post(s, "/mchat.php", HOST, buf2, "http://"HOST"/mchat.php", buf, USERAGENT, NULL);
    			k=1;
                while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
                    if(k) { parsehttpheadersforgettingcookies(cookies, buf); }
    				parse_minichat_mess(buf, bytes, &msg, k);
    				k=0;
    			}
    			state = WAIT_BEFORE_WATCHING_NEW_MESSAGES;
    			close(s);
    			break;

            case RETRIEVING_THE_LIST_OF_USERS:
                // de temps en temps, on peut regarder qui est là.
                s = maketcpconnexion(HOST, PORT);
                generate_cookies_string(cookies, buf, MAXBUF);
                http_post(s, "/mchat.php", HOST, "mode=stats", "http://"HOST"/mchat.php", buf, USERAGENT, NULL);
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
    			http_post(s, "/mchat.php", HOST, "mode=add&message="TESTMSG"&helpbox=Tip%3A+Styles+can+be+applied+quickly+to+selected+text.&addbbcode20=100&addbbcode_custom=%23", "http://"HOST"/mchat.php", buf, USERAGENT, NULL);
                                                //mode=add&message=meowwmeow&helpbox=Tip%3A+Styles+can+be+applied+quickly+to+selected+text.&addbbcode20=100&addbbcode_custom=%23
                k=1;
                while ((bytes=recv(s, buf, sizeof(buf), 0)) > 0) {
                    if(k) parsehttpheadersforgettingcookies(cookies, buf);
    				fwrite(buf, bytes, 1, stdout); // envoi vers console
    				fwrite(buf, bytes, 1, f); // envoi vers fichier
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
                    if (t>=ATTENTE*4){
                        /*
                        if (b++ == 5){
                            state = POSTING_A_MESSAGE;
                        }
                        else {
                        */
                            state = WATCHING_NEW_MESSAGES;
                        //}
                        fprintf(stderr, "\r                                              ");
                    }
                    else {
                        fprintf(stderr, ".");
                    }
                }

                #if defined (WIN32)
                    Sleep(250);
                #else
                    usleep(250*1000);
                #endif
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
