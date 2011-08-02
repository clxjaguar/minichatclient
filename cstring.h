#ifndef CSTRING_H
#define CSTRING_H
#include "clist.h"

/**
 * This is a cstring.
 *
 * This is a cstring. It contains a suite of characters terminated by a NUL byte
 * (or just a NUL byte), and a length.
 * It is advised NOT to modify either of those directly.
 * You can use cstring_convert to get a char*, though (in this case, the cstring
 * MUST NOT be used again, and you are resposible for freeing the said char*).
 */
typedef struct cstring_struct cstring;
typedef struct cstring_private_struct cstring_private;

struct cstring_struct {
	char *string;
	size_t length;
	cstring_private *private;
};

/**
 * Instanciate a new cstring.
 *
 * Create (and allocate the memory for) a new cstring.
 * Do not forget to call free_cstring(cstring) when done.
 */
cstring *new_cstring();

/**
 * Free the given cstring.
 *
 * Free all the resources allocated for this cstring.
 *
 * @param string:
 * 	the cstring to free, which MUST NOT be used again afterward
 */
void free_cstring(cstring *string);

/**
 * Add another cstring at the end of the first one.
 * 
 * Add another cstring at the end of the first one.
 * 
 * @param self:
 * 	the cstring to work on
 * @param source:
 * 	the cstring to append to 'self'
 */
void cstring_add(cstring *self, cstring *source);

/**
 * Add another cstring at the end of the first one, starting from index.
 * 
 * Add another cstring at the end of the first one, starting from index.
 * 
 * @param self:
 * 	the cstring to work on
 * @param source:
 * 	the cstring to append to 'self'
 * @param index:
 * 	the starting index at which to copy from the source
 */
void cstring_addf(cstring *self, cstring *source, int index);

/**
 * Add another cstring at the end of the first one, up to N chars long.
 *
 * Add another cstring at the end of the first one, up to N chars long.
 * 
 * @param self:
 * 	the cstring to work on
 * @param source:
 * 	the cstring to append to 'self'
 * @param n:
 * 	the maximum number of chars to add (excluding the NUL byte)
 */
void cstring_addn(cstring *self, cstring *source, int n);

/**
 * Add another cstring at the end of the first one, starting from index, up to N chars long.
 *
 * Add another cstring at the end of the first one, starting from index, up to N chars long.
 * 
 * @param self:
 * 	the cstring to work on
 * @param source:
 * 	the cstring to append to 'self'
 * @param index:
 * 	the starting index at which to copy from the source
 * @param n:
 * 	the maximum number of chars to add (excluding the NUL byte)
 */
void cstring_addfn(cstring *self, cstring *source, int index, int n);

/**
 * Add a char at the end of the given cstring.
 * 
 * Add a char at the end of the given cstring.
 *
 * @param self:
 * 	the cstring to work on
 * @param source:
 * 	the character to add
 */
void cstring_addc(cstring *self, char source);

/**
 * Add a string at the end of the given cstring.
 * 
 * Add a string (a sequence of char that MUST end with '\0') at the end of the given cstring.
 * 
 * @param self:
 * 	the cstring to work on
 * @param source:
 * 	the string to add
 */
void cstring_adds(cstring *self, char *source);

/**
 * Add a string at the end of the given cstring, starting from index.
 * 
 * Add a string (a sequence of char that MUST end with '\0') at the end of the given cstring, starting from index.
 * 
 * @param self:
 * 	the cstring to work on
 * @param source:
 * 	the string to add
 * @param index:
 * 	the starting index at which to copy from the source
 */
void cstring_addfs(cstring *self, char *source, int index);

/**
 * Add a string at the end of the given cstring, up to N chars long.
 * 
 * Add a string (a sequence of char that MAY end with '\0') at the end of the given cstring, up to N chars long.
 * 
 * @param self:
 * 	the cstring to work on
 * @param source:
 * 	the string to add
 * @param n:
 * 	the maximum number of chars to add (excluding the NUL byte)
 */
