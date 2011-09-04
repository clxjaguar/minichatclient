#ifndef PARSER_H
#define PARSER_H

#include "clist.h"

#define TYPE_MESSAGE      0
#define TYPE_OPENING_TAG  1
#define TYPE_CLOSING_TAG  2

typedef struct {
	void *data;
} parser_config;

typedef struct message_part_struct message_part;
struct message_part_struct {
	int type; // (0 or 1 or 2: 0 is text, 1 is <>, 2 is </>)
	char *data;
	message_part *link; // (NULL for text)
	clist *attributes; // (NULL for text)
};

typedef struct {
	char *tag;
	char *value;
	char *start;
	char *stop;
} parser_rule;

/**
 * Process and return the message_parts found in this message.
 * They will be fully checked against all special rules and such.
 * Don't forget to call free_clist() on it when done.
 *
 * @param message the message to work on
 *
 * @return a list of message_part's
 */
clist *get_parser_parts(const char *message);

/**
 * Parse the given message_part's and return a new, parsed string.
 * This is a helper function to parse_html() which also create
 * and return a string with the parsed message.
 *
 * @param parts the message_part's to parse
 * @param config the configuration to use to parse the message_part's
 *
 * @return the parsed string
 */
char *parse_html_in_message(clist *parts, parser_config *config);

/**
 * Parse the given message_part's in a separate, given function.
 * It will call (operation) on each message_part, telling you which rule apply
 * to this part, if any.
 *
 * @param parts the message_part's to parse
 * @param config the configuration to use to parse the message_part's
 * @param operation the function to use to parse each message_part
 * @param argument the user argument to pass to the function
 */
void parse_html(clist *parts, parser_config *config, void (*operation)(message_part *part, parser_rule *rul, void *argument), void *argument);

/**
 * Parse the given file for an output configuration.
 * Don't forget free_parser_config().
 *
 * @param filename the file to open
 *
 * @return the configuration object
 */
parser_config *get_parser_config(const char filename[]);

/**
 * Free the parser_config.
 *
 * @param config the configuration object
 */
void free_parser_config(parser_config *config);

#endif
