#ifndef PARSER_P_H
#define PARSER_P_H

#define TYPE_MESSAGE      0
#define TYPE_OPENING_TAG  1
#define TYPE_CLOSING_TAG  2

/**
 * This is an attribute (a key-value pair).
 * You should use attribute instead of this struct.
 * Note that the name cannot be NULL, but the value can.
 * Edit (cLx) : 
 * [NOTE : THIS SECTION HAS BEEN COPIED FROM INI.H]
 */
struct attribute_struct {
	char *name;
	char *value;
};

/**
 * This is an attribute (a key-value pair).
 */
typedef struct attribute_struct attribute;

/*
[END OF THE COPY'D SECTION]
*/

typedef struct message_part message_part_t;
struct message_part {
	int type; // (0 or 1 or 2: 0 is text, 1 is <>, 2 is </>)
	char *data;
	message_part_t *link; // (NULL for text)
	attribute **attributes; // (NULL for text)
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
