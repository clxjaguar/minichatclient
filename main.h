// CE FICHIER EST UNIQUEMENT LA POUR APPELER LES FONCTIONS QUI SONT DANS MAIN.C

void minichat_message(char *username, char *message, char *usericonurl, char *userprofileurl);

#include "mccirc.h"
// Cet include est la uniquement pour avoir le type mccirc afin de faire...
mccirc *get_mccirc(void);
// ... et de recuperer la variable "irc" qui est dans le main.
