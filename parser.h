#ifndef PARSER_H
#define PARSER_H

#include "clist.h"

#define TYPE_MESSAGE      0
#define TYPE_OPENING_TAG  1
#define TYPE_CLOSING_TAG  2

typedef struct {
	void *data;
} parser_config;

typedef struct parser_message_struct parser_message;
struct parser_message_struct {
	int type; // (0 or 1 or 2: 0 is text, 1 is <>, 2 is </>)
	char *data;
	parser_message *link; // (NULL for text)
	clist *attributes; // (NULL for text)
};

typedef struct {
	char *tag;
	char *value;
	char *start;
	char *stop;
} parser_rule;

/**
 * Process and return the parser_messages found in this message.
 * They will be fully checked against all special rules and such.
 * Don't forget to call free_clist() on it when done.
 *
 * @param message the message to work on
 *
 * @return a list of parser_message's
 */
clist *parser_get_parts(const char *message);

/**
 * Locked... Thanks...
 */
char *parse_html_in_message(const char *message, parser_config *config);

/**
 * Parse the given message (HTML) and return a new, parsed string.
 * This is a helper function to parser_parse_messages() which also create
 * and return a string with the parsed message.
 *
 * @param message the message to parse
 * @param config the configuration to use to parse the parser_message's
 *
 * @return the parsed string
 */
char *parser_parse_html(const char *message, parser_config *config);

/**
 * Parse the given parser_message's in a separate, given function.
 * It will call (operation) on each parser_message, telling you which rule apply
 * to this part, if any.
 *
 * @param parts the parser_message's to parse
 * @param config the configuration to use to parse the parser_message's
 * @param operation the function to use to parse each parser_message
 * @param argument the user argument to pass to the function
 */
void parser_parse_messages(clist *parts, parser_config *config, 
	void (*operation)(parser_message *part, parser_rule *rul, void *argument),
	void *argument);

/**
 * Parse the given file for an output configuration.
 * Don't forget free_parser_config().
 *
 * @param filename the file to open
 *
 * @return the configuration object
 */
parser_config *parser_get_config(const char filename[]);

/**
 * Free the parser_config.
 *
 * @param config the configuration object
 */
void free_parser_config(parser_config *config);

#endif
