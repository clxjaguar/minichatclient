/*
  Name:        cookies.c
  Copyright:   cLx (cc-by-nc) 2011
  Author:      cLx
  Date:        08/04/11 13:15
  Description: Un truc �crit en 1/2 heure pour stocker les cookies ^^'
               Qu'est ce que c'est pratique que Remove-Cookie n'existe pas :D
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cookies.h"


int debug = 1;

int storecookie(cookie_t *cookies, char* name, char* value){
    int i;
    for (i=0; i<MAXCOOKIES; i++){
        if (cookies[i].name == NULL){
            // oh, a free location. we'll store it here!
            if (debug) fprintf(stderr, "Cookie slot %d was free. Storing \"%s\" here, its value is \"%s\"\n", i, name, value);
            cookies[i].name = malloc((strlen(name)+1)*sizeof(char));
            strcpy(cookies[i].name, name);
            if (cookies[i].value) { free(cookies[i].value); }
            cookies[i].value = malloc((strlen(value)+1)*sizeof(char));
            strcpy(cookies[i].value, value);
            break;
        }
        else {
            if (!strcmp(cookies[i].name, name)) {
                // match!
                if (debug) fprintf(stderr, "Cookie slot %d already containing \"%s\". We change \"%s\" for \"%s\".\n", i, name, cookies[i].value, value);
                free(cookies[i].name);
                cookies[i].name = malloc((strlen(name)+1)*sizeof(char));
                strcpy(cookies[i].name, name);

                if (cookies[i].value) { free(cookies[i].value); }
                cookies[i].value = malloc((strlen(value)+1)*sizeof(char));
                strcpy(cookies[i].value, value);
                break;
            }
        }
    }

    return 1; // out of space !
}

int listcookies(cookie_t *cookies){
    int i;
    printf("\nArray of cookies stored at %p (%d bytes) :\n", cookies, sizeof(cookies));
    for (i=0; i<MAXCOOKIES; i++){
        if (cookies[i].name == NULL && cookies[i].value == NULL) {
            if (!debug) { continue; }
        }
        printf("Slot %d (%p): ", i, &cookies[i]);
        if (cookies[i].name == NULL && cookies[i].value == NULL) {
            if (debug) {
                printf("[UNOCCUPED]\n");
            }
        }
        else {
             printf("%s\t%s\n", cookies[i].name, cookies[i].value);
        }
    }

    return 0;
}

char* getcookie(cookie_t *cookies, char *name){
    int i;
    for (i=0; i<MAXCOOKIES; i++){
        if (cookies[i].name == NULL){
            return NULL;
        }
        else if (!strcmp(cookies[i].name, name)){
             return cookies[i].value;
        }
    }
    return NULL;
}

int generate_cookies_string(cookie_t *cookies, char *buf, int maxbuf){
    int i;
    buf[0] = 0;

    for (i=0; i<MAXCOOKIES; i++){
        if (cookies[i].name != NULL && cookies[i].value != NULL) {
            if (i) { strncat(buf, "; ", maxbuf); }
            strncat(buf, cookies[i].name, maxbuf);
            strncat(buf, "=", maxbuf);
            strncat(buf, cookies[i].value, maxbuf);
        }
    }
    return 0;
}

typedef enum {
    WAITINGFORNEWLINE = 0,
    IN_HEADER,
    HEADER_FOUND,
    IN_COOKIENAME,
    COOKIE_NAME_STORED,
    IN_COOKIE_VALUE,
    ENDOFHTTPHEADERS
} tstate;

#define MAXBUFNAME  50
#define MAXBUFVALUE 200

int parsehttpheadersforgettingcookies(cookie_t *cookies, const char *string) {
    const char header[] = "Set-Cookie:";
    int i, j;
    char bufname[MAXBUFNAME+1], bufvalue[MAXBUFVALUE+1];
    tstate state = WAITINGFORNEWLINE;

    i=0; // index de "string"
    j=0; // index pour la chaine � d�tecter
    while(string[i]) {
        switch(state){
            case WAITINGFORNEWLINE:
                 if (string[i] == 10) { //line feed
                     state = IN_HEADER;
                 }
                 break;

            case IN_HEADER:
                 if (string[i] == 10) { //line feed
                     state = ENDOFHTTPHEADERS;
                 }
                 else if (string[i] == 13) { ; } // cr ?
                 else if (string[i] != header[j++]) {
                     j=0;
                     state = WAITINGFORNEWLINE;
                 }
                 else {
                     if (j==sizeof(header)-1) {
                         j=0;
                         state = HEADER_FOUND;
                     }
                 }
                 break;

            case HEADER_FOUND:
                 if (string[i] == ' ') { ; } // rien
                 else if (string[i] == '\n') { state = IN_HEADER; } // caract�res invalides
                 else if (string[i] < '0') { state = WAITINGFORNEWLINE; } // caract�res invalides
                 else if (string[i] == ';') { state = WAITINGFORNEWLINE; } // pas encore!
                 else {
                     bufname[j++] = string[i];
                     state = IN_COOKIENAME;
                 }
                 break;

            case IN_COOKIENAME:
                 if (string[i] < ' ' || string[i] == ';') { // caract�res invalides ou fin de cookie pr�matur�e
                     bufname[j++] = 0; j=0;
                     if (debug) fprintf(stderr, "I got the half of a cookie ?!\n");
                     storecookie(cookies, bufname, "");
                     state = WAITINGFORNEWLINE;
                     if (string[i] == '\n') { state = IN_HEADER; }
                 }
                 else if (string[i] != '=') {
                     if (j >= MAXBUFNAME) {
                           bufname[j++] = 0; j=0;
                           fprintf(stderr, "\rOOOOPPs! Cookie name too long ! Begins with: \"%s\"\n", bufname);
                           state = WAITINGFORNEWLINE;
                     }
                     else {
                         bufname[j++] = string[i];
                     }
                 }
                 else {
                     bufname[j++] = 0; j=0;
                     state = IN_COOKIE_VALUE;
                 }
                 break;

            case IN_COOKIE_VALUE:
                 if (string[i] < ' ' || string[i] == ';') { // fin du contenu du cookie
                     bufvalue[j] = 0; j=0;
                     //if (debug) fprintf(stderr, "Yummy, I got a nice cookie !\n");
                     storecookie(cookies, bufname, bufvalue);
                     state = WAITINGFORNEWLINE;
                     if (string[i] == '\n') { state = IN_HEADER; }
                 }
                 else {
                     if (j >= MAXBUFVALUE) {
                           bufvalue[j++] = 0; j=0;
                           fprintf(stderr, "\rOOOOPPs! Cookie \"%s\"'s value is too long ! Begins with: \"%s\"\n", bufname, bufvalue);
                           state = WAITINGFORNEWLINE;
                     }
                     else {
                         bufvalue[j++] = string[i];
                     }
                 }
                 break;

            case ENDOFHTTPHEADERS:
                 // dead end !
                 //printf("\n*** END OF HEADERS ***");
                 return 0;
                 break;

            default:
                 state = WAITINGFORNEWLINE;
        }
        i++;
    }
    return 0;
}

int freecookies(cookie_t *cookies){
    int i;
    for (i=0; i<MAXCOOKIES; i++){
        if(cookies[i].name)  { free(cookies[i].name);  cookies[0].name  = NULL; }
        if(cookies[i].value) { free(cookies[i].value); cookies[0].value = NULL; }
    }
    return 0;
}