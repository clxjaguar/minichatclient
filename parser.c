#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "parser_p.h"

/*


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
	} else if (strcmp(message->data, "/strong") == 0) {
		result = (char *)malloc(sizeof(char) * 2);
		result[0] = '*';
		result[1] = '\0';
	} else if (strcmp(message->data, "span") == 0) {
		//TODO: check attributes
	} else if (strcmp(message->data, "/span") == 0) {
		//TODO: check linked message
	} else {
		result = (char *)malloc(sizeof(char));
		result[0] = '\0';
	}
	
	return result;
}

void configure() {
	// TODO
}

char *get_date() {
	// TODO
	char *date = malloc(sizeof(char) * strlen("[TimeStamp]: "));
	strcpy(date, "[TimeStamp]: ");
	return date;
}

char *parse_html_for_output(char *message) {
	message_part_t **parts;
	int ptr;
	char *out;
	
	configure();
	out = get_date();
	
	parts = get_parts(message);
	for (ptr = 0; parts[ptr] != NULL ; ptr++) {
		char *text = NULL;
		if (parts[ptr]->type == TYPE_MESSAGE)
			text = parts[ptr]->data;
		else
			text = get_text(parts[ptr]);
			
		out = realloc(out, sizeof(char) * (strlen(out) + strlen(text) + 1));
		out = strcat(out, text);
		
		if (parts[ptr]->type != TYPE_MESSAGE)
			free(text);
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
		} else if (bracket && car == '>') {
			bracket = 0;
			list = message_part_add_to_list(list, process_part(prev_data, 0));
			prev_data = malloc(sizeof(char) * 1);
			prev_data[0] = '\0';
		} else {	
			prev_data = add_char(prev_data, car);
		}
	}
	
	if (!bracket) 
		list = message_part_add_to_list(list, process_part(prev_data, 1));
	else
		list = message_part_add_to_list(list, process_part(prev_data, 0));
	
	return list;
}

void free_message_part(message_part_t* message) {
	int i;
	
	if (message->data != NULL)
		free(message->data);
	if (message->attributes != NULL) {
		for (i = 0 ; message->attributes[i] != NULL ; i++) {
			attribute_t *attribute = message->attributes[i];
			if (attribute->name != NULL)
				free(attribute->name);
			if (attribute->value != NULL)
				free(attribute->value);
		}
	}
	
	free(message);
}

void free_message_parts(message_part_t ** messages) {
	int i, ii;
	
	if (messages != NULL) {
		for (i = 0 ; messages[i] != NULL ; i++) {
			message_part_t *message = messages[i];
			free_message_part(message);
		}
	}
	
	free(messages);
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


