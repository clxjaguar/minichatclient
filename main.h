// CE FICHIER EST UNIQUEMENT LA POUR APPELER LES FONCTIONS QUI SONT DANS MAIN.C

void minichat_message(const char *username, const char *message, const char *usericonurl, const char *userprofileurl);
char *malloc_globalise_url(const char *url);
void force_polling(void);

#include "mccirc.h"
// Cet include est la uniquement pour avoir le type mccirc afin de faire...
mccirc *get_mccirc(void);
// ... et de recuperer la variable "irc" qui est dans le main.
