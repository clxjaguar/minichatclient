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
 */
clist *ini_get_select(FILE *file, int (*filter)(attribute *att));

/**
 * Return the value of the last attribute whose name matches the given one.
 */
char *ini_get(FILE *file, char name[]);

#endif
