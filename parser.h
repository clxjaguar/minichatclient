#ifndef PARSER_H
#define PARSER_H

typedef struct parser_config_struct parser_config;

/**
 * Parse the given HTML string and return a new, parsed string.
 * Don't forget to free() it after use !
 */
char *parse_html_for_output(char *message, parser_config *config);

/**
 * Parse the given file for an output configuration.
 */
parser_config *parser_get_config(char filename[]);

#endif
