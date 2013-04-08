#include <stdio.h>

#include "htmlnode.h"
#include "cstring.h"

// private prototypes:
int htmlnode_parse_dom(htmlnode *self, FILE *file, int fd, const char input[]);
int htmlnode_parse_sax(FILE *file, int fd, const char input[],void (*callback)(htmlnode_elem *elem, void *data), void *data);
int htmlnode_read(cstring *string, FILE *file, int fd, const char input[]);
void htmlnode_text_found(cstring *text, void (*callback)(htmlnode_elem *elem, void *data), void *data);
void htmlnode_tag_found(cstring *name, int open, void (*callback)(htmlnode_elem *elem, void *data), void *data);
void htmlnode_attribute_found(cstring *key, cstring *value, cstring *tag, void (*callback)(htmlnode_elem *elem, void *data), void *data);

// private enum:
enum HTMLNODE_STATE {
	HTMLNODE_TEXT, // text mode (out of <>)
	HTMLNODE_TAG_UNTAG, // look for '/' or not
	HTMLNODE_TAG_NAMING, // look for ' ', add char to name if not
	HTMLNODE_UNTAG_NAMING, // look for ' ', add char to name if not
	HTMLNODE_TAG_NAMED, // look for ^[>]
	HTMLNODE_TAG_INATT_NAMING, // look for =
	HTMLNODE_TAG_INATT_NAMED, // look for " or other non-space
	HTMLNODE_TAG_INATT_NAMED_WAIT_EQUALS, // look for " or other non-space
	HTMLNODE_TAG_INATT_NAMED_QUOTED, // look for closing "
	HTMLNODE_TAG_INATT_NAMED_UNQUOTED, // look for ' '
};

htmlnode *htmlnode_new() {
	htmlnode *self;

	self = (htmlnode *)malloc(sizeof(htmlnode));
	self->name = NULL;
	self->attributes = NULL;
	self->nodes = NULL;
	self->parent = NULL;
	self->text = NULL;

	return self;
}

void htmlnode_free(htmlnode *self) {
	if (!self)
		return;

	free(self->name);
	free(self->text);
	clist_free(self->attributes);
	clist_free(self->nodes);
	// note: we DO NOT free the parent

	free(self);
}

htmlnode_elem *htmlnode_elem_new() {
	htmlnode_elem *self;

	self = (htmlnode_elem *)malloc(sizeof(htmlnode_elem));
	self->type = HTMLNODE_ELEM_TEXT;
	self->text = NULL;

	return self;
}

void htmlnode_elem_free(htmlnode_elem *self) {
	if (!self)
		return;

	switch (self->type) {
	case HTMLNODE_ELEM_TEXT:
		free(self->text);
	break;
	case HTMLNODE_ELEM_TAG:
		free(self->name);
	break;
	case HTMLNODE_ELEM_ATTRIBUTE:
		free(self->tag);
		free(self->key);
		free(self->value);
	break;
	default:
	break;
	}

	free(self);
}

int htmlnode_parse_dom_file(htmlnode *self, FILE *file) {
	if (file)
		rewind(file);

	return htmlnode_parse_dom(self, file, -1, NULL);
}

int htmlnode_parse_dom_net(htmlnode *self, int fd) {
	return htmlnode_parse_dom(self, NULL, fd, NULL);
}

int htmlnode_parse_dom_str(htmlnode *self, const char input[]) {
	return htmlnode_parse_dom(self, NULL, -1, input);
}

int htmlnode_parse_sax_file(FILE *file, void (*callback)(htmlnode_elem *elem, void *data), void *data) {
	return htmlnode_parse_sax(file, -1, NULL, callback, data);
}

int htmlnode_parse_sax_net(int fd, void (*callback)(htmlnode_elem *elem, void *data), void *data) {
	return htmlnode_parse_sax(NULL, fd, NULL, callback, data);
}

int htmlnode_parse_sax_str(const char input[], void (*callback)(htmlnode_elem *elem, void *data), void *data) {
	return htmlnode_parse_sax(NULL, -1, input, callback, data);
}

int htmlnode_read(cstring *string, FILE *file, int fd, const char input[]) {
	// do not read from char *input if string is not empty or if input is empty
	if (!string->length && input && input[0]) {
		cstring_adds(string, input);
		return 1;
	}

	if (file)
		return cstring_readline(string, file);

	if (fd != -1)
		return cstring_readnet(string, fd);

	return 0;
}

