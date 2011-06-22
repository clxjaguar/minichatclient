/*
  Name:         parser.c
  Author:       Niki
  Description:  Parse HTML inside each messages
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


char *get_text(message_part *message) {
	// TODO
	cstring *result;
	
	result = new_cstring();
	
	if (strcmp(message->data, "strong") == 0) {
		cstring_addc(result, '*');
	} 
	else if (strcmp(message->data, "/strong") == 0) {
		cstring_addc(result, '*');
	} 
	/* // cLx
	else if (strcmp(message->data, "span") == 0) {
		//TODO: check attributes
	} 
	else if (strcmp(message->data, "/span") == 0) {
		//TODO: check linked message
	} 
	*/
	else {
		// Nothing to add
	}
	
	return cstring_convert(result);
}

void configure() {
	// TODO
}

char *parse_html_for_output(char *message) {
	clist *parts;
	clist_node *ptr;
	message_part *part;
	cstring *out;
	char *tmp;
	
	configure();
	
	out = new_cstring();
	parts = get_parts(message);
	for (ptr = parts->first ; ptr != NULL ; ptr = ptr->next) {
		part = (message_part *)ptr->data;
		
		if (part->type == TYPE_MESSAGE) {
			cstring_adds(out, part->data);
		}
		else {
			tmp = get_text(part);
			cstring_adds(out, tmp);
			free(tmp);
		}
	}
	
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

