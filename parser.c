/*
  Name:         parser.c
  Author:       Niki
  Description:  Parse HTML inside each messages
  Date:         20/06/11
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
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

(10:20:36) cLx: est ce que vous pouvez cliquer sur cette URL (http://perdu.com) ?
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


char *get_text(message_part_t *message) {
	// TODO
	char *result;
	
	if (strcmp(message->data, "strong") == 0) {
		result = (char *)malloc(sizeof(char) * 2);
		result[0] = '*';
		result[1] = '\0';
	} 
	else if (strcmp(message->data, "/strong") == 0) {
		result = (char *)malloc(sizeof(char) * 2);
		result[0] = '*';
		result[1] = '\0';
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
		result = (char *)malloc(sizeof(char));
		result[0] = '\0';
	}
	
	return result;
}

void configure() {
	// TODO
}

char *parse_html_for_output(char *message) {
	message_part_t **parts;
	int ptr;
	char *out = NULL;
	
	configure();
	
	parts = get_parts(message);
	for (ptr = 0; parts[ptr] != NULL ; ptr++) {
		char *text = NULL;
		if (parts[ptr]->type == TYPE_MESSAGE) {
			text = parts[ptr]->data;
		}
		else {
			text = get_text(parts[ptr]);
		}	
		if (!out) { 
			out = malloc(sizeof(char) + strlen(text) + 1);
			strcpy(out, text);
		}
		else {
			out = realloc(out, sizeof(char) * (strlen(out) + strlen(text) + 1));
			out = strcat(out, text);
		}
		if (parts[ptr]->type != TYPE_MESSAGE) {
			printf("* Line %d : free(text)\n", __LINE__);
			free(text);
			text = NULL;
		}
	}
	free_message_parts(parts);
	return out;
}

message_part_t **get_parts(char *message) {
	message_part_t **list;
	int i_list;
	char *prev_data;
	int bracket;
	int i;
	char car;
	
	list = malloc(sizeof(message_part_t *) * 1);
	i_list = 0;
	list[i_list] = NULL;
	
	prev_data = malloc(sizeof(char) * 1);
	prev_data[0] = '\0';
	
	bracket = 0;
	i = 0;
	
	for (car = message[i] ; car != '\0' ; car = message[++i]) {
		if (!bracket && car == '<') {
			bracket = 1;
			list = message_part_add_to_list(list, process_part(prev_data, 1));
			prev_data = malloc(sizeof(char) * 1);
			prev_data[0] = '\0';
		} 
		else if (bracket && car == '>') {
			bracket = 0;
			list = message_part_add_to_list(list, process_part(prev_data, 0));
			prev_data = malloc(sizeof(char) * 1);
			prev_data[0] = '\0';
		} 
		else {	
			prev_data = add_char(prev_data, car);
		}
	}
	
	if (!bracket) {
		list = message_part_add_to_list(list, process_part(prev_data, 1));
	}
	else {
		list = message_part_add_to_list(list, process_part(prev_data, 0));
	}
	return list;
}

void free_message_part(message_part_t* message) {
	int i;
	
	if (message->data != NULL) {
		printf("* Line %d : free(message->data)\n", __LINE__);
		free(message->data);
		message->data = NULL;
	}
	if (message->attributes != NULL) {
		for (i = 0 ; message->attributes[i] != NULL ; i++) {
			attribute_t *attribute = message->attributes[i];
			if (attribute->name != NULL) {
				printf("* Line %d : free(attribute->name)\n", __LINE__);
				free(attribute->name);
				attribute->name = NULL;
			}
			if (attribute->value != NULL) {
				printf("* Line %d : free(attribute->value)\n", __LINE__);
				free(attribute->value);
				attribute->value = NULL;
			}
		}
	}
	printf("* Line %d : free(message)\n", __LINE__);
	free(message);
	message = NULL;
}

void free_message_parts(message_part_t ** messages) {
	int i;
	
	if (messages != NULL) {
		for (i = 0 ; messages[i] != NULL ; i++) {
			message_part_t *message = messages[i];
			free_message_part(message);
		}
	}

	printf("* Line %d : free(message)\n", __LINE__);
	free(messages);
	messages = NULL;
}

message_part_t *process_part(char *data, int text) {
	message_part_t *part;
	part = malloc(sizeof(message_part_t));

	//TODO
	part->type = text?TYPE_MESSAGE:TYPE_OPENING_TAG;
	part->data = data;
	part->link = NULL;
	part->attributes = NULL;
	
	return part;
}

message_part_t **message_part_add_to_list(message_part_t **list, message_part_t *part) {
	int i;
	message_part_t *ptr;
	
	ptr = NULL;
	i = 0;
	
	if (list != NULL)
		for (ptr = list[i] ; ptr != NULL ; ptr = list[++i]);
	else {
		list = malloc(sizeof(message_part_t *));
		list[0] = NULL;
	}
	
	list[i] = part;
	list = (message_part_t **)realloc(list, sizeof(message_part_t *) * (i + 2));
	list[i + 1] = NULL;
	
	return list;
}

char *add_char(char *dest, char car) {
	int i;
	
	i = strlen(dest);
	dest = (char *)realloc(dest, sizeof(char) * (i + 2));
	dest[i] = car;
	dest[i + 1] = '\0';
	
	return dest;
}


