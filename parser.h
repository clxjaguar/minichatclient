#ifndef PARSER_H
#define PARSER_H

typedef struct {
	void *data;
} parser_config;

/* Parse the given message (HTML) and return a clean, neat, text only version.
 * This is a helper function to parser_parse_messages() which also create
 * and return a string with the parsed message.
 * AND PERMIT TO DON'T INCLUDE ANOTHER .H WITH THAT .H !!!
 *
 * @param message the message to parse
 * @param config the configuration to use to parse the parser_message's
 * @return the parsed string */
char *parse_html_in_message(const char *message, parser_config *config);

/* Parse the given file for an output configuration.
 * Don't forget free_parser_config().
 *
 * @param filename the file to open
 * @return the configuration object */
parser_config *parser_get_config(const char filename[]);

/* Free the parser_config.
 *
 * @param config the configuration object */
void free_parser_config(parser_config *config);
#endif
