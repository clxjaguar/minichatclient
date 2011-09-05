#ifndef PARSER_P_H
#define PARSER_P_H

#include "ini.h" // because we want to use attribute's

#define bool int
#define true 1
#define false 0

/**
 * This is the content of parser_config.
 */
typedef struct {
	clist *parser_config_lines;
} parser_config_private;

/**
 * A confirugration line from a .conf configuration file
 * (like parser_rules.conf).
 */
typedef struct {
	char *context;
	clist *rules;
} parser_config_line;

//

/**
 * A filter to use with ini_get_select() that will return all the values
 * whose 'name' property if any of:
 * - context
 * - tag
 * - value
 * - start
 * - stop
 *
 * @param att the attribute to filter
 * @param argument an unused argument (e.g. NULL)
 *
 * @return true if the attribute is to be kept
 */
bool filter_config(attribute *att, void *argument);

/**
 * Process a parser_message according to the rules in parser_config, and give
 * the hand to the (operation) function on it with the correct parameters.
 *
 * @param opeartion the operation to apply on the parser_message
 * @param argument the user argument to give to the operation function
 * @param groups the list of parser_config_lines
 * @param context_stack the context stack (a list of char *)
 * @param part the message to process
 */
void process_parser_message(void (*operation)(parser_message *part,
	parser_rule *rul, void *argument), void *argument, clist *parser_config_lines,
	clist *context_stack, parser_message *part);

/**
 * Process the message against a signle parser_config_line. It will be called by 
 * process_message on each parser_config_line for each parser_message.
 *
 * @param out the cstring on which to work
 * @param parser_config_line the configuration line to test against
 * @param context_stack the context stack (list of char *)
 * @param part the message to process
 * 
 * @return true if the rule was used
 */
parser_rule *process_parser_message_sub(parser_config_line *line, clist *context_stack,
	parser_message *part);

/**
 * Cut a string into parser_message's, checking if they are opening tags, closing
 * tags or text.
 *
 * @param message the message to cut
 *
 * @return a clist of parser_message 
 */
clist *create_parts(const char *message);

/**
 * Create a parser_message out of the given text.
 *
 * @param data the text
 * @param text true if the text is a text message and not a tag
 *
 * @return a clist_node with the newly allocated parser_message in it
 */
clist_node *create_part(char *data, bool text);

/**
 * Associate the links between them (the parameters in the <> tags will be 
 * tranferred into the pending </> tags, and vice-versa.
 *
 * @param list the clist of parser_message's to work with
 */
void associate_links(clist *list);

/**
 * Force-close tags which are not closed.
 *
 * @param list the clist of parser_message's to work with
 */
void force_close_tags(clist *list);

/**
 * Count the number of <span>/</span> you have that follow the pattern:
 * "<span><span>DATA</span></span>" (this one would return 2).
 *
 * @param ptr the item to start the check at
 *
 * @return the number of <span></span>'s
 */
int count_span_data_span(clist_node *ptr);

/**
 * Check the rule that marks someone as 'called' by "@ <span>Someone</span>, ".
 * If it is the case, we will remove the <span> and the </span>, the "@ ", and
 * replace it with "<nick>Someone</nick>".
 * Note that you can have <span><span>Someone</span></span>, too.
 *
 * @param list the list of parser_message from which we will process one item
 * @param ptr the item to process from the list
 *
 * @return the new current pointer (so, the next parser_message to check against
 * 	special rules)
 */
clist_node *check_at_rule(clist *list, clist_node *ptr);

/**
 * Check the rule that says that a reduced URL must be remove.
 * So, change "<a href="abcdefghi">abc ... ghi</a>" into 
 * "<a nohref="abc ... ghi">abcdefghi</a>" (note that it actually works on
 * a clist of parser_message's, not on the pure text).
 *
 * @param list the list of parser_message from which we will process one item
 * @param ptr the item to process from the list
 *
 * @return the new current pointer (so, the next parser_message to check against
 * 	special rules)
 */
clist_node *check_reduce_link_rule(clist_node *ptr);

/**
 * A simple (operation) function that will output the given parser_message
 * following the given rule into the cstring given as argument.
 *
 * @param part the message part
 * @param tul the rule to apply to it
 * @param argument a cstring to output into
 */
void process_tag(parser_message *part, parser_rule *rul, void *argument);


// Operations to allocate/free/clone clist_node's.

clist_node *clone_parser_message_node(clist_node *node);
void free_rule_node(clist_node *node);
void free_group_node(clist_node *node);
void free_parser_config(parser_config *pconfig);
clist_node *new_rule_node(parser_rule *rrule);
clist_node *new_string_node(char *string);
clist_node *new_parser_config_line_node(parser_config_line *group);
void free_parser_message(parser_message* message);
void free_parser_message_node(clist_node* node);

#endif
