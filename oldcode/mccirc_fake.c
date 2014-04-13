/**
 * @file
 * @author niki
 * @date 20 Nov 2012
 */

#include "mccirc.h"

//////////////////////////////////
// constructors and destructors //
//////////////////////////////////

mccirc *mccirc_new() {
	return NULL;
}

void mccirc_free(mccirc *self) {
	if (self) {}
}

////////////////////////////
// other public functions //
////////////////////////////

void mccirc_init(mccirc *self, const char ffname[], const char server_name[], const char channel_name[], const char channel_topic[], int port) {
	if (self) {}
	if (ffname) {}
	if (server_name) {}
	if (channel_name) {}
	if (channel_topic) {}
	if (port) {}
}

const char *mccirc_check_message(mccirc *self) {
	if (self) {}
	return NULL;
}

void mccirc_chatserver_error(mccirc *self) {
	if (self) {}
}

void mccirc_chatserver_resume(mccirc *self) {
	if (self) {}
}

void mccirc_clear_nicklist(mccirc *self) {
	if (self) {}
}

void mccirc_add_nick(mccirc *self, const char name[]) {
	if (self) {}
	if (name) {}
}

void mccirc_remove_nick(mccirc *self, const char name[]) {
	if (self) {}
	if (name) {}
}

void mccirc_chatserver_message(mccirc *self, const char name[], const char message[]) {
	if (self) {}
	if (name) {}
	if (message) {}
}

void mccirc_set_topic_mode(mccirc *self, int mode) {
	if (self) {}
	if (mode) {}
}

void mccirc_topic(mccirc *self, const char topic[]) {
	if (self) {}
	if (topic) {}
}

void mccirc_nicks_start(mccirc *self) {
	if (self) {}
}

void mccirc_nicks_stop(mccirc *self) {
	if (self) {}
}

void mccirc_nicks_add(mccirc *self, const char username[]) {
	if (self) {}
	if (username) {}
}

