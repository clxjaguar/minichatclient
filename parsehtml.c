/*
  Name:               parsehtml.c
  Author:             cLx - Surtout ne pas toucher si vous voulez pas mettre tout cela en panne !
  Date:               13/04/11
                      15/02/14
  Description:        Extrait des pages HTML de mchat.php les messages et autres infos associ�es
  Fonction a appeler: parse_minichat_mess(char *input, unsigned int bytes);
  Copyright:          cc-by-nc 2011

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "entities.h"
#include "parsehtml.h"
#include "nicklist.h"
#include "parser.h"
#include "display_interfaces.h"
#include "main.h"
#include "strfunctions.h"

typedef enum {
	READY=0,
	IN_MSG_ID,
	LOOKING_FOR_PROFILE_URL,
	IN_PROFILE_URL,
	LOOKING_FOR_USERICON_URL,
	IN_USERICON_URL,
	LOOKING_FOR_MESSAGE,
	IN_MESSAGE,
	IN_MESSAGE_AND_IN_DIV,
	LOOKING_FOR_USERNAME,
	IN_USERNAME,
	LOOKING_FOR_STATS,
	LOOKING_FOR_USERS,
	LOOKING_FOR_USERS_IN_A,
	LOOKING_FOR_USERS_IN_A_HREF,
	LOOKING_FOR_USERS_IN_USERNAME
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
	config = parser_get_config("parser_rules.conf");
	if (config != NULL) { return 0; }
	else { return 1; }
}

int watchfor(const char *string, const char c, unsigned int *counter){
	if (c != string[(*counter)++]) {
		*counter=0; return 0; // no!
	}
	if (*counter >= strlen(string)) {
		*counter=0; return 1; // yes!
	}
	return 0; // not yet ?
}

// special for twin strings, use with caution ! if you're not sure, use two normal watchfor()
int watchfor2(const char *str1, const char *str2, const char c, unsigned int *counter){
	if ((c == str1[*counter])||(c == str2[*counter])) {
		(*counter)++;
		if ((*counter >= strlen(str1))||(*counter >= strlen(str2))) {
			*counter = 0; return 1; // yes!
		}
		return 0; // not yet?
	}
	else {
		*counter = 0; return 0; // no!
	}
}

unsigned int parse_minichat_mess(char input[], signed int bytes, message_t *msg, int reset){
	unsigned int i = 0;
	static unsigned int j, l;
	static tstate state;
	static unsigned int nbmessages = 0;
	static unsigned int o = 0;
	static char buffer[2000], *url=NULL, *tmp=NULL;
	static tstate oldstate = READY;

	if (reset) {
		state = READY;
		j=0; l=0;
		o=0;
		nbmessages = 0;
		FREE(msg->username);
		FREE(msg->message);
		FREE(msg->usericonurl);
		FREE(msg->userprofileurl);
		FREE(url);
	}

#ifdef DEBUG
	else {
		fprintf(stderr, "[CONT]");
	}
#endif

	if (bytes<=0) { return 0; }
	for (i=0; i<(unsigned int)bytes; i++){
		if (state != oldstate) {
			j=0; l=0; //reset watchfor() counters
#ifdef DEBUG
			fprintf(stderr, "[S%u=>S%u]", oldstate, state);
#endif
			oldstate = state;
		}
#ifdef DEBUG
		fprintf(stderr, "%c", input[i]);
#endif

		// ignore carrier return and line feed from the input buffer
		if (input[i] == '\r' || input[i] == '\n') { continue; }

		switch(state){
			default:
			case READY:
				// did we got a message ?
				if (watchfor("<div id=\"mess", input[i], &j)){
					o=0;
					state = IN_MSG_ID;
				}

				// or it is the "stats" part of the page ?
				if (watchfor2("<div id=\"mChatStats\" class=\"mChatStats\">", "<div class=\"mChatStats\" id=\"mChatStats\">", input[i], &l)){
					o=0;
					state = LOOKING_FOR_STATS;
				}
				break;

			case IN_MSG_ID:
				if (input[i] != '\"') {
					if (j>=20){ j-=5; } // very ugly but can save the day !
					msg->msgid[j++] = input[i];
				}
				else {
					msg->msgid[j] = '\0'; //j=0;
					state = LOOKING_FOR_USERICON_URL;
				}
				break;

			case LOOKING_FOR_USERICON_URL:
				if (watchfor("<img src=\"", input[i], &j)){
					o=0;
					state = IN_USERICON_URL;
				}
				break;

			case IN_USERICON_URL:
				if (input[i] == '\"'){
					buffer[o] = '\0';
					FREE(msg->usericonurl);
					msg->usericonurl = malloc(o+1);
					decode_html_entities_utf8(msg->usericonurl, buffer); // work like strcpy()
					state = LOOKING_FOR_PROFILE_URL;
				}
				else {
					buffer[o++] = input[i];
				}
				break;

			case LOOKING_FOR_PROFILE_URL:
				if (watchfor("<a href=\".", input[i], &j)){
					o=0; buffer[o++] = '.';
					state = IN_PROFILE_URL;
				}
				break;

			case IN_PROFILE_URL:
				if (input[i] == '"') {
					buffer[o] = '\0';
					FREE(msg->userprofileurl);
					msg->userprofileurl = malloc(o+1);
					decode_html_entities_utf8(msg->userprofileurl, buffer); // work like strcpy()
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
					msg->username = malloc(o+1);
					decode_html_entities_utf8(msg->username, buffer); // work like strcpy()
					nbmessages++;
					state = LOOKING_FOR_MESSAGE;
				}
				else {
					buffer[o++] = input[i];
				}
				break;

			case LOOKING_FOR_MESSAGE:
				if (watchfor("<div class=\"avatarMessage mChatMessage\">", input[i], &j)){
					o=0;
					state = IN_MESSAGE;
				}
				break;

			case IN_MESSAGE_AND_IN_DIV:
				buffer[o++] = input[i];
				if (watchfor("</div>", input[i], &j)){
					state = IN_MESSAGE;
				}
				break;

			case IN_MESSAGE:
				buffer[o++] = input[i];
				if (watchfor("</div>", input[i], &j)){
					o-=(unsigned int)strlen("</div>");
					buffer[o] = '\0';
					FREE(msg->message);

					// si le parseur de niki est activé, on l'utilise
					tmp = NULL;
					if (config){ tmp = parse_html_in_message(buffer, config); }
					if (tmp){
						msg->message = malloc(strlen(tmp)+1);
						decode_html_entities_utf8(msg->message, tmp); //strcpy-like
						free(tmp);
					}
					else {
						msg->message = malloc((o+1));
						decode_html_entities_utf8(msg->message, buffer); // strcpy-like
					}

					minichat_message(msg->username, msg->message, msg->usericonurl, msg->userprofileurl);
					state = READY;
				}

				if (watchfor2("<div>", "<div ", input[i], &l)){
					state = IN_MESSAGE_AND_IN_DIV;
				}
				break;

			case LOOKING_FOR_STATS:
				buffer[o++] = input[i];

				// check for the end of the topic line, and start of the nicklist
				if (watchfor("<br />", input[i], &l)){
					buffer[o++] = '\0';
					html_strip_tags(buffer);
					tmp = malloc(o);
					decode_html_entities_utf8(tmp, buffer);

					// sanitize topic a little bit...
					strrep(NULL, &tmp, "  ", " ");
					strrep(NULL, &tmp, "( ", "(");
					strrep(NULL, &tmp, " )", ")");
					strrep(NULL, &tmp, "  ", " ");
					strrep(NULL, &tmp, "  ", " ");

					// and display/memorize it.
					nicklist_topic(tmp);
					free(tmp);

					// clear nicklist list prior to relist
					nicklist_recup_start();
					o=0;
					state = LOOKING_FOR_USERS;
				}

				if (watchfor("</div>", input[i], &j)){
					o-=(unsigned int)strlen("</div>");
					buffer[o++] = 0;
					tmp = malloc(o); o=0;
					decode_html_entities_utf8(tmp, buffer);
					nicklist_topic(tmp);
					free(tmp);

					// ok, if we don't do that the nicklist will never be showed empty when is no more users in the chat
					nicklist_recup_start();
					nicklist_recup_end();
					state = READY;
				}
				break;

			case LOOKING_FOR_USERS:
				if (watchfor("</div>", input[i], &j)){
					nicklist_recup_end();
					state = READY;
				}

				if (watchfor("<a ", input[i], &l)){
					o=0;
					state = LOOKING_FOR_USERS_IN_A;
				}
				break;

			case LOOKING_FOR_USERS_IN_A:
				if (input[i] == '>') { 
					o=0;
					state = LOOKING_FOR_USERS_IN_USERNAME;
				}

				if (watchfor("href=\"", input[i], &l)){
					o=0;
					state = LOOKING_FOR_USERS_IN_A_HREF;
				}
				break;

			case LOOKING_FOR_USERS_IN_A_HREF:
				if (input[i] == '"') {
					// we got a profile URL in the mode=stats or global page
					buffer[o] = '\0';
					FREE(url);
					url = malloc(o+1);
					decode_html_entities_utf8(url, buffer);
					o=0;
					state = LOOKING_FOR_USERS_IN_A;
				}
				else {
					buffer[o++] = input[i];
				}
				break;

			case LOOKING_FOR_USERS_IN_USERNAME:
				if (input[i] == '<') {
					buffer[o] = '\0';
					nicklist_recup_name(buffer, url);
					FREE(url);
					state = LOOKING_FOR_USERS;
				}
				else {
					buffer[o++] = input[i];
				}
				break;
		}
		if (o>=sizeof(buffer)) { o=0; display_debug("parsehtml.c output buffer is full, sorry.", 0); }
	}
	return nbmessages;
}
