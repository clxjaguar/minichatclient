#ifndef INI_H
#define INI_H

#include <stdio.h>
#include "attribute.h"
#include "clist.h"

/**
 * (File: Only one level without [sections], comments starts with #, whole line only.)
 * It will only return the attributes that match the given name (or NULL for all),
 * in a clist of <attribute>s.
 */
clist *ini_get_all(FILE *file, char mask[]);

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
clist *ini_get_select(FILE *file, int (*filter)(attribute *att, void *argument), void *argument);

/**
 * Return the value of the last attribute whose name matches the given one.
 *
 * @param file the file to look into
 * @param the attribute nameto match
 *
 * @return the attribute or NULL
 */
char *ini_get(FILE *file, char name[]);

#endif