void cstring_addns(cstring *self, char *source, int n);

/**
 * Add a string at the end of the given cstring, starting from index, up to N chars long.
 * 
 * Add a string (a sequence of char that MAY end with '\0') at the end of the given cstring, starting from index, up to N chars long.
 * 
 * @param self:
 * 	the cstring to work on
 * @param source:
 * 	the string to add
 * @param index:
 * 	the starting index at which to copy from the source
 * @param n:
 * 	the maximum number of chars to add (excluding the NUL byte)
 */
void cstring_addfns(cstring *self, char *source, int index, int n);

/**
 * Add an int at the end of the given cstring.
 *
 * Add an int at the end of the given cstring.
 *
 * @param self:
 * 	the cstring to work on
 * @param source:
 * 	the integer to add to the string
 */
void cstring_addi(cstring *self, int source);

/**
 * Add an int at the end of the given cstring, in hexadecimal format (lower case).
 *
 * Add an int at the end of the given cstring, in hexadecimal format (lower case).
 *
 * @param self:
 * 	the cstring to work on
 * @param source:
 * 	the hexadecimal value (eg: 0xff10) to add to the string
 */
void cstring_addx(cstring *self, int source);

/**
 * Add an int at the end of the given cstring, in hexadecimal format (upper case).
 *
 * Add an int at the end of the given cstring, in hexadecimal format (upper case).
 *
 * @param self:
 * 	the cstring to work on
 * @param source:
 * 	the hexadecimal value (eg: 0xff10) to add to the string
 */
void cstring_addX(cstring *self, int source);

/**
 * Add a number to the given cstring, in the given format.
 *
 * Add a number to the given cstring, in the given format.
 *
 * @param self:
 * 	the cstring to work on
 * @param source:
 * 	is the number to add
 * @param radix:
 * 	8, 10 or 16 (the radix of the number to add -- anything above 10 will be assigned a letter from a to z (then, kaboom))
 * @param cap:
 * 	capitalize (upper case) the letters if any; 0 = no, 1 = yes
 */
void cstring_addN(cstring *self, int source, int radix, int cap);

/**
 * Cut the cstring at the given size if it is greater.
 *
 * Cut the cstring at the given size if it is greater.
 * E.g.: it will have (at most) this many characters (without counting NUL) in it after.
 *
 * @param self:
 * 	the cstring to work on
 * @param size:
 * 	the size to cut at (the maximum size of the cstring after this operation, NUL excepted)
 */
void cstring_cut_at(cstring *self, size_t size);

/**
 * Create a substring of this cstring.
 *
 * Create a substring of this cstring.
 *
 * @param self:
 * 	the cstring to work on
 * @param start:
 * 	the index to start at
 * @param length:
 * 	the number of characters to copy, -1 for 'up to the end'
 * @return:
 * 	a newly allocated cstring
 */
cstring *cstring_substring(cstring *self, int start, int length);

/**
 * Split a cstring into "smaller" cstrings every time the given separator is found.
 *
 * Split a cstring into "smaller" cstrings every time the given separator is found.
 * Will also allow empty fields, ie: "a,b,,c" will return 4 cstrings, the third being empty).
 *
 * @param self:
 * 	the cstring to work on
 * @param delim:
 * 	the separator, which can be longer than one character 
 * @return:
 * 	A list of cstring
 */
clist *cstring_split(cstring *self, cstring *delim, cstring *quote);

/**
 * Split a cstring into "smaller" cstrings every time the given separator is found.
 *
 * Split a cstring into "smaller" cstrings every time the given separator (which MUST end in \0) is found.
 * Will also allow empty fields, ie: "a,b,,c" will return 4 cstrings, the third being empty).
 *
 * @param self:
 * 	the cstring to work on
 * @param delim:
 * 	the separator, which can be longer than one character and MUST end with \0
 * @return:
 * 	A list of cstring
 */
