/*
  Name:         parser.c
  Author:       Niki
  Description:  Parse HTML inside each message
  Date:         20/06/11
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "cstring.h"
#include "clist.h"
#include "parser.h"
#include "ini.h" // because we want to use attribute's

#define bool            int
#define true              1
#define false             0

#define TYPE_MESSAGE      0
#define TYPE_OPENING_TAG  1
#define TYPE_CLOSING_TAG  2

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

typedef struct {
	clist *parser_config_lines;
} parser_config_private;

typedef struct {
	char *context;
	clist *rules;
} parser_config_line;

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

// Those are the functions defined in the (public) .h:

parser_config *parser_get_config(const char filename[]) {
	parser_config_line *config;
	clist *parser_config_lines;
	attribute *att;
	clist *atts;
	clist_node *ptr;
	parser_rule *rul;
	cstring *tmp;
	parser_config *parserconf;
	parser_config_private *data;
	FILE *file;
	
	file = fopen(filename, "r");
	
	if (!file) {
		return NULL;
	}
	
	rul = NULL;
	config = NULL;
	parser_config_lines = new_clist();
	atts = ini_get_select(file, filter_config, NULL);

	for (ptr = atts->first ; ptr != NULL ; ptr = ptr->next) {
		att = (attribute *)ptr->data;
		if(!strcmp(att->name, "context")) {
			config = (parser_config_line *)malloc(sizeof(parser_config_line));
			config->rules = new_clist();
			clist_add(parser_config_lines, new_parser_config_line_node(config));
			
			tmp = new_cstring();
			cstring_adds(tmp, att->value);
			config->context = cstring_convert(tmp);
		} else {
			// config can be NULL if bad INI
			if (config != NULL) {
				if(!strcmp(att->name, "tag")) {			
					rul = (parser_rule *)malloc(sizeof(parser_rule));					
					clist_add(config->rules, new_rule_node(rul));
					
					tmp = new_cstring();
					cstring_adds(tmp, att->value);
					rul->tag = cstring_convert(tmp);
				} else {
					// rule can be NULL if bad INI
					if (rul != NULL) {
						if(!strcmp(att->name, "value")) {
							tmp = new_cstring();
							cstring_adds(tmp, att->value);
							rul->value = cstring_convert(tmp);
						} else if(!strcmp(att->name, "start")) {
							tmp = new_cstring();
							cstring_adds(tmp, att->value);
							rul->start = cstring_convert(tmp);
						} else if(!strcmp(att->name, "stop")) {
							tmp = new_cstring();
							cstring_adds(tmp, att->value);
							rul->stop = cstring_convert(tmp);
						}
					}
				}
			}
		}
	}
	
	free_clist(atts);

	parserconf = (parser_config *)malloc(sizeof(parser_config));
	data = (parser_config_private *)malloc(sizeof(parser_config_private));
	data->parser_config_lines = parser_config_lines;
	parserconf->data = data;

	fclose(file);
	return parserconf;
}

void parser_parse_messages(clist *parts, parser_config *config,
	void (*operation)(parser_message *part, parser_rule *rul,
	void *argument), void *argument) {
	
	clist_node *ptr;
	parser_message *part;
	cstring *tmp;
	clist *context_stack;
	clist_node *context_node;	
	clist *parser_config_lines;
	parser_config_private *data;
	
	// Validity check.
	if (parts == NULL || operation == NULL) {
		return;
	}
	
	data = (parser_config_private *)config->data;
	parser_config_lines = data->parser_config_lines;
	
	context_stack = new_clist();
	for (ptr = parts->first ; ptr != NULL ; ptr = ptr->next) {
		part = (parser_message *)ptr->data;
		
		switch(part->type) {
		case TYPE_OPENING_TAG:
			tmp = new_cstring();
			if (part->data != NULL) {
				cstring_adds(tmp, part->data);
			}

			clist_add(context_stack, new_string_node(cstring_convert(tmp)));
			process_parser_message(operation, argument, parser_config_lines, context_stack, part);		break;
		case TYPE_CLOSING_TAG:
			if(context_stack->size > 0 && !strcmp(part->data, (char *)context_stack->last->data)) {
				process_parser_message(operation, argument, parser_config_lines, context_stack, part);
				
				context_node = clist_remove(context_stack, context_stack->last);
				free_clist_node(context_node);
			}
		break;
		case TYPE_MESSAGE:
		default:
			if (part->data != NULL) {
				operation(part, NULL, argument);
			}
		break;
		}
	}

	free_clist(context_stack);
}

char *parse_html_in_message(const char *message, parser_config *pconfig) {
	cstring *out;
	clist *parts;
	
	out = new_cstring();
	parts = parser_get_parts(message);
	parser_parse_messages(parts, pconfig, process_tag, out);
	free_clist(parts);
	
	return cstring_convert(out);	
}

clist *parser_get_parts(const char *message) {
	clist *list;
	clist_node *ptr, *ptr2;
	
	list = create_parts(message);
	
	associate_links(list);
	force_close_tags(list);
	
	// Apply some special rules (eg: "@ ")
	for (ptr = list->first ; ptr != NULL ; ) {
		ptr2 = ptr;
		
		// The check..() functions can accept NULL as input
		ptr = check_at_rule(list, ptr);
		ptr = check_reduce_link_rule(ptr);
		
		// If nothing changed, we still need to increment the counter
		if (ptr == ptr2) {
			ptr = ptr->next;
		}
	}
	
	return list;
}

// Those are the functions are only defined in a private (_p) .h:

bool filter_config(attribute *att, void *argument) {
	// useless test, to remove a compile warning...
	if (argument == NULL && argument != NULL) {
		return false;
	}
	
	return (!strcmp(att->name, "context") || !strcmp(att->name, "tag") || !strcmp(att->name, "value") || !strcmp(att->name, "start") || !strcmp(att->name, "stop"));
}

parser_rule *process_parser_message_sub(parser_config_line *line, clist *context_stack, parser_message *part) {
	clist_node *cnode, *rnode, *tnode;
	clist *atts;
	parser_rule *rul;
	char *context;
	attribute *att;
	bool in_context, do_apply, global_tag, global_value, tag_equals, value_equals;
	
	do_apply = false;
	
	atts = part->attributes;
	if (part->type == TYPE_CLOSING_TAG) {
		if (part->link != NULL) {
			atts = part->link->attributes;
		}
	}
	
	in_context = false;
	for (cnode = context_stack->first ; !in_context && cnode != NULL ; cnode = cnode->next) {
		context = (char *)cnode->data;
		if (line->context == NULL || line->context[0] == '\0' || !strcmp(line->context, context)) {
			in_context = true;
		}
	}
	
	if (in_context) {
		for (rnode = line->rules->first ; rnode != NULL ; rnode = rnode->next) {
			rul = (parser_rule *)rnode->data;
			global_tag = (rul->tag == NULL || rul->tag[0] == '\0');
			
			do_apply = global_tag;
			for (tnode = atts->first ; !do_apply && tnode != NULL ; tnode = tnode->next) {
				att = (attribute *)tnode->data;
				
				tag_equals = (!strcmp(rul->tag, att->name));
				global_value = (rul->value == NULL || rul->value[0] == '\0');
				value_equals = (!strcmp(rul->value, att->value));
				
				if (tag_equals && (global_value || value_equals)) {
					do_apply = true;
				}
			}
			
			if (do_apply) {
				return rul;
			}
		}
	}
	
	return NULL;
}

void process_parser_message(void (*operation)(parser_message *part, parser_rule *rul, void *argument), void *argument, clist *parser_config_lines, clist *context_stack, parser_message *part) {
	parser_config_line *line;
	clist_node *ptr;
	bool applied;
	parser_rule *rul;

	applied = false;
	
	switch (part->type) {
		case TYPE_CLOSING_TAG:
		case TYPE_OPENING_TAG:
			for (ptr = parser_config_lines->first ; !applied && ptr != NULL ; ptr = ptr->next) {
				line = (parser_config_line *)ptr->data;
				rul = process_parser_message_sub(line, context_stack, part);
				if (rul != NULL) {
					operation(part, rul, argument);
					applied = true;
				}
			}
			break;
		default:
			// should not happen
		break;
	}
	
	if (!applied) {
		operation(part, NULL, argument);
	}
}

clist_node *create_part(char *data, bool text) {
	parser_message *part;
	clist_node *node;
	clist *tab;
	cstring *tmp, *string;
	attribute *att;
	size_t i, first_equ;
	
	part = malloc(sizeof(parser_message));

	if (text) {
		part->type = TYPE_MESSAGE;
		part->data = data;
		part->link = NULL;
		part->attributes = NULL;
	} else {
		if (data[0] == '/') {
			part->type = TYPE_CLOSING_TAG;
			i = 1;
		} else {		
			part->type = TYPE_OPENING_TAG;
			i = 0;
		}
		
		part->attributes = new_clist();
		
		tmp = new_cstring();
		cstring_addfs(tmp, data, i);
		free(data);
		
		tab = cstring_splitc(tmp, ' ', '\"');
		node = tab->first;
		if (node == NULL) {
			part->data = cstring_convert(new_cstring());
		} else {
			part->data = cstring_convert(node->data);
			node->data = NULL; // Because we don't want to free() it
			for (node = node->next ; node != NULL ; node = node->next) {
				string = (cstring*)node->data;
				node->data = NULL;
			
				for (first_equ = 0 ; string->string[first_equ] != '\0' && string->string[first_equ] != '=' ; first_equ++);
				if (string->string[first_equ] != '=') {
					first_equ = 0;
				}

				att = new_attribute();
				att->value = cstring_convert(cstring_substring(string, first_equ + 1, 0));
				cstring_cut_at(string, first_equ);			
				att->name = cstring_convert(string);

				attribute_add_to_clist(part->attributes, att);
			}
		}
		free_clist(tab);
		free_cstring(tmp);
		
		// The links are not linked here, but in a second pass.
		part->link = NULL;
	}
	
	node = new_clist_node();
	node->free_node = free_parser_message_node;
	node->data = part;
	
	return node;
}

clist *create_parts(const char *message) {
	clist *list;
	cstring *prev_data;
	bool bracket;
	size_t i;
	char car;
	clist_node *node;
	parser_message *part;
	
	list = new_clist();
	prev_data = new_cstring();
	
	bracket = false;
	i = 0;
	
	// cut the string at every '<' and '>'
	for (car = message[i] ; car != '\0' ; car = message[++i]) {
		if (!bracket && car == '<') {
			bracket = true;
			if (prev_data->length > 0) {
				clist_add(list, create_part(cstring_convert(prev_data), true));
				prev_data = new_cstring();
			}
		} else if (bracket && car == '>') {
			bracket = false;
			if (prev_data->length > 0) {
				if ((i > 0 && message[i - 1] != '/') || prev_data->length > 1) {
					node = create_part(cstring_convert(prev_data), false);
					part = (parser_message *)node->data;
					clist_add(list, node);
			
					if (i > 0 && message[i - 1] == '/') {
						node = clone_parser_message_node(node);
						part = (parser_message *)node->data;
						part->type = TYPE_CLOSING_TAG;
						clist_add(list, node);
					}

					prev_data = new_cstring();
				}
			}
		} else {
			cstring_addc(prev_data, car);
		}
	}
	
	if (prev_data->length > 0) {
		clist_add(list, create_part(cstring_convert(prev_data), true));
	} else {
		free_cstring(prev_data);
	}
	
	return list;
}

void associate_links(clist *list) {
	clist *parts_stack;
	clist_node *ptr, *node;
	parser_message *part, *linked_part;
	
	parts_stack = new_clist();
	for (ptr = list->first ; ptr != NULL ; ptr = ptr->next) {
		part = (parser_message *)ptr->data;
		
		switch (part->type) {
		case TYPE_OPENING_TAG:
			node = new_clist_node();
			node->free_node = NULL; // we don't want to free() the data!
			node->data = part;
			clist_add(parts_stack, node);
			break;
		case TYPE_CLOSING_TAG:
			if (parts_stack->last != NULL) {
				linked_part = (parser_message *)parts_stack->last->data;
				free_clist_node(clist_remove(parts_stack, parts_stack->last));
				if (!strcmp(part->data, linked_part->data)) {
					part->link = linked_part;
					linked_part->link = part;
				}
			}
			break;
		case TYPE_MESSAGE:
		default:
			break;
		}
	}
	free_clist(parts_stack);
}

void force_close_tags(clist *list) {
	cstring *cdata;
	clist_node *ptr, *node;
	parser_message *part;
	
	for (ptr = list->first ; ptr != NULL ; ptr = ptr->next) {
		part = (parser_message *)ptr->data;
		
		switch (part->type) {
		case TYPE_OPENING_TAG:
			if (part->link == NULL) {
				cdata = new_cstring();
				cstring_addc(cdata, '/');
				cstring_adds(cdata, part->data);
				
				node = create_part(cstring_convert(cdata), false);
				
				// Link both
				((parser_message *)node->data)->link = part;
				part->link = node->data;
				
				node->next = ptr->next;
				node->prev = ptr;
				ptr->next = node;
			}
		break;
		default:
		break;
		}
	}
}

int count_span_data_span(clist_node *ptr) {
	parser_message *part;
	int num_of_spans, num_of_sspans;
	
	num_of_spans = 0;
	part = NULL;
	
	if (ptr != NULL) {
		part = (parser_message *)ptr->data;
	}
		
	while (ptr != NULL && !strcmp(part->data, "span") && part->type == TYPE_OPENING_TAG) {
		num_of_spans++;
		ptr = ptr->next;
		if (ptr != NULL) {
			part = (parser_message *)ptr->data;
		}
	}
	
	if (part == NULL || part->type != TYPE_MESSAGE) {
		return 0;
	}
	
	if (ptr != NULL) {
		ptr = ptr->next;
	}
	
	if (ptr != NULL) {
		num_of_sspans = 0;
		part = (parser_message *)ptr->data;
		while (ptr != NULL && !strcmp(part->data, "span") && part->type == TYPE_CLOSING_TAG) {
			num_of_sspans++;
			ptr = ptr->next;
			if (ptr != NULL) {
				part = (parser_message *)ptr->data;
			}
		}
		
		if (num_of_spans <= num_of_sspans) {
			return num_of_spans;
		}
	}
	
	return 0;
}

clist_node *check_at_rule(clist *list, clist_node *ptr) {
	// @ <-UTF8-> (Ox40/64 OxC2/194)
	//const char s_at1[4] = {0x40, 0xC2, 0xA0, '\0'};
		
	cstring *cdata;
	clist_node *node;
	parser_message *part;
	char blast, bblast; // before last, before before last chars
	size_t size;
	int num_of_spans, i;
	
	// Validity check.
	if (list == NULL || ptr == NULL) {
		return ptr;
	}
	
	blast = '\0';
	bblast = '\0';
	part = (parser_message *)ptr->data;
	size = strlen(part->data);
	
	if (size >= 3) bblast = part->data[size - 1 - 2];
	if (size >= 2) blast = part->data[size - 1 - 1];
	
	if (blast == '@' || bblast == '@') {
		num_of_spans = count_span_data_span(ptr->next);
		if (num_of_spans > 0) {
			// remove the @
			if ((blast == '@' && size == 2) || (bblast == '@' && size == 3)) {
				// The "@xx" is on its own parser_message
				node = ptr->next;
				clist_remove(list, ptr);
				free_clist_node(ptr);
				ptr = node;
			} else {
				// The "@xx" is at the end of a parser_message
				cdata = new_cstring();
				cstring_addns(cdata, part->data, size);
				free(part->data);
				part->data = cstring_convert(cdata);
				ptr = ptr->next;
			}
		}
		
		for (; num_of_spans > 0 ; num_of_spans = count_span_data_span(ptr)) {
			// change the <span> into <nick>
			part = (parser_message *)ptr->data;
			cdata = new_cstring();
			cstring_adds(cdata, "nick");
			free(part->data);
			part->data = cstring_convert(cdata);
			ptr = ptr->next;
			
			// delete the <span>'s
			for (i = 0 ; i < num_of_spans - 1; i++) {
				node = ptr->next;
				clist_remove(list, ptr);
				free_clist_node(ptr);
				ptr = node;
			}
			
			// skip the nickname
			if (ptr != NULL) {
				ptr = ptr->next;
			}
			
			// delete the </span>'s
			for (i = 0 ; i < num_of_spans - 1; i++) {
				node = ptr->next;
				clist_remove(list, ptr);
				free_clist_node(ptr);
				ptr = node;
			}
			
			// change the </span> into </nick>
			if (ptr != NULL) {
				part = (parser_message *)ptr->data;
				cdata = new_cstring();
				cstring_adds(cdata, "nick");
				free(part->data);
				part->data = cstring_convert(cdata);
				ptr = ptr->next;
			}
			
			// remove the ", " if it exists
			if (ptr != NULL) {
				part = (parser_message *)ptr->data;
				cdata = new_cstring();
				cstring_adds(cdata, part->data);
				if (cstring_starts_withs(cdata, ", ", 0)) {
					if (cdata->length <= 2) {
						// The ", " is on its own line
						node = ptr->next;
						clist_remove(list, ptr);
						free_clist_node(ptr);
						ptr = node;
					} else {
						// The ", " is at the start of another line
						free(part->data);
						part->data = cstring_convert(cstring_substring(cdata, 2, 0));
					}
				}
				free_cstring(cdata);
			}
		}
	}
	
	return ptr;
}

clist_node *check_reduce_link_rule(clist_node *ptr) {
	cstring *prev_data, *cdata, *cdata2, *starting, *ending;
	long long i;
	bool do_apply;
	char *string;
	clist_node *node;
	parser_message *part, *linked_part;
	attribute *att;
	
	// Validity check.
	if (ptr == NULL) {
		return ptr;
	}
	
	part = (parser_message *)ptr->data;
	cdata = new_cstring();
	cstring_adds(cdata, part->data);
	if (part->type == TYPE_OPENING_TAG && (strcmp(part->data, "a") == 0 || strcmp(part->data, "A") == 0)) {
		// Change <a href="abcdefghi">abc ... ghi</a> into <a href="abc ... ghi">abcdefghi</a>
		if (ptr->next != NULL) {
			do_apply = false;
			linked_part = (parser_message *)ptr->next->data;
			starting = new_cstring();
			cstring_adds(starting, (char *)linked_part->data);
			i = cstring_finds(starting, " ... ", 0);

			cdata2 = new_cstring();
			att = NULL;
			for (node = part->attributes->first ; node != NULL ; node = node->next) {
				att = (attribute *)node->data;
				if (strcmp(att->name, "href") == 0) {
					cstring_adds(cdata2, att->value);
					break;
				}
			}

			// option 1: both links are equals
			if (att != NULL && strcmp((char *)linked_part->data, att->value) == 0) {
				do_apply = true;
			}

			// option 2: displayed link is "truncated" href link (abc ... ghi)
			if (att != NULL && !do_apply && i >= 0) {
				// cut starting into 'starting' and 'ending' (separated by " ... ")
				// and set cdata2 to the value associated with the href attribute
				ending = new_cstring();
				cstring_addf(ending, starting, (size_t)(i + 5));
				cstring_cut_at(starting, (size_t)i);
			
				if (cstring_starts_with(cdata2, starting, 0) && cstring_ends_with(cdata2, ending, 0)) {
					do_apply = true;
				}

				free_cstring(ending);
			}

			if (do_apply) {
				// the href correspond to the next node value (eg: 'abcdef' for the href and 'ab ... ef' for the
				// next node value)
				// What we do: we invert href and the next node value, and we change href into nohref
				string = att->value;
				att->value = (char *)linked_part->data;
				linked_part->data = string;
			
				free(att->name);
				prev_data = new_cstring();
				cstring_adds(prev_data, "nohref");
				att->name = cstring_convert(prev_data);
			}
			free_cstring(starting);
			free_cstring(cdata2);
		}
	}
	
	if (cdata != NULL) {
		free_cstring(cdata);
	}
	
	return ptr;
}

void process_tag(parser_message *part, parser_rule *rul, void *argument) {
	clist *atts;
	attribute *att;
	cstring *text_to_apply, *from, *to;
	clist_node *att_node;
	
	cstring *out = (cstring *)argument;
	
	if (rul == NULL) {
		// If no rule for <tag> (or </tag>): do_nothing()
		if (part->type == TYPE_MESSAGE) {
			cstring_adds(out, part->data);
		}
	} else {
		atts = part->attributes;
		if (part->type == TYPE_CLOSING_TAG) {
			if (part->link != NULL) {
				atts = part->link->attributes;
			}
		}
		
		text_to_apply = new_cstring();
	
		switch (part->type) {
		case TYPE_OPENING_TAG:
			cstring_adds(text_to_apply, rul->start);
			break;
		case TYPE_CLOSING_TAG:
			cstring_adds(text_to_apply, rul->stop);
			break;
		default:
		break;
		}

		if (atts != NULL && atts->size > 0) {
			from = new_cstring();
			to = new_cstring();
			for (att_node = atts->first ; att_node != NULL ; att_node = att_node->next) {
				att = (attribute *)att_node->data;
				cstring_clear(from);
				cstring_adds(from, "\\{");
				cstring_adds(from, att->name);
				cstring_addc(from, '}');

				cstring_clear(to);
				cstring_adds(to, att->value);
				
				cstring_replace(text_to_apply, from, to);
			}
			free_cstring(to);
			free_cstring(from);
		}
			
		cstring_add(out, text_to_apply);
		free_cstring(text_to_apply);
	}
}

// Functions relative to clist_node's (allocate/free/clone):

clist_node *clone_parser_message_node(clist_node *node) {
	parser_message *message, *new_message;
	clist_node *new_node;
	
	message = (parser_message *) node->data;
	new_node = new_clist_node();
	new_message = (parser_message *)malloc(sizeof(parser_message));
	
	new_message->type = message->type;
	new_message->attributes = new_clist(); //TODO: not correct for a clone operation!
	new_message->link = NULL;
	if (message->data == NULL) {	
		new_message->data = NULL;
	} else {
		new_message->data = malloc((strlen(message->data) + 1) * sizeof(char));
		strcpy(new_message->data, message->data);
	}

	new_node->data = new_message;
	new_node->free_node = node->free_node;
	return new_node;
}

void free_rule_node(clist_node *node) {
	parser_rule *rul;
	
	if (node->data != NULL) {
		rul = (parser_rule *)node->data;
		if (rul->tag != NULL) {
			free(rul->tag);
		}
		if (rul->value != NULL) {
			free(rul->value);
		}
		if (rul->start != NULL) {
			free(rul->start);
		}
		if (rul->stop != NULL) {
			free(rul->stop);
		}
		free(rul);
	}
	
	free(node);
}

void free_group_node(clist_node *node) {
	parser_config_line *group;
	
	if (node->data != NULL) {
		group = (parser_config_line *)node->data;
		if (group->context != NULL) {
			free(group->context);
		}
		if (group->rules != NULL) {
			free_clist(group->rules);
		}
		free(group);
	}
	free(node);
}

void free_parser_config(parser_config *pconfig) {
	parser_config_private *data;
	
	data = (parser_config_private *)pconfig->data;
	
	if (data->parser_config_lines != NULL) {
		free_clist(data->parser_config_lines);
	}

	free(pconfig->data);
	free(pconfig);
}

clist_node *new_rule_node(parser_rule *rrule) {
	clist_node *node;
	
	node = new_clist_node();
	node->data = rrule;
	node->free_node = free_rule_node;
	
	return node;
}

clist_node *new_string_node(char *string) {
	clist_node *context_node;
	
	context_node = new_clist_node();
	context_node->free_node = free_clist_node_data;
	context_node->data = string;

	return context_node;
}

clist_node *new_parser_config_line_node(parser_config_line *group) {
	clist_node *node;
	
	node = new_clist_node();
	node->data = group;
	node->free_node = free_group_node;
	
	return node;
}

void free_parser_message(parser_message* message) {
	if (message->data != NULL) {
		free(message->data);
	}
	
	if (message->attributes != NULL) {
		free_clist(message->attributes);
	}
	
	if (message->link != NULL) {
		message->link->link = NULL;
	}
	
	free(message);
}

void free_parser_message_node(clist_node* node) {
	if(node->data != NULL) {
		free_parser_message((parser_message *)node->data);
	}
	free(node);
}