void htmlnode_text_found(cstring *text, void (*callback)(htmlnode_elem *elem, void *data), void *data) {
	htmlnode_elem *elem;

	elem = htmlnode_elem_new();
	elem->type = HTMLNODE_ELEM_TEXT;
	elem->text = cstring_sclone(text);

	callback(elem, data);

	htmlnode_elem_free(elem);
}

void htmlnode_tag_found(cstring *name, int open, void (*callback)(htmlnode_elem *elem, void *data), void *data) {
	htmlnode_elem *elem;
	cstring *s;

	elem = htmlnode_elem_new();
	elem->type = HTMLNODE_ELEM_TAG;
	elem->name = cstring_sclone(name);

	if (open) {
		callback(elem, data);
	} else {
		s = cstring_new();
		cstring_addc(s, '/');
		cstring_add(s, name);
		
		free(elem->name);
		elem->name = cstring_convert(s);
		
		callback(elem, data);
	}

	htmlnode_elem_free(elem);
}

void htmlnode_attribute_found(cstring *key, cstring *value, cstring *tag, void (*callback)(htmlnode_elem *elem, void *data), void *data) {
	htmlnode_elem *elem;

	elem = htmlnode_elem_new();
	elem->type = HTMLNODE_ELEM_ATTRIBUTE;
	elem->key = cstring_sclone(key);
	elem->value = cstring_sclone(value);
	elem->tag = cstring_sclone(tag);

	callback(elem, data);

	htmlnode_elem_free(elem);
}

int htmlnode_parse_dom(htmlnode *self, FILE *file, int fd, const char input[]) {
	//TODO
	if (self) {}
	if (file) {}
	if (fd) {}
	if (input) {}
	return 0;
}

