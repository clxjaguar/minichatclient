/*
  Name:               parsehtml.c
  Author:             Nejaa & cLx
  Date:               13/04/11
  Description:        Extrait d'une page HTML d'un minichat les messages et autres infos associ�es
  Fonction � appeler: parse_minichat_mess(char *input, unsigned int bytes);
  Copyright:          cc-by-nc 2011

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parsehtml.h"

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

#define FREE(p) if (p != NULL) { free(p); p = NULL; }
#define DEBUG 0

unsigned int parse_minichat_mess(char input[], unsigned int bytes, message_t *msg, int reset){
    unsigned int i = 0;
    static int j;
    static tstate state;
        
    char str1[] = "<div id=\"mess";
    char str2[] = "<a href=\".";
    char str3[] = "<img src=\".";
    char str4[] = "<div class=\"avatarMessage mChatMessage\">";
    char str5[] = "</div>";
    
    //static char *username = NULL, *message = NULL, *usericonurl = NULL, *userprofileurl = NULL;
    static unsigned int o = 0;
    static char buffer[4000];
    
    if (reset) { 
        state = READY;
        j=0; 
        o=0;
        FREE(msg->username)
        FREE(msg->message)
        FREE(msg->usericonurl)
        FREE(msg->userprofileurl)
    }
    else {
        if (DEBUG) fprintf(stderr, "[CONT]"); 
    }

    if (o>=sizeof(buffer)) { o--; fprintf(stderr, "Buffer full when parsing responses !\n"); }

    for (i=0; i<bytes; i++){
        
        if (DEBUG) { fprintf(stderr, "%c", input[i]); }
        
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
                    //printf(" ");
                }
                else { 
                    if (j>=20){ j-=5; } // very ugly but can save the day !
                    msg->msgid[j++] = input[i];
                    //printf("%c", input[i]);
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
                    FREE(msg->usericonurl)
                    msg->usericonurl = malloc((o)*sizeof(char)+1);
                    strcpy(msg->usericonurl, buffer);
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
                        o=0;
                        state = IN_PROFILE_URL; 
                    }
                }
                else { j=0; }
                break;
                 
            case IN_PROFILE_URL:
                if (input[i] == '"') {
                    buffer[o] = '\0';
                    FREE(msg->userprofileurl)
                    msg->userprofileurl = malloc((o)*sizeof(char)+1);
                    strcpy(msg->userprofileurl, buffer);
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
                     FREE(msg->username)
                     msg->username = malloc((o)*sizeof(char)+1);
                     strcpy(msg->username, buffer);
                     state = LOOKING_FOR_MESSAGE;
                 }
                 else {
                     buffer[o++] = input[i];
                     //printf("%c", input[i]);
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
                        FREE(msg->message)
                        msg->message = malloc((o)*sizeof(char)+1);
                        strcpy(msg->message, buffer);
                        minichat_message(msg->username, msg->message, msg->usericonurl, msg->userprofileurl);
                        state = READY;
                    }
                }
                else { j=0; }
                break;
                 
        }
    }
    return 0;
}
