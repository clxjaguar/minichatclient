#ifndef PARSER_H
#define PARSER_H

/**
 * Parse the given HTML string and return a new, parsed string.
 * Don't forget to free() it after use !
 */
char *parse_html_for_output(char *message);

#endif