int htmlnode_parse_sax(FILE *file, int fd, const char input[], void (*callback)(htmlnode_elem *elem, void *data), void *data) {
	cstring *str, *text, *name, *key, *value;
	size_t i;
	enum HTMLNODE_STATE state;
	char prev;
	
	if (file)
		rewind(file);

	prev = '\0';
	str = cstring_new();
	text = cstring_new();
	name = cstring_new();
	key = cstring_new();
	value = cstring_new();

	state = HTMLNODE_TEXT;
	while (htmlnode_read(str, file, fd, input)) {
		for (i = 0 ; i < str->length ; i++) {
			switch (state) {
			// Normal mode
			case HTMLNODE_TEXT:
				if (str->string[i] == '<') {
					if (text->length)
						htmlnode_text_found(text, callback, data);
					
					cstring_clear(text);
					cstring_clear(name);

					state = HTMLNODE_TAG_UNTAG;
				} else {
					cstring_addc(text, str->string[i]);
				}
			break;
			// We just encountered a <
			case HTMLNODE_TAG_UNTAG:
				if (str->string[i] == '/') {
					state = HTMLNODE_UNTAG_NAMING;
				} else if (str->string[i] != ' ' && str->string[i] != '\t') {
					state = HTMLNODE_TAG_NAMING;
					cstring_addc(name, str->string[i]);
				}
			break;
			// We are in a tag, looking for its name
			case HTMLNODE_TAG_NAMING:
				if (str->string[i] == '>' 
						|| str->string[i] == ' ' 
						|| str->string[i] == '\t') {

					if (prev == '/')
						cstring_cut_at(name, name->length - 1);
					htmlnode_tag_found(name, 1, callback, data);
					if (prev == '/')
						htmlnode_tag_found(name, 0, callback, data);

					if (str->string[i] == '>')
						state = HTMLNODE_TEXT;
					else
						state = HTMLNODE_TAG_NAMED;
				} else {
					cstring_addc(name, str->string[i]);
				}
			break;
			// We are in a closing tag, looking for its name
			case HTMLNODE_UNTAG_NAMING:
				if (str->string[i] == '>') {
					htmlnode_tag_found(name, 0, callback, data);
					state = HTMLNODE_TEXT;
				} else {
					cstring_addc(name, str->string[i]);
				}
			break;
			// We are in a tag, we know its name, it is still not closed
			case HTMLNODE_TAG_NAMED:
				if (str->string[i] == '>') {
					if (prev == '/')
						htmlnode_tag_found(name, 0, callback, data);
					
					state = HTMLNODE_TEXT;
				} else if (str->string[i] != ' ' && str->string[i] != '\t') {
					cstring_addc(key, str->string[i]);
					state = HTMLNODE_TAG_INATT_NAMING;
				}
			break;
			// We found an attribute in a tag, we are looking for its name
			case HTMLNODE_TAG_INATT_NAMING:
				if (str->string[i] == '>') {
					if (prev == '/')
						cstring_cut_at(key, key->length - 1);

					if (key->length) {
						htmlnode_attribute_found(key, NULL, name, callback, data);
						cstring_clear(key);
					}

					if (prev == '/')
						htmlnode_tag_found(name, 0, callback, data);

					state = HTMLNODE_TEXT;
				} else if (str->string[i] == ' ' 
						|| str->string[i] == '\t') {
					state = HTMLNODE_TAG_INATT_NAMED_WAIT_EQUALS;
				} else if (str->string[i] == '=') {
					state = HTMLNODE_TAG_INATT_NAMED;
				} else {
					cstring_addc(key, str->string[i]);
				}
			break;
			// We are in a tag, we found an attribute, we know its name,
			// we look for = or a new tag
			case HTMLNODE_TAG_INATT_NAMED_WAIT_EQUALS:
				if (str->string[i] == '>') {
					if (prev == '/')
						cstring_cut_at(key, key->length - 1);
					
					htmlnode_attribute_found(key, NULL, name, callback, data);
					cstring_clear(key);

					if (prev == '/')
						htmlnode_tag_found(name, 0, callback, data);
					
					state = HTMLNODE_TEXT;
				} else if (str->string[i] == '=') {
					state = HTMLNODE_TAG_INATT_NAMED;
				} else if (str->string[i] != ' ' && str->string[i] != '\t') {
					htmlnode_attribute_found(key, NULL, name, callback, data);
					
					cstring_clear(key);
					
					cstring_addc(key, str->string[i]);
					
					state = HTMLNODE_TAG_NAMED;
				}
			break;
			// We are in a tag, we found an attribute, we know its name,
			// it was followed by an = sign (the value should come shortly)
			case HTMLNODE_TAG_INATT_NAMED:
				if (str->string[i] == '>') {
					if (prev == '/')
						cstring_cut_at(key, key->length - 1);

					htmlnode_attribute_found(key, value, name, callback, data);
					
					cstring_clear(key);
					cstring_clear(value);
					
					if (prev == '/')
						htmlnode_tag_found(name, 0, callback, data);

					state = HTMLNODE_TEXT;
				} else if (str->string[i] != ' ' && str->string[i] != '\t') {
					if (str->string[i] == '"') {
						state = HTMLNODE_TAG_INATT_NAMED_QUOTED;
					} else {
						cstring_addc(value, str->string[i]);
						state = HTMLNODE_TAG_INATT_NAMED_UNQUOTED;
					}
				}
			break;
			// We are in a tag, in an attribute, we know its name, it was
			// followed by =, the value is quoted and we already passed the first '
			case HTMLNODE_TAG_INATT_NAMED_QUOTED:
				if (str->string[i] == '"') {
					htmlnode_attribute_found(key, value, name, callback, data);
					
					cstring_clear(key);
					cstring_clear(value);

					state = HTMLNODE_TAG_NAMED;
				} else {
					cstring_addc(value, str->string[i]);
				}
			break;
			// We are in a tag, in an attribute, we know its name, it was
			// followed by =, the value is not quoted, and we already got a char
			case HTMLNODE_TAG_INATT_NAMED_UNQUOTED:
				if (str->string[i] == '>') {
					if (prev == '/')
						cstring_cut_at(value, value->length - 1);

					htmlnode_attribute_found(key, value, name, callback, data);
					
					cstring_clear(key);
					cstring_clear(value);
					
					if (prev == '/')
						htmlnode_tag_found(name, 0, callback, data);

					state = HTMLNODE_TEXT;
				} else if (str->string[i] == ' ' || str->string[i] == '\t') {
					htmlnode_attribute_found(key, value, name, callback, data);
					
					cstring_clear(key);
					cstring_clear(value);
					
					state = HTMLNODE_TAG_NAMED;
				} else {
					cstring_addc(value, str->string[i]);
				}
			break;
			default:
				state = HTMLNODE_TEXT;
			break;
			}

			prev = str->string[i]; // to be able to identify "/>"
		}
	}
	
	if (text->length)
		htmlnode_text_found(text, callback, data);

	cstring_free(str);
	cstring_free(text);
	cstring_free(name);
	cstring_free(key);
	cstring_free(value);

	return 1;
}

