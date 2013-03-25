#ifdef __cplusplus
extern "C" {
#endif

#ifndef INI_H
#define INI_H

#include <stdio.h>
#include "attribute.h"
#include "clist.h"

typedef struct {
	FILE *file;
	int trim;
} ini;

ini *ini_new();

ini *ini_new_f(FILE *file);

ini *ini_new_ft(FILE *file, int trim);

void ini_free(ini *self);

/**
 * (File: Only one level without [sections], comments starts with #, whole line only.)
 * It will only return the attributes that match the given name (or NULL for all),
 * in a clist of <attribute>s.
 */
clist *ini_get_all(ini *self, const char mask[]);

/**
 * Return all the attributes that match the filter.
 * Will never be NULL (but can be empty).
 *
 * @param file the file to get the attribute's from
 * @param filter the filter to apply on each incoming attribute to know if
 * 	we keep it or not
 * @param argument the user argument to pass to the filter
 *
 * @return a clist of attribute's or an empty clist
 */
clist *ini_get_select(ini *self, int (*filter)(attribute *att, void *argument), void *argument);

/**
 * Return the value of the last attribute whose name matches the given one.
 *
 * @param file the file to look into
 * @param the attribute nameto match
 *
 * @return the attribute or NULL
 */
char *ini_get(ini *self, const char name[]);

/**
 * Return the default configuration file for the given conf name.
 * It will check (in order) for:
 * ./NAME
 * ./.NAME
 * $HOME/NAME (not on WIN32)
 * $HOME/.NAME (not on WIN32)
 * /etc/NAME (not on WIN32)
 * /etc/.NAME (not on WIN32)
 *
 * @brief ini_open_default_conf open the default configuration file for the
 *        given name
 * @param name the configuration file name (example: prog.ini)
 * @return the opened FILE or NULL
 */
char *ini_get_default_conf_file(const char name[]);

#endif

#ifdef __cplusplus
}
#endif
