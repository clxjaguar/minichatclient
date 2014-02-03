/*
  Name:               parsehtml.c
  Author:             cLx - Surtout ne pas toucher si vous voulez pas mettre tout cela en panne !
  Date:               13/04/11
  Description:        Extrait des pages HTML de mchat.php les messages et autres infos associ�es
  Fonction � appeler: parse_minichat_mess(char *input, unsigned int bytes);
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
	LOOKING_FOR_STATS_BUT_IN_TAG,
	LOOKING_FOR_USERS,
	LOOKING_FOR_USERS_IN_A,
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

unsigned int parse_minichat_mess(char input[], signed int bytes, message_t *msg, int reset){
	unsigned int i = 0;
	static unsigned int j, l;
	static tstate state;
	static unsigned int nbmessages = 0;

	const char str1[] = "<div id=\"mess";
	const char str2[] = "<a href=\".";
	const char str3[] = "<img src=\"";
	const char str4[] = "<div class=\"avatarMessage mChatMessage\">";
	const char str5[] = "</div>";
	const char str6[] = "<div id=\"mChatStats\" class=\"mChatStats\">";
	const char str7[] = "<div class=\"mChatStats\" id=\"mChatStats\">";
	const char str8[] = "<br />";
	const char str9[] = "<a ";
	const char str0a[]= "<div>";
	const char str0b[]= "<div ";

	static unsigned int o = 0;
	static char buffer[4000];
#ifdef DEBUG
	static tstate oldstate = READY;
#endif

	if (reset) {
		state = READY;
		j=0; l=0;
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

	if (bytes<=0) { return 0; }
	if (o>=sizeof(buffer)) { o--; display_debug("Buffer full when parsing responses !", 0); }

	for (i=0; i<(unsigned int)bytes; i++){
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
			default:
			case READY:
				// message ?
				if (input[i] == str1[j++]) {
					if (j >= strlen(str1)) {
						j=0;
						o=0;
						state = IN_MSG_ID;
					}
				}
				else { j=0; }

				// stats ?
				if ((input[i] == str6[l])||(input[i] == str7[l])) {
					if (++l >= strlen(str6)) {
						l=0;
						o=0;
						state = LOOKING_FOR_STATS;
					}
				}
				else { l=0; }
				//{char c[3]; c[0]=input[i];c[1]=0; c[2]=0;if(l) { c[1] = '0'+l; } display_debug(c, i!=0);}

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
						j=0; l=0;
						o=0;
						state = IN_MESSAGE;
					}
				}
				else { j=0; }
				break;

			case IN_MESSAGE_AND_IN_DIV:
				buffer[o++] = input[i];
				if (input[i] == str5[j++]) {
					if (j >= strlen(str5)) {
						j=0; l=0;
						state = IN_MESSAGE;
					}
				}
				else { j=0; }
				break;

			case IN_MESSAGE:
				buffer[o++] = input[i];
				if (input[i] == str5[j++]) {
					if (j >= strlen(str5)) {
						o-=(unsigned int)strlen(str5);
						buffer[o] = '\0';
						FREE(msg->message);
						{
							char *withouthtmlmessage = NULL;
							if (config != NULL){
								// si le parseur de niki est activé, on l'utilise
								withouthtmlmessage = parse_html_in_message(buffer, config);
							}

							if (withouthtmlmessage != NULL){
								msg->message = malloc(strlen(withouthtmlmessage)+1);
								decode_html_entities_utf8(msg->message, withouthtmlmessage); //strcpy-like
								free(withouthtmlmessage);
							}
							else {
								msg->message = malloc((o+1)*sizeof(char));
								decode_html_entities_utf8(msg->message, buffer); // strcpy-like
							}
						}

						minichat_message(msg->username, msg->message, msg->usericonurl, msg->userprofileurl);
						state = READY;
					}
				}
				else { j=0; }

				if (input[i] == str0a[l] || input[i] == str0b[l]){
					l++;
					if (l >= strlen(str0a)) {
						j=0; l=0;
						state = IN_MESSAGE_AND_IN_DIV;
					}
				}
				else { l=0; }

				break;

			case LOOKING_FOR_STATS:
			case LOOKING_FOR_STATS_BUT_IN_TAG:
				//{char c[2]; c[1]=0; c[0]=input[i]; display_debug(c, 1); }
				if (input[i] == '<') { state = LOOKING_FOR_STATS_BUT_IN_TAG; }
				else if (input[i] == '>') { state = LOOKING_FOR_STATS; }
				else if (state == LOOKING_FOR_STATS) { buffer[o++] = input[i]; }

				if (input[i] != str8[l++]) { l=0; }
				else {
					if (l >= strlen(str8)) {
						buffer[o++] = 0;
						{
							char *tmp = malloc(o);
							decode_html_entities_utf8(tmp, buffer);
							strrep(NULL, &tmp, "  ", " ");
							strrep(NULL, &tmp, "( ", "(");
							strrep(NULL, &tmp, " )", ")");
							strrep(NULL, &tmp, "  ", " ");
							strrep(NULL, &tmp, "  ", " ");
							nicklist_topic(tmp);
							free(tmp);
						}

						// clear nicklist list prior to relist
						nicklist_recup_start();

						state = LOOKING_FOR_USERS; o=0; l=0;
					}
				}
				if (input[i] != str5[j++]) { j=0; }
				else {
					if (j >= strlen(str5)) {
						buffer[o++] = 0;
						{
							char *tmp = malloc(o); o=0;
							decode_html_entities_utf8(tmp, buffer);
							display_statusbar(tmp);
							free(tmp);
						}

						// ok, if we don't do that the nicklist will never be showed empty when is no more users in the chat
						nicklist_recup_start();
						nicklist_recup_end();

						state = READY;

					}
				}
				break;

			case LOOKING_FOR_USERS:
				if (input[i] != str5[j++]) { j=0; }
				else if (j >= strlen(str5)){
					j=0; l=0;
					nicklist_recup_end();
					state = READY;
				}

				if (input[i] != str9[l++]) { l=0; }
				else if (l >= strlen(str9)){
					state = LOOKING_FOR_USERS_IN_A;
				}
				break;

			case LOOKING_FOR_USERS_IN_A:
				if (input[i] == '>') { state = LOOKING_FOR_USERS_IN_USERNAME; }
				j=0; l=0;
				break;

			case LOOKING_FOR_USERS_IN_USERNAME:
				if (input[i] == '<') {
					buffer[l] = '\0';

					nicklist_recup_name(buffer);
					state = LOOKING_FOR_USERS; j=0; l=0;
				}
				else {
					buffer[l++] = input[i];
				}
				break;
		}
	}
	return nbmessages;
}
