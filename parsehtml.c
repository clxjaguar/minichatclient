/*
  Name:               parsehtml.c
  Author:             Nejaa & cLx
  Date:               13/04/11
  Description:        Extrait d'une page HTML d'un minichat les messages et autres infos associées
  Fonction à appeler: parse_minichat_mess(char *input, unsigned int bytes);
  Copyright:          cc-by-nc 2011

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "entities.h"
#include "parsehtml.h"
#include "parser.h"
#include "display_interfaces.h"

typedef enum {
    READY=0,
    IN_MSG_ID,
    LOOKING_FOR_PROFILE_URL,
    IN_PROFILE_URL,
    LOOKING_FOR_USERICON_URL,
    IN_USERICON_URL,
    LOOKING_FOR_MESSAGE,
    IN_MESSAGE,
    LOOKING_FOR_USERNAME,
    IN_USERNAME
} tstate;

parser_config *config = NULL;

#define FREE(p); if (p != NULL) { free(p); p = NULL; }
//#define DEBUG

int parser_freerules(void){
	if (config != NULL){
		free_parser_config(config);
		config = NULL;
		return 1;
	}
	return 0;
}

int parser_loadrules(void){
	parser_freerules();
	config = get_parser_config("parser_rules.conf");
	if (config != NULL) { return 0; }
	else { return 1; }
}

unsigned int parse_minichat_mess(char input[], unsigned int bytes, message_t *msg, int reset){
    unsigned int i = 0;
    static unsigned int j;
    static tstate state;
    static int nbmessages = 0;
    //char *ptmp = NULL;
        
    const char str1[] = "<div id=\"mess";
    const char str2[] = "<a href=\".";
    const char str3[] = "<img src=\"";
    const char str4[] = "<div class=\"avatarMessage mChatMessage\">";
    const char str5[] = "</div>";

    static unsigned int o = 0;
    static char buffer[4000]; //TODO: rendre ce truc dynamique
#ifdef DEBUG
    static tstate oldstate = READY;
#endif

    if (reset) { 
        state = READY;
        j=0; 
        o=0;
        nbmessages = 0;
        FREE(msg->username);
        FREE(msg->message);
        FREE(msg->usericonurl);
        FREE(msg->userprofileurl);
	}

#ifdef DEBUG
	else {
		fprintf(stderr, "[CONT]"); 
	}
#endif

    if (o>=sizeof(buffer)) { o--; display_debug("Buffer full when parsing responses !", 0); }

    for (i=0; i<bytes; i++){
#ifdef DEBUG      
		if (state != oldstate) {
			fprintf(stderr, "[S%u=>S%u]", oldstate, state); 
			oldstate = state;
		}
		fprintf(stderr, "%c", input[i]); 
#endif
        if (input[i] == '\r') { continue; }
        if (input[i] == '\n') { continue; }
        
        switch(state){
            case READY:
                if (input[i] == str1[j++]) {
                    if (j >= strlen(str1)) { 
                        j=0;
                        o=0;
                        state = IN_MSG_ID; 
                    }
                }
                else { j=0; }
                break;
                 
            case IN_MSG_ID:
                if (input[i] == '\"') {
                    msg->msgid[j] = '\0'; j = 0;
                    state = LOOKING_FOR_USERICON_URL;
                }
                else { 
                    if (j>=20){ j-=5; } // very ugly but can save the day !
                    msg->msgid[j++] = input[i];
                }
                break;
                
            case LOOKING_FOR_USERICON_URL:
                if (input[i] == str3[j++]) {
                    if (j >= strlen(str3)) { 
                        j=0; 
                        o=0;
                        state = IN_USERICON_URL; 
                    }
                }
                else { j=0; }
                break;
            
            case IN_USERICON_URL:
                if (input[i] == '\"'){
                    buffer[o] = '\0';
                    FREE(msg->usericonurl);
                    msg->usericonurl = malloc((o+1)*sizeof(char));
                    //strcpy(msg->usericonurl, buffer);
                    decode_html_entities_utf8(msg->usericonurl, buffer);
                    state = LOOKING_FOR_PROFILE_URL;
                }
                else {
                    buffer[o++] = input[i];
                }
                break;
                 
            case LOOKING_FOR_PROFILE_URL:
                if (input[i] == str2[j++]) {
                    if (j >= strlen(str2)) { 
                        j=0; 
                        o=0; buffer[o++] = '.';
                        state = IN_PROFILE_URL; 
                    }
                }
                else { j=0; }
                break;
                 
            case IN_PROFILE_URL:
                if (input[i] == '"') {
                    buffer[o] = '\0';
                    FREE(msg->userprofileurl);
                    msg->userprofileurl = malloc((o+1)*sizeof(char));
                    //strcpy(msg->userprofileurl, buffer);
                    decode_html_entities_utf8(msg->userprofileurl, buffer);
                    state = LOOKING_FOR_USERNAME;
                }
                else {
                    buffer[o++] = input[i];
                }
                break;
                
            case LOOKING_FOR_USERNAME:
                if (input[i] == '>'){
                    o=0;
                    state = IN_USERNAME;
                }
                break;
                 
            case IN_USERNAME:
                if (input[i] == '<'){
                    buffer[o] = '\0';
                    FREE(msg->username);
                    msg->username = malloc((o+1)*sizeof(char));
                    //strcpy(msg->username, buffer);
                    decode_html_entities_utf8(msg->username, buffer);
                    nbmessages++;
                    state = LOOKING_FOR_MESSAGE;
                }
                else {
                    buffer[o++] = input[i];
                }
                break;
                                 
            case LOOKING_FOR_MESSAGE:
                if (input[i] == str4[j++]) { 
                    if (j >= strlen(str4)) { 
                        j=0; 
                        o=0;
                        state = IN_MESSAGE; 
                    }
                }
                else { j=0; }
                break;
                
            case IN_MESSAGE:
                buffer[o++] = input[i];
                if (input[i] == str5[j++]) {
                    if (j >= strlen(str5)) {
                        o-=strlen(str5);
                        buffer[o] = '\0';
                        FREE(msg->message);
                        msg->message = malloc((o+1)*sizeof(char));

						{
							char *buffer2 = NULL;
							if (config != NULL){
								clist *parts = get_parser_parts(buffer);
								buffer2 = parse_html_in_message(parts, config);
								free_clist(parts);
							}
							
							if (buffer2 != NULL){
								decode_html_entities_utf8(msg->message, buffer2); //strcpy(msg->message, buffer2);
								free(buffer2);
							}
							else {
								decode_html_entities_utf8(msg->message, buffer); //strcpy(msg->message, buffer);
							}
						}
						minichat_message(msg->username, msg->message, msg->usericonurl, msg->userprofileurl);
						state = READY;
                    }
                }
                else { j=0; }
                break;
                 
        }
    }
    return nbmessages;
}
