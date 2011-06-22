#ifndef PARSER_P_H
#define PARSER_P_H

#define TYPE_MESSAGE      0
#define TYPE_OPENING_TAG  1
#define TYPE_CLOSING_TAG  2

#include "attribute.h"
#include "clist.h"

typedef struct rule_struct rule;
struct rule_struct {
	char *tag;
	char *value;
	char *start;
	char *stop;
};

typedef struct rule_group_struct rule_group;
struct rule_group_struct {
	char *context;
	clist *rules;
};

typedef struct message_part_struct message_part;
struct message_part_struct {
	int type; // (0 or 1 or 2: 0 is text, 1 is <>, 2 is </>)
	char *data;
	message_part *link; // (NULL for text)
	clist *attributes; // (NULL for text)
};

char *get_date();
clist *get_parts(char *message);
char *get_text(message_part *message);
clist *configure(FILE *file);
clist_node *process_part(char *data, int text);

clist_node *add_group_node(clist *list, rule_group *group);
clist_node *add_rule_node(rule_group *group, rule *rule);

void free_message_part(message_part* message);
void free_message_parts(message_part** messages);
void free_message_part_node(clist_node* node);
void free_rule_node(clist_node *node);
void free_group_node(clist_node *node);

int filter_config(attribute *att);
#endif
