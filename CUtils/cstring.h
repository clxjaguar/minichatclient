#ifdef __cplusplus
extern "C" {
#endif

#ifndef CSTRING_H
#define CSTRING_H

#include <stdio.h>
#include "clist.h"

/**
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
	cstring_private *priv;
};

// Old
/** Obsolete. */
cstring *new_cstring();
/** Obsolete. */
void free_cstring(cstring *string);
//

/**
 * Instanciate a new cstring.
 *
 * Create (and allocate the memory for) a new cstring.
 * Do not forget to call cstring_free(cstring) when done.
 */
cstring *cstring_new();

/**
 * Free the given cstring.
 *
 * Free all the resources allocated for this cstring.
 *
 * @param string the cstring to free, which MUST NOT be used again afterward
 */
void cstring_free(cstring *string);

/**
 * Add another cstring at the end of the first one.
 *
 * @param self the cstring to work on
 * @param source the cstring to append to 'self'
 */
void cstring_add(cstring *self, cstring *source);

/**
 * Add another cstring at the end of the first one, starting from index.
 *
 * @param self the cstring to work on
 * @param source the cstring to append to 'self'
 * @param index the starting index at which to copy from the source
 */
void cstring_addf(cstring *self, cstring *source, size_t index);

/**
 * Add another cstring at the end of the first one, up to N chars long.
 *
 * @param self the cstring to work on
 * @param source the cstring to append to 'self'
 * @param n the maximum number of chars to add (excluding the NUL byte)
 */
void cstring_addn(cstring *self, cstring *source, size_t n);

/**
 * Add another cstring at the end of the first one, starting from index, up to N chars long.
 *
 * @param self the cstring to work on
 * @param source the cstring to append to 'self'
 * @param index the starting index at which to copy from the source
 * @param n the maximum number of chars to add (excluding the NUL byte)
 */
void cstring_addfn(cstring *self, cstring *source, size_t index, size_t n);

/**
 * Add a char at the end of the given cstring.
 *
 * @param self the cstring to work on
 * @param source the character to add
 */
void cstring_addc(cstring *self, char source);

/**
 * Add a string (a sequence of char that MUST end with '\0') at the end of the given cstring.
 *
 * @param self the cstring to work on
 * @param source the string to add
 */
void cstring_adds(cstring *self, const char source[]);

/**
 * Add a string (a sequence of char that MUST end with '\0') at the end of the given cstring, starting from index.
 *
 * @param self the cstring to work on
 * @param source the string to add
 * @param index the starting index at which to copy from the source
 */
void cstring_addfs(cstring *self, const char source[], size_t index);

/**
 * Add a string (a sequence of char that MAY end with '\0') at the end of the given cstring, up to N chars long.
 *
 * @param self the cstring to work on
 * @param source the string to add
 * @param n the maximum number of chars to add (excluding the NUL byte)
 */
void cstring_addns(cstring *self, const char source[], size_t n);

/**
 * Add a string (a sequence of char that MAY end with '\0') at the end of the given cstring, starting from index, up to N chars long.
 *
 * @param self the cstring to work on
 * @param source the string to add
 * @param index the starting index at which to copy from the source
 * @param n the maximum number of chars to add (excluding the NUL byte)
 */
void cstring_addfns(cstring *self, const char source[], size_t index, size_t n);

/**
 * Add an int at the end of the given cstring.
 *
 * @param self the cstring to work on
 * @param source the integer to add to the string
 */
void cstring_addi(cstring *self, int source);

void cstring_addd(cstring *self, double source, int afterDot);

/**
 * Add an int at the end of the given cstring, in hexadecimal format (lower case).
 *
 * @param self the cstring to work on
 * @param source the hexadecimal value (eg: 0xff10) to add to the string
 */
void cstring_addx(cstring *self, int source);

/**
 * Add an int at the end of the given cstring, in hexadecimal format (upper case).
 *
 * @param self the cstring to work on
 * @param source the hexadecimal value (eg: 0xff10) to add to the string
 */
void cstring_addX(cstring *self, int source);

/**
 * Add a number to the given cstring, in the given format.
 *
 * @param self the cstring to work on
 * @param source is the number to add
 * @param radix 8, 10 or 16 (the radix of the number to add -- anything above 10 will be assigned a letter from a to z (then, kaboom))
 * @param cap capitalize (upper case) the letters if any; 0 = no, 1 = yes
 */
void cstring_addN(cstring *self, int source, int radix, int cap);

/**
 * Cut the cstring at the given size if it is greater.
 * E.g.: it will have (at most) this many characters (without counting NUL) in it after.
 *
 * @param self the cstring to work on
 * @param size the size to cut at (the maximum size of the cstring after this operation, NUL excepted)
 */
void cstring_cut_at(cstring *self, size_t size);

/**
 * Create a substring of this cstring.
 *
 * @param self the cstring to work on
 * @param start the index to start at
 * @param length the number of characters to copy, 0 for 'up to the end'
 *
 * @return a newly allocated cstring
 */
cstring *cstring_substring(cstring *self, size_t start, size_t length);

/**
 * Split a cstring into "smaller" cstrings every time the given separator is found.
 * Will also allow empty fields, ie: "a,b,,c" will return 4 cstrings, the third being empty).
 *
 * @param self the cstring to work on
 * @param delim the separator, which can be longer than one character
 *
 * @return a list of cstring
 */
clist *cstring_split(cstring *self, cstring *delim, cstring *quote);

/**
 * Split a cstring into "smaller" cstrings every time the given separator (which MUST end in \0) is found.
 * Will also allow empty fields, ie: "a,b,,c" will return 4 cstrings, the third being empty).
 *
 * @param self the cstring to work on
 * @param delim the separator, which can be longer than one character and MUST end with \0
 *
 * @return a list of cstring
 */
clist *cstring_splits(cstring *self, const char delim[], const char quote[]);

/**
 * Split a cstring into "smaller" cstrings every time the given separator is found.
 * Will also allow empty fields, ie: "a,b,,c" will return 4 cstrings, the third being empty).
 *
 * @param self the cstring to work on
 * @param delim the separator
 *
 * @return a list of cstring
 */
clist *cstring_splitc(cstring *self, char delim, char quote);

/**
 * Reverse the given cstring.
 *
 * @param self the cstring to work on
 */
void cstring_reverse(cstring *self);

/**
 * Replace all occurences of a cstring inside the given cstring by another.
 *
 * @param self the cstring to work on
 * @param from the cstring to replace
 * @param to the replacement cstring
 *
 * @return the number of occurences changed
 */
int cstring_replace(cstring *self, cstring *from, cstring *to);

/**
 * Replace all occurences of a string inside the given cstring by another.
 *
 * @param self the cstring to work on
 * @param from the string to replace
 * @param to the replacement string
 *
 * @return the number of occurences changed
 */
int cstring_replaces(cstring *self, const char from[], const char to[]);

/**
 * Check if the cstring starts with the given pattern.
 *
 * @param self the cstring to work on
 * @param find the cstring to find
 * @param start_index the index at which to start the comparison
 *
 * @return true if it does
 */
int cstring_starts_with(cstring *self, cstring *find, size_t start_index);

/**
 * Check if the cstring starts with the given pattern.
 *
 * @param self the cstring to work on
 * @param find the string to find
 * @param start_index the index at which to start the comparison
 *
 * @return true if it does
 */
int cstring_starts_withs(cstring *self, const char find[], size_t start_index);

/**
 * Check if the string starts with the given pattern.
 *
 * @param self the string to work on
 * @param find the string to find
 * @param start_index the index at which to start the comparison
 *
 * @return true if it does
 */
int cstring_sstarts_withs(const char string[], const char find[],
		size_t start_index);

/**
 * Check if the cstring ends with the given pattern.
 *
 * @param self the cstring to work on
 * @param find the cstring to find
 * @param start_index the index at which to start the comparison
 *
 * @return true if it does
 */
int cstring_ends_with(cstring *self, cstring *find, size_t start_index);

/**
 * Check if the cstring ends with the given pattern.
 *
 * @param self the cstring to work on
 * @param find the string to find
 * @param start_index the index at which to start the comparison
 *
 * @return true if it does
 */
int cstring_ends_withs(cstring *self, const char find[], size_t start_index);

/**
 * Check if the given string is contained by this one.
 *
 * @param self the cstring to work on
 * @param find the string to find
 * @param start_index the index at which to start the comparison
 *
 * @return the start index of the found string if found, or a negative value if not
 */
long long cstring_find(cstring *self, cstring *find, size_t start_index);

/**
 * Check if the given string is contained by this one.
 *
 * @param self the cstring to work on
 * @param find the string to find
 * @param start_index the index at which to start the comparison
 *
 * @return the start index of the found string if found, or a negative value if not
 */
long long cstring_finds(cstring *self, const char find[], size_t start_index);

/**
 * Check if any of the given characters (in a char* which MUST end with '\0') is found.
 *
 * @param self the cstring to work on
 * @param find the characters to find, which MUST be an array of char that ends with '\0'
 * @param start_index the index at which to start the comparison
 *
 * @return the start index of the first found character if found, or a negative value if not
 */
long long cstring_find_anys(cstring *self, const char find[],
		size_t start_index);

/**
 * Clear (truncate its size to 0) the given cstring.
 *
 * @param self the cstring to work on
 */
void cstring_clear(cstring *self);

/**
 * Convert this cstring into a string
 * This means that you MUST NOT call cstring_free nor use the cstring anymore.
 * NULL will return NULL.
 *
 * @param self the cstring to work on
 */
char *cstring_convert(cstring *self);

/**
 * Clone this cstring.
 * NULL will return NULL.
 *
 * @param self the cstring to clone
 */
cstring *cstring_clone(cstring *self);

/**
 * Clone this string into a new cstring.
 * NULL will return NULL.
 *
 * @param self the string to clone
 */
cstring *cstring_clones(const char self[]);

/**
 * Clone this cstring into a new string.
 * NULL will return NULL.
 *
 * @param self the cstring to clone
 */
char *cstring_sclone(cstring *self);

/**
 * Clone this string into a new string.
 * NULL will return NULL.
 *
 * @param self the string to clone
 */
char *cstring_sclones(const char self[]);

/**
 * Trim this cstring of all 'car' instances from the start and/or the
 * end of the string.
 * 
 * @param self the cstring to work on
 * @param car the character to trim
 * @param start true to trim the start
 * @param end true to trim the end
 * 
 * @return a trimmed cstring
 */
cstring *cstring_trimc(cstring *self, char car, int start, int end);

/**
 * Compact the memory used by this cstring.
 *
 * @param self the cstring to work on
 */
void cstring_compact(cstring *self);

/**
 * Read a whole line (CR, LN or CR+LN terminated) from the given file stream.
 *
 * @param self the cstring to read into
 * @param file the file to read
 *
 * @return true if a line was read, false if not
 */
int cstring_readline(cstring *self, FILE *file);

/**
 * Read a whole line (CR, LN or CR+LN terminated) from the given socket.
 *
 * @param self the cstring to read into
 * @param fd the socket to read from
 *
 * @return true if a line was read, false if not
 */
int cstring_readnet(cstring *self, int fd);

/**
 * Combine two paths and return a newly allocated strings with it.
 * @param dir the first path component
 * @param file the second path component
 *
 * @return a new string that you are responsible to free
 */
cstring *cstring_combine(cstring *self, cstring *file);

/**
 * Combine two paths and return a newly allocated strings with it.
 * @param dir the first path component
 * @param file the second path component
 *
 * @return a new string that you are responsible to free
 */
cstring *cstring_combines(cstring *self, const char file[]);

/**
 * Combine two paths and return a newly allocated strings with it.
 * @param dir the first path component
 * @param file the second path component
 *
 * @return a new string that you are responsible to free
 */
cstring *cstring_scombines(const char dir[], const char file[]);

cstring *cstring_getdir(cstring *path);

cstring *cstring_getdirs(const char path[]);

cstring *cstring_getfile(cstring *path);

cstring *cstring_getfiles(const char path[]);

/**
 * Remove all the \r and \n at the end of the given cstring.
 * 
 * @return how many removed characters
 */
int cstring_remove_crlf(cstring *self);

#endif

#ifdef __cplusplus
}
#endif
