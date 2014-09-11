// CE FICHIER EST UNIQUEMENT LA POUR APPELER LES FONCTIONS QUI SONT DANS MAIN.C

#include "commons.h"

//void minichat_message(const char *username, const char *message, const char *usericonurl, const char *userprofileurl);
void minichat_message(const message_t *msg);
char *malloc_globalise_url(const char *url);
void force_polling(void);
