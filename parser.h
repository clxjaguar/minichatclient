#ifndef PARSER_H
#define PARSER_H

#include "clist.h"

typedef struct {
	void *data;
} parser_config;

/**
 * Parse the given HTML string and return a new, parsed string.
 * Don't forget to free() it after use !
 */
char *parse_html_in_message(char *message, parser_config *config);

/**
 * Parse the given file for an output configuration.
 */
parser_config *get_parser_config(char filename[]);

void free_parser_config(parser_config *pconfig);

#endif
