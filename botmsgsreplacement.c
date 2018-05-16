#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include "pcre-src/pcreposix.h"
#else
#include <sys/types.h>
#include <regex.h>
#endif

#define FREE(p); if (p != NULL) { free(p); p = NULL; }

#include "commons.h"
#include "strfunctions.h"
#include "display_interfaces.h"
#include "botmsgsreplacement.h"

#define NMATCH 10
char* regex_mget_match(const char *string, const regmatch_t *pmatch) {
	char *p = NULL;
	size_t size;
	if (pmatch->rm_so != -1 && pmatch->rm_eo != -1) {
		size = (size_t)pmatch->rm_eo - (size_t)pmatch->rm_so;
		p = malloc(size+1);
		strncpy(p, &string[(size_t)pmatch->rm_so], size);
		p[pmatch->rm_eo - pmatch->rm_so] = '\0';
	}
	return p;
}


regex_t regex;
int ok = 0;

int botmsgsreplacement_init(void) {
	// https://regex101.com/r/3SzjWp/2/tests
	const char regex_string[] = "(<img.+? src=\"([^\"]+)\"[^>]*?>|).*?<span[^>]*?>(.+?)</span>: *(.+)$";

	if(regcomp(&regex, regex_string, REG_EXTENDED) != 0) {
		display_debug("Bot messages handler error: could not compile regex", 0);
		return -1;
	}
	ok=1;
	return 0;
}

int botmsgsreplacement_proceed(message_t *msg) {
	regmatch_t pmatch[NMATCH];
	int ret;

	if (!ok) { return -1; }

	if (!strcasecmp(msg->username, "K9_Discord")) {
		ret = regexec(&regex, msg->message, NMATCH, pmatch, 0);
		if (!ret) {
			supersede(&msg->usericonurl, regex_mget_match(msg->message, &pmatch[2]), FREE_OLD_POINTER);
			supersede(&msg->username,    regex_mget_match(msg->message, &pmatch[3]), FREE_OLD_POINTER);
			supersede(&msg->message,     regex_mget_match(msg->message, &pmatch[4]), FREE_OLD_POINTER);
		}
	}
	return 0;
}

int botmsgsreplacement_destroy(void) {
	/* Free memory allocated to the pattern buffer by regcomp() */
	regfree(&regex);
	ok = 0;
	return 0;
}
