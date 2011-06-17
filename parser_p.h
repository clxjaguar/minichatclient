#ifndef PARSER_P_H
#define PARSER_P_H
#include "parser.h"
#include "ini.h"

int TYPE_MESSAGE = 0;
int TYPE_OPENING_TAG = 1;
int TYPE_CLOSING_TAG = 2;

typedef struct message_part message_part_t;
struct message_part {
	int type; // (0 or 1 or 2: 0 is text, 1 is <>, 2 is </>)
	char *data;
	message_part_t *link; // (NULL for text)
	attribute_t **attributes; // (NULL for text)
};

char *get_date();
message_part_t **get_parts(char *message);
char *get_text(message_part_t *message);
void configure();
char *add_char(char *dest, char car);
message_part_t **message_part_add_to_list(message_part_t **list, message_part_t *part);
message_part_t *process_part(char *data, int text);
void free_message_part(message_part_t* message);
void free_message_parts(message_part_t** messages);

#endif
