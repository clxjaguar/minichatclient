#ifndef PARSER_P_H
#define PARSER_P_H

#define TYPE_MESSAGE      0
#define TYPE_OPENING_TAG  1
#define TYPE_CLOSING_TAG  2

#include "attribute.h"
#include "clist.h"

typedef struct rule_struct {
	char *tag;
	char *value;
	char *start;
	char *stop;
} rule;

typedef struct parser_one_config_struct {
	char *context;
	clist *rules;
} parser_one_config;

typedef struct parser_config_private_struct {
	clist *groups;
} parser_config_private;

typedef struct message_part_struct message_part;
struct message_part_struct {
	int type; // (0 or 1 or 2: 0 is text, 1 is <>, 2 is </>)
	char *data;
	message_part *link; // (NULL for text)
	clist *attributes; // (NULL for text)
};

clist *get_parts(char *message);
clist_node *process_part(char *data, int text);

clist_node *add_group_node(clist *list, parser_one_config *group);
clist_node *add_rule_node(parser_one_config *group, rule *rule);

void free_message_part(message_part* message);
void free_message_part_node(clist_node* node);
void free_rule_node(clist_node *node);
void free_group_node(clist_node *node);

int filter_config(attribute *att);
#endif
