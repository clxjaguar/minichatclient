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
	return (!strcmp(att->name, "context") || !strcmp(att->name, "tag") || !strcmp(att->name, "value") || !strcmp(att->name, "use") || !strcmp(att->name, "end-tag"));
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

char *parse_html_for_output(char *message, parser_config *pconfig) {
	clist *parts;
	clist_node *ptr;
	message_part *part;
	cstring *out;
	cstring *tmp;
	clist *context_stack;
	clist_node *context_node;	
	clist *configs;
	
	configs = pconfig->data->config_lines;
	
	out = new_cstring();
	parts = get_parts(message);
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
			
			// Work:
			cstring_adds(out, process_message_part(configs, context_stack, part));
		break;
		case TYPE_CLOSING_TAG:
			if(context_stack->size > 0 && !strcmp(part->data, (char *)context_stack->last->data)) {
				context_node = clist_remove(context_stack, context_stack->last);
				free_clist_node(context_node);

				// Work:
				cstring_adds(out, process_message_part(configs, context_stack, part));
			}
		break;
		}
	}

	free_clist(context_stack);
	free_clist(parts);
	return cstring_convert(out);
}

clist *get_parts(char *message) {
	clist *list;
	int i_list;
	cstring *prev_data;
	int bracket;
	int i;
	char car;
	
	//free_message_part_node
	
	list = new_clist();
	prev_data = new_cstring();
	
	bracket = 0;
	i = 0;
	
	for (car = message[i] ; car != '\0' ; car = message[++i]) {
		if (!bracket && car == '<') {
			bracket = 1;
			clist_add(list, process_part(cstring_convert(prev_data), 1));
			prev_data = new_cstring();
		} 
		else if (bracket && car == '>') {
			bracket = 0;
			clist_add(list, process_part(cstring_convert(prev_data), 0));
			prev_data = new_cstring();
		} 
		else {
			cstring_addc(prev_data, car);
		}
	}
	
	if (!bracket) {
		clist_add(list, process_part(cstring_convert(prev_data), 1));
	}
	else {
		clist_add(list, process_part(cstring_convert(prev_data), 0));
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
	attribute *att;
	
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
	clist_node *anode;
	rule *rul;
	char *context;
	attribute *att;
	
	//TODO
	return;
	
	for (rnode = line->rules->first ; rnode != NULL ; rnode = rnode->next) {
		rul = (rule *)rnode->data;
		for (cnode = context_stack->first ; cnode != NULL ; cnode = cnode->next) {
			context = (char *)cnode->data;
			//if (rul->context == NULL || rul->context[0] == '\0' || !strcmp(rul->context, context)) {
				if (!strcmp(rul->tag, part->data)) {
					for (anode = part->attributes->first ; anode != NULL ; anode = anode->next) {
						att = (attribute *)anode->data;
						//TODO: check each attribute
					}
				}
			//}
		}
	}
}

clist_node *process_part(char *data, int text) {
	message_part *part;
	clist_node *node;
	
	part = malloc(sizeof(message_part));

	//TODO
	part->type = text ? TYPE_MESSAGE : TYPE_OPENING_TAG;
	part->data = data;
	part->link = NULL;
	part->attributes = NULL;
	
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

free_parser_config(parser_config *pconfig) {
	if (pconfig->data->config_lines != NULL) {
		free_clist(pconfig->data->config_lines);
	}
	free(pconfig->data);
	free(pconfig);
}
