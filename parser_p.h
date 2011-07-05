#ifndef PARSER_P_H
#define PARSER_P_H

#define TYPE_MESSAGE      0
#define TYPE_OPENING_TAG  1
#define TYPE_CLOSING_TAG  2
#define TYPE_NICK         3

#include "attribute.h"
#include "clist.h"
#include "cstring.h"

typedef struct rule_struct {
	char *tag;
	char *value;
	char *start;
	char *stop;
} rule;

typedef struct config_line_struct {
	char *context;
	clist *rules;
} config_line;

typedef struct parser_config_private_struct {
	clist *config_lines;
} parser_config_private;

typedef struct message_part_struct message_part;
struct message_part_struct {
	int type; // (0 or 1 or 2: 0 is text, 1 is <>, 2 is </>)
	char *data;
	message_part *link; // (NULL for text)
	clist *attributes; // (NULL for text)
};

/**
 * Process and return the message_parts found in this message.
 * return:
 * 	A list of message_part*
 */
clist *get_parts(clist *config_linrd, char *message);

clist_node *process_part(clist *config_lines, char *data, int text);

/**
 * Process a message_part and return the replacement string according to the rules in parser_config.
 * groups:
 * 	the list of config_lines
 * context_stack:
 * 	the context stack (a list of char*)
 */
char *process_message_part(clist *config_lines, clist *context_stack, message_part *part);

/**
 * Process a message against a single config_line.
 *
 * Process the message against a signle config_line. It will be called by process_message on each
 * config_line for each message_part.
 *
 * @param out the cstring on which to work
 * @param config_line the configuration line to test against
 * @param context_stack the context stack (list of char *)
 * @param part the message to process
 * 
 */
void process_message_part_sub(cstring *out, config_line *line, clist *context_stack, message_part *part);

clist_node *new_string_node(char *string);
clist_node *new_group_node(config_line *config);
clist_node *new_rule_node(rule *rule);

void free_message_part(message_part* message);
void free_message_part_node(clist_node* node);
void free_rule_node(clist_node *node);
void free_group_node(clist_node *node);

int filter_config(attribute *att);
#endif
