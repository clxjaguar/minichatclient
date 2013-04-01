#ifndef MAIN_MINICHAT_H
#define MAIN_MINICHAT_H

#include "mccirc.h"
#include "parser.h"

// haha, that file is so unusual !
void minichat_message(char* username, char* html, char *usericonurl, char *userprofileurl, parser_config *config);
//void minichat_users_at_this_moment(char *string);
//void main_start_nicks_update();
//void main_end_nicks_update();
//void main_add_nick(char *buffer);

mccirc *get_mccirc(void);

#endif

