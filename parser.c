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

/*

Todo :

(09:52:50) cLx: [TimeStamp]: http://www.serveur.com/2011/03 ... -a-lecole/
(09:53:04) cLx: il a pas renvoyé le bon lien ^^'
(09:55:07) niki: ok
(09:55:49) niki: mais il devait renovyer quoi?
(09:57:10) cLx: http://www.divertissonsnous.com/2011/03/14/se-battre-contre-un-gros-a-lecole/
(09:57:22) cLx: MAIS ! si le html était :
(09:57:49) niki: pcq bon, c un choix... je compte mettre (http://url] nom-url
(09:58:04) cLx: ouais voila
(09:58:20) niki: mais ca c pas encore fait ^^"'
(09:58:39) cLx: mais pour pas que ça soit trop lourd, il faut vérifier que le texte du lien soit pas la même chose que le lien, ou bien la même chose en version raccourcie ...
(09:59:41) cLx: mais sinon a part ça, super boulot, c'est cool :)
(09:59:54) niki: merci :)
(10:00:13) niki: (la version raccourcie, bonjour pour vérifier.. :x)
(10:01:25) cLx: je pense qu'il faut couper la chaine avec " ... "
(10:01:54) cLx: et après regarder si le coté gauche correspond au début, et le coté droit correspond à la fin ...
(10:02:11) niki: oui, on peut faire ça.. en supposant qu'ils raccourcissent toujours de la meme facon
(10:02:24) cLx: ouais je pense
(10:02:28) niki: ok alors

Bref:

(10:20:36) cLx: est ce que vous pouvezprocess_part(prev_data, 1) cliquer sur cette URL (http://perdu.com) ?
(10:20:48) cLx: sans que ça prenne les parenthèses dans l'adresse ?
(10:21:34) niki: oui
(10:21:41) cLx: parce que c'est assez classe comme façon de sortir "<!-- m --><a class="postlink" href="http://perdu.com">URL</a><!-- m --> ?"
(10:22:17) niki: oui
(10:22:19) niki: à noter :)

<strong>Nouveau Sujet</strong>: <a href="http://forum.francefurs.org/viewtopic.php?p=172728#p172728" class="postlink">Grosse surprise ce matin Oo</a>

clx@CatServ:~/softwares/minichatclient$ ./parser-test '<strong>Nouveau Sujet</strong>: <a href="http://forum.url.org/viewtopic.php?p=172728#p172728" class="postlink">Grosse surprise ce matin Oo</a>'
[TimeStamp]: *Nouveau Sujet*: Grosse surprise ce matin Oo
=> *Nouveau Sujet*: Grosse surprise ce matin Oo (http://forum.url.org/viewtopic.php?p=172728#p172728)

Et:
clx@CatServ:~/softwares/minichatclient$ ./parser-test '@ <span style="font-weight: bold">nick</span>'     
*** glibc detected *** ./parser-test: free(): invalid pointer: 0x08048960 ***
( http://pastebin.com/2WZnyq98, répétable !)

CHANGED : j'ai supprimé le timestamp, il ne doit pas être mis ici, et j'ai rajouté les mises à null après les free(). </cLx>

****

<aa> @ <span ....>XX</span>, FF
	from: aa
	to: XX
	mess: FF

<span style="font-weight: bold">
</span>

### Comments

context=
tag=strong
value=
use=*
end-tag=*

context=span
tag=style
value="font-weight: bold"
use=*
tag=style
value="font-???: italic"
use=_
end-tag=!REVERSE!

strong=*
/strong=*
a=[\{href}
/a=]

loop: link each tag with its corresponding off-tag

<span style="font-weight: bold">
	replace with <b>, replace corresp by </b>


*/


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
	cstring *prev_data, *cdata;
	int bracket;
	int i;
	char car;
	clist_node *ptr, *node;
	message_part *part, *linked_part;
	
	list = new_clist();
	prev_data = new_cstring();
	
	bracket = 0;
	i = 0;
	
	for (car = message[i] ; car != '\0' ; car = message[++i]) {
		if (!bracket && car == '<') {
			bracket = 1;
			clist_add(list, process_part(config_lines, cstring_convert(prev_data), 1));
			prev_data = new_cstring();
		} 
		else if (bracket && car == '>') {
			bracket = 0;
			clist_add(list, process_part(config_lines, cstring_convert(prev_data), 0));
			prev_data = new_cstring();
		} 
		else {
			cstring_addc(prev_data, car);
		}
	}
	
	if (!bracket) {
		clist_add(list, process_part(config_lines, cstring_convert(prev_data), 1));
	}
	else {
		clist_add(list, process_part(config_lines, cstring_convert(prev_data), 0));
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
			linked_part = (message_part *)parts_stack->last->data;
			free_clist_node(clist_remove(parts_stack, parts_stack->last));
			if (!strcmp(part->data, linked_part->data)) {
				part->link = linked_part;
				linked_part->link = part;
			}
			break;
		case TYPE_MESSAGE:
			break;
		}
	}
	free_clist(parts_stack);

	// Apply some rules (eg: "@ ")
	for (ptr = list->first ; ptr != NULL ; ptr = ptr->next) {
		part = (message_part *)ptr->data;
		cdata = new_cstring();
		cstring_adds(cdata, part->data);
		if (part->type == TYPE_MESSAGE && cstring_ends_withs(cdata, "@ ", 0)) {
			// @ span NICK span
			node = ptr->next;
			clist_remove(list, ptr);
			free_clist_node(ptr);
			ptr = node->next;
			clist_remove(list, node);
			free_clist_node(node);
			node = ptr->next;
			clist_remove(list, node);
			free_clist_node(node);
			part = (message_part *)ptr->data;
			part->type = TYPE_NICK;
		}
		free_cstring(cdata);
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
	
	out = new_cstring();
	switch (part->type) {
	case TYPE_OPENING_TAG:
		for (ptr = config_lines->first ; ptr != NULL ; ptr = ptr->next) {
			line = (config_line *)ptr->data;
			process_message_part_sub(out, line, context_stack, part);
		}
		break;
	case TYPE_CLOSING_TAG:
		for (ptr = config_lines->last ; ptr != NULL ; ptr = ptr->prev) {
			line = (config_line *)ptr->data;
			process_message_part_sub(out, line, context_stack, part);
		}
		break;
	}
	return cstring_convert(out);
}

void process_message_part_sub(cstring *out, config_line *line, clist *context_stack, message_part *part) {
	clist_node *cnode;
	clist_node *rnode;
	clist_node *tnode;
	clist *atts;
	rule *rul;
	char *context;
	attribute *att;
	int in_context, do_apply, global_tag, global_value, tag_equals, value_equals;
	
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
				switch (part->type) {
				case TYPE_OPENING_TAG:
					cstring_adds(out, rul->start);
					break;
				case TYPE_CLOSING_TAG:
					cstring_adds(out, rul->stop);
					break;
				}
			}
		}
	}
}

clist_node *process_part(clist *config_lines, char *data, int text) {
	message_part *part;
	clist_node *node;
	clist *tab, *tab2;
	cstring *tmp;
	cstring *string;
	int i;
	attribute *att;
	
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
				tab2 = cstring_splitc(string, '=', '\"');
				att = new_attribute();
				att->name = cstring_convert((cstring *)tab2->first->data);
				tab2->first->data = NULL;
				if (tab2->first->next == NULL) {
					att->value = cstring_convert(new_cstring());
				} else {
					att->value = cstring_convert((cstring *)tab2->first->next->data);
					tab2->first->next->data = NULL;
				}
				free(tab2);
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