clist *cstring_splits(cstring *self, char *delim, char *quote);

/**
 * Split a cstring into "smaller" cstrings every time the given separator is found.
 *
 * Split a cstring into "smaller" cstrings every time the given separator is found.
 * Will also allow empty fields, ie: "a,b,,c" will return 4 cstrings, the third being empty).
 *
 * @param self:
 * 	the cstring to work on
 * @param delim:
 * 	the separator 
 * @return:
 * 	A list of cstring
 */
clist *cstring_splitc(cstring *self, char delim, char quote);

/**
 * Reverse the given cstring.
 *
 * Reverse the given cstring.
 *
 * @param self:
 * 	the cstring to work on
 */
void cstring_reverse(cstring *self);

/**
 * Replace all occurences of a cstring insode the given cstring by another.
 *
 * Replace all occurences of a cstring insode the given cstring by another.
 * 
 * @param self:
 * 	the cstring to work on
 * @param from:
 * 	the cstring to replace
 * @param to:
 * 	the replacement cstring
 * @return:
 * 	the number of occurences changed
 */
int cstring_replace(cstring *self, cstring *from, cstring *to);

/**
 * Check if the cstring starts with the given pattern.
 *
 * Check if the cstring starts with the given pattern.
 *
 * @param self:
 * 	the cstring to work on
 * @param find:
 * 	the cstring to find
 * @param start_index:
 * 	the index at which to start the comparison
 * @return:
 * 	true if it does
 */
int cstring_starts_with(cstring *self, cstring *find, int start_index);

/**
 * Check if the cstring starts with the given pattern.
 *
 * Check if the cstring starts with the given pattern.
 *
 * @param self:
 * 	the cstring to work on
 * @param find:
 * 	the string to find
 * @param start_index:
 * 	the index at which to start the comparison
 * @return:
 * 	true if it does
 */
int cstring_starts_withs(cstring *self, char *find, int start_index);

/**
 * Check if the cstring ends with the given pattern.
 *
 * Check if the cstring ends with the given pattern.
 *
 * @param self:
 * 	the cstring to work on
 * @param find:
 * 	the cstring to find
 * @param start_index:
 * 	the index at which to start the comparison
 * @return:
 * 	true if it does
 */
int cstring_ends_with(cstring *self, cstring *find, int start_index);

/**
 * Check if the cstring ends with the given pattern.
 *
 * Check if the cstring ends with the given pattern.
 *
 * @param self:
 * 	the cstring to work on
 * @param find:
 * 	the string to find
 * @param start_index:
 * 	the index at which to start the comparison
 * @return:
 * 	true if it does
 */
int cstring_ends_withs(cstring *self, char *find, int start_index);

/*
 * Check if the given string is contained by this one.
 *
 * @param self:
 * 	the cstring to work on
 * @param find:
 * 	the string to find
 * @param start_index:
 * 	the index at which to start the comparison
 * @return:
 * 	the start index of the found string if found, or a negative value if not
 */
int cstring_find(cstring *self, cstring *find, int start_index);

/*
 * Check if the given string is contained by this one.
 *
 * @param self:
 * 	the cstring to work on
 * @param find:
 * 	the string to find
 * @param start_index:
 * 	the index at which to start the comparison
 * @return:
 * 	the start index of the found string if found, or a negative value if not
 */
int cstring_finds(cstring *self, char *find, int start_index);

/*
 * Clear the given cstring.
 *
 * Clear (truncate its size to 0) the given cstring.
 *
 * @param self:
 * 	the cstring to work on
 */
void cstring_clear(cstring *self);

/**
 * Convert this cstring into a string.
 *
 * Convert this cstring into a string
 * This means that you MUST NOT call free_cstring nor use the cstring anymore.
 * NULL will return NULL.
 *
 * @param self:
 * 	the cstring to work on
 */
char *cstring_convert(cstring *self);

#endif
