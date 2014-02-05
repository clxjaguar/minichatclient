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

#include "nicklist.h"
#include "display_interfaces.h"
#include "mccirc.h"
#include "main.h" //malloc_globalise_url(), get_mccirc()

typedef struct {
	char *username;
	char *ident;
	char *profil_url;
	char *icon_url;
	time_t added;
	time_t lastmessage;
} tlist;

void nicklist_init(void) {
}

void nicklist_destroy(void) {
}

void nicklist_msg_update(const char *username, const char *profil_url, const char *icon_url) {
	char *profil_url2, *icon_url2;

	icon_url2 = malloc_globalise_url(icon_url);
	profil_url2 = malloc_globalise_url(profil_url);

	display_debug(profil_url2, 0);
	display_debug(icon_url2, 0);

	free(profil_url2);
	free(icon_url2);
}

void nicklist_recup_start(void) {
	display_nicklist(NULL);
	mccirc_nicks_start(get_mccirc());
}

void nicklist_recup_end(void) {
	mccirc_nicks_stop(get_mccirc());
}

void nicklist_recup_name(const char* nickname) {
	display_nicklist(nickname);
	mccirc_nicks_add(get_mccirc(), nickname);
}

void nicklist_topic(const char *string){
	display_statusbar(string);
	mccirc_topic(get_mccirc(), string);
}
