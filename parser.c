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
#include "parser_p.h"
#include "ini.h"

int filter_config(attribute *att) {
	return (!strcmp(att->name, "context") || !strcmp(att->name, "tag") || !strcmp(att->name, "value") || !strcmp(att->name, "start") || !strcmp(att->name, "stop"));
}

parser_config *get_parser_config(char filename[]) {
	config_line *config;
	clist *config_lines;
	attribute *att;
	clist *atts;
	clist_node *ptr;
	rule *rul;
	cstring *tmp;
	parser_config *parserconf;
	FILE *file;
	
	file = fopen(filename, "r");
	
	rul = NULL;
	config = NULL;
	config_lines = new_clist();
	atts = ini_get_select(file, filter_config);

	for (ptr = atts->first ; ptr != NULL ; ptr = ptr->next) {
		att = (attribute *)ptr->data;
		if(!strcmp(att->name, "context")) {
			config = (config_line *)malloc(sizeof(config_line));
			config->rules = new_clist();
			clist_add(config_lines, new_group_node(config));
			
			tmp = new_cstring();
			cstring_adds(tmp, att->value);
			config->context = cstring_convert(tmp);
		} else {
			// config can be NULL if bad INI
			if (config != NULL) {
				if(!strcmp(att->name, "tag")) {			
					rul = (rule *)malloc(sizeof(rule));					
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

	parserconf = (parser_config *)malloc(sizeof(parser_config));
	parserconf->data = (parser_config_private *)malloc(sizeof(parser_config_private));
	parserconf->data->config_lines = config_lines;
	
	fclose(file);
	return parserconf;
}

char *parse_html_in_message(char *message, parser_config *pconfig) {
	clist *parts;
	clist_node *ptr;
	message_part *part;
	cstring *out;
	cstring *tmp;
	clist *context_stack;
	clist_node *context_node;	
	clist *config_lines;
	
	config_lines = pconfig->data->config_lines;
	
	out = new_cstring();
	parts = get_parts(config_lines, message);
	
	context_stack = new_clist();
	for (ptr = parts->first ; ptr != NULL ; ptr = ptr->next) {
		part = (message_part *)ptr->data;
		
		switch(part->type) {
		case TYPE_MESSAGE:
			if (part->data != NULL) {
				cstring_adds(out, part->data);
			}
		break;
		case TYPE_OPENING_TAG:
			tmp = new_cstring();
			if (part->data != NULL) {
				cstring_adds(tmp, part->data);
			}

			clist_add(context_stack, new_string_node(cstring_convert(tmp)));
			
			cstring_adds(out, process_message_part(config_lines, context_stack, part));
		break;
		case TYPE_CLOSING_TAG:
			if(context_stack->size > 0 && !strcmp(part->data, (char *)context_stack->last->data)) {
				cstring_adds(out, process_message_part(config_lines, context_stack, part));
				
				context_node = clist_remove(context_stack, context_stack->last);
				free_clist_node(context_node);
			}
		break;
		}
	}
	
	free_clist(context_stack);
	free_clist(parts);
	
	return cstring_convert(out);
}

clist *get_parts(clist *config_lines, char *message) {
	clist *list, *parts_stack;
	cstring *prev_data, *cdata, *cdata2, *starting, *ending;
	int bracket;
	int i, do_apply;
	char car;
	char *string;
	clist_node *ptr, *node;
	message_part *part, *linked_part;
	attribute *att;
	
	list = new_clist();
	prev_data = new_cstring();
	
	bracket = 0;
	i = 0;
	
	for (car = message[i] ; car != '\0' ; car = message[++i]) {
		if (!bracket && car == '<') {	
			bracket = 1;
			if (prev_data->length > 0) {
				clist_add(list, process_part(config_lines, cstring_convert(prev_data), 1));
				prev_data = new_cstring();
			}
		} 
		else if (bracket && car == '>') {
			bracket = 0;
			if (prev_data->length > 0) {
				if ((i > 0 && message[i - 1] != '/') || prev_data->length > 1) {
					node = process_part(config_lines, cstring_convert(prev_data), 0);
					part = (message_part *)node->data;
					clist_add(list, node);
			
					if (i > 0 && message[i - 1] == '/') {
						node = clone_message_part_node(node);
						part = (message_part *)node->data;
						part->type = TYPE_CLOSING_TAG;
						clist_add(list, node);
					}

					prev_data = new_cstring();
				}
			}
		} 
		else {
			cstring_addc(prev_data, car);
		}
	}
	
	if (prev_data->length > 0) {
		clist_add(list, process_part(config_lines, cstring_convert(prev_data), 1));
	}
	
	// associate the links between them
	parts_stack = new_clist();
	for (ptr = list->first ; ptr != NULL ; ptr = ptr->next) {
		part = (message_part *)ptr->data;
		
		switch (part->type) {
		case TYPE_OPENING_TAG:
			node = new_clist_node();
			node->free_node = NULL; // we don't want to free() the data!
			node->data = part;
			clist_add(parts_stack, node);
			break;
		case TYPE_CLOSING_TAG:
			if (parts_stack->last != NULL) {
				linked_part = (message_part *)parts_stack->last->data;
				free_clist_node(clist_remove(parts_stack, parts_stack->last));
				if (!strcmp(part->data, linked_part->data)) {
					part->link = linked_part;
					linked_part->link = part;
				}
			}
			break;
		case TYPE_MESSAGE:
			break;
		}
	}
	free_clist(parts_stack);
	
	// associate the links between them (second pass, to force-close parts if they are not linked)
	parts_stack = new_clist();
	for (ptr = list->first ; ptr != NULL ; ptr = ptr->next) {
		part = (message_part *)ptr->data;
		
		switch (part->type) {
		case TYPE_OPENING_TAG:
			if (part->link == NULL) {
				cdata = new_cstring();
				cstring_addc(cdata, '/');
				cstring_adds(cdata, part->data);
				
				node = process_part(config_lines, cstring_convert(cdata), 0);
				node->next = ptr->next;
				node->prev = ptr;
				ptr->next = node;
				
				ptr = node;
			}
		break;
		}
	}
	
	// Apply some rules (eg: "@ ")
	for (ptr = list->first ; ptr != NULL ; ptr = ptr->next) {
		part = (message_part *)ptr->data;
		cdata = new_cstring();
		cstring_adds(cdata, part->data);
		if (part->type == TYPE_MESSAGE && cstring_ends_withs(cdata, "@ ", 0)) {
			// '@ <span ...>NICK</span>, '
			// remove the "@ " and point 'ptr' to the fist span
			if (!strcmp(cdata->string, "@ ")) {
				node = ptr->next;
				clist_remove(list, ptr);
				free_clist_node(ptr);
				ptr = node;
			} else {
				cstring_cut_at(cdata, cdata->length - 2);
				free(part->data);
				part->data = cstring_convert(cdata);
				cdata = NULL;
				ptr = ptr->next;
			}
			if (ptr != NULL) {
				// process the first <span>
				part = (message_part *)ptr->data;
				cdata2 = new_cstring();
				cstring_adds(cdata2, "nick");
				if (part->data != NULL) {
					free(part->data);
				}
				part->data = cstring_convert(cdata2);
				if (part->attributes != NULL) {
					free_clist(part->attributes);
				}
				part->attributes = new_clist();		
				ptr = ptr->next;
			
				if (ptr != NULL && ptr->next != NULL) {
					// process the NICK
					ptr = ptr->next;
					
					// process the second </span>
					part = (message_part *)ptr->data;
					cdata2 = new_cstring();
					cstring_adds(cdata2, "nick");
					if (part->data != NULL) {
						free(part->data);
					}
					part->data = cstring_convert(cdata2);
					if (part->attributes != NULL) {
						free_clist(part->attributes);
					}
					part->attributes = new_clist();
					ptr = ptr->next;
				
					if (ptr != NULL) {
						// check the text after the NICK, and remove ", " from it, then points to it
						part = (message_part *)ptr->data;
						cdata2 = new_cstring();
						cstring_adds(cdata2, part->data);
						if (cstring_starts_withs(cdata2, ", ", 0)) {
							if (!strcmp(cdata2->string, ", ")) {
								node = ptr;
								ptr = node->next;
								clist_remove(list, node);
								free_clist_node(node);
							} else {
								prev_data = cstring_substring(cdata2, 2, -1);
								free(part->data);
								part->data = cstring_convert(prev_data);
							}
						}
						free_cstring(cdata2);
					}
				}
			}
		}
		if (part->type == TYPE_OPENING_TAG && (strcmp(part->data, "a") == 0 || strcmp(part->data, "A") == 0)) {
			// Change <a href="abcdefghi">abc ... ghi</a> into <a href="abc ... ghi">abcdefghi</a>
			if (ptr->next != NULL) {
				do_apply = 0;
				linked_part = (message_part *)ptr->next->data;
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
					do_apply = 1;
				}

				// option 2: displayed link is "truncated" href link (abc ... ghi)
				if (att != NULL && !do_apply && i >= 0) {
					// cut starting into 'starting' and 'ending' (separated by " ... ")
					// and set cdata2 to the value associated with the href attribute
					ending = new_cstring();
					cstring_addf(ending, starting, i + 5);
					cstring_cut_at(starting, i);
					
					if (cstring_starts_with(cdata2, starting, 0) && cstring_ends_with(cdata2, ending, 0)) {
						do_apply = 1;
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

		// If the input is not correct, you can 'skip' some node
		// and thus end on a ptr == NULL -- which will crash
		// at the next increment of the loop (ptr = ptr->next)
		if (ptr == NULL) {
			break;
		}
	}
	
	return list;
}

void free_message_part_node(clist_node* node) {
	if(node->data != NULL) {
		free_message_part((message_part *)node->data);
	}
	free(node);
}

void free_message_part(message_part* message) {
	clist_node *node;
	clist_node *next;
	
	if (message->data != NULL) {
		free(message->data);
	}
	
	if (message->attributes != NULL) {
		for (node = message->attributes->first ; node != NULL ; ) {
			next = node->next;
			free_clist_node(node);
			node = next;
		}
	}
	
	free(message);
}

char *process_message_part(clist *config_lines, clist *context_stack, message_part *part) {
	cstring *out;
	config_line *line;
	clist_node *ptr;
	int applied;

	out = new_cstring();
	switch (part->type) {
	case TYPE_CLOSING_TAG: //TODO: check
	case TYPE_OPENING_TAG:
		applied = 0;
		for (ptr = config_lines->first ; !applied && ptr != NULL ; ptr = ptr->next) {
			line = (config_line *)ptr->data;
			applied = process_message_part_sub(out, line, context_stack, part);
		}
		break;
	/*case TYPE_CLOSING_TAG:
		applied = 0;
		for (ptr = config_lines->last ; !applied && ptr != NULL ; ptr = ptr->prev) {			
			line = (config_line *)ptr->data;
			applied = process_message_part_sub(out, line, context_stack, part);
		}
		break;*/
	}
	
	return cstring_convert(out);
}

int process_message_part_sub(cstring *out, config_line *line, clist *context_stack, message_part *part) {
	clist_node *cnode;
	clist_node *rnode;
	clist_node *tnode;
	clist *atts;
	rule *rul;
	char *context;
	attribute *att;
	int in_context, do_apply, global_tag, global_value, tag_equals, value_equals;
	cstring *text_to_apply, *from, *to;
	clist_node *att_node;

	do_apply = 0;
	
	atts = part->attributes;
	if (part->type == TYPE_CLOSING_TAG) {
		if (part->link != NULL) {
			atts = part->link->attributes;
		}
	}
	
	in_context = 0;
	for (cnode = context_stack->first ; !in_context && cnode != NULL ; cnode = cnode->next) {
		context = (char *)cnode->data;
		if (line->context == NULL || line->context[0] == '\0' || !strcmp(line->context, context)) {
			in_context = 1;
		}
	}
	
	if (in_context) {
		for (rnode = line->rules->first ; rnode != NULL ; rnode = rnode->next) {
			rul = (rule *)rnode->data;
			global_tag = (rul->tag == NULL || rul->tag[0] == '\0');
			
			do_apply = global_tag;
			for (tnode = atts->first ; !do_apply && tnode != NULL ; tnode = tnode->next) {
				att = (attribute *)tnode->data;
				
				tag_equals = (!strcmp(rul->tag, att->name));
				global_value = (rul->value == NULL || rul->value[0] == '\0');
				value_equals = (!strcmp(rul->value, att->value));
				
				if (tag_equals && (global_value || value_equals)) {
					do_apply = 1;
				}
			}
			
			if (do_apply) {
				text_to_apply = new_cstring();
				
				switch (part->type) {
				case TYPE_OPENING_TAG:
					cstring_adds(text_to_apply, rul->start);
					break;
				case TYPE_CLOSING_TAG:
					cstring_adds(text_to_apply, rul->stop);
					break;
				}

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
				
				cstring_add(out, text_to_apply);
				free_cstring(text_to_apply);
			}
		}
	}
	
	return do_apply;
}

clist_node *process_part(clist *config_lines, char *data, int text) {
	message_part *part;
	clist_node *node;
	clist *tab;
	cstring *tmp;
	cstring *string;
	int i;
	attribute *att;
	int first_equ;
	
	part = malloc(sizeof(message_part));

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
		
		i = 0;
		tab = cstring_splitc(tmp, ' ', '\"');
		for (node = tab->first ; node != NULL ; node = node->next) {
			if (!i) {
				part->data = cstring_convert(node->data);
				node->data = NULL;
				i = 1;
			} else {
				string = (cstring*)node->data;
				
				for (first_equ = 0 ; string->string[first_equ] != '\0' && string->string[first_equ] != '=' ; first_equ++);
				if (string->string[first_equ] != '=') {
					first_equ = 0;
				}

				att = new_attribute();
				att->value = cstring_convert(cstring_substring(string, first_equ + 1, -1));
				cstring_cut_at(string, first_equ);			
				att->name = cstring_convert(string);

				attribute_add_to_clist(part->attributes, att);
			}
		}
		
		//TODO
		part->link = NULL;
	}
	
	node = new_clist_node();
	node->free_node = free_message_part_node;
	node->data = part;
	
	return node;
}

clist_node *clone_message_part_node(clist_node *node) {
	message_part *message, *new_message;
	clist_node *new_node;
	
	message = (message_part *) node->data;
	new_node = new_clist_node();
	new_message = (message_part *)malloc(sizeof(message_part));
	
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
	return new_node;
}

clist_node *new_rule_node(rule *rule) {
	clist_node *node;
	
	node = new_clist_node();
	node->data = rule;
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

clist_node *new_group_node(config_line *group) {
	clist_node *node;
	
	node = new_clist_node();
	node->data = group;
	node->free_node = free_group_node;
	
	return node;
}

void free_rule_node(clist_node *node) {
	rule *rul;
	
	if (node->data != NULL) {
		rul = (rule *)node->data;
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
	}
	
	free(node);
}

void free_group_node(clist_node *node) {
	config_line *group;
	
	if (node->data != NULL) {
		group = (config_line *)node->data;
		if (group->context != NULL) {
			free(group->context);
		}
		if (group->rules != NULL) {
			free_clist(group->rules);
		}
		
	}
	free(node);
}

void free_parser_config(parser_config *pconfig) {
	if (pconfig->data->config_lines != NULL) {
		free_clist(pconfig->data->config_lines);
	}
	free(pconfig->data);
	free(pconfig);
}
