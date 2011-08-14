#ifndef PARSER_P_H
#define PARSER_P_H

#include "attribute.h"

/*
 * This is the content of parser_config.
 */
typedef struct {
	clist *config_lines;
} parser_config_private;

typedef struct {
	char *tag;
	char *value;
	char *start;
	char *stop;
} rule;

typedef struct {
	char *context;
	clist *rules;
} config_line;

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
clist *get_parts(char *message);

clist_node *process_part(char *data, int text);

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
 * @param out:
 * 	the cstring on which to work
 * @param config_line:
 * 	the configuration line to test against
 * @param context_stack:
 * 	the context stack (list of char *)
 * @param part:
 *	the message to process
 * 
 * @return
 * 	true if the rule was used
 */
int process_message_part_sub(cstring *out, config_line *line, clist *context_stack, message_part *part);

clist_node *new_string_node(char *string);
clist_node *new_group_node(config_line *config);
clist_node *new_rule_node(rule *rule);

clist_node *clone_message_part_node(clist_node *message_node);

void free_message_part(message_part* message);
void free_message_part_node(clist_node* node);
void free_rule_node(clist_node *node);
void free_group_node(clist_node *node);

int filter_config(attribute *att);

#endif
