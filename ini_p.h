/*
  Name:        ini_p.h
  Copyright:   niki (cc-by-nc) 2011
  Author:      niki
  Date:        13/06/11 10:52
  Description: The 'private' header for ini.c, which you don't need to use ini.c
*/


#ifndef INI_P_H
#define INI_P_H

#include <stdio.h>
#include "attribute.h"

/**
 * Create an attribute from a string (it will cut it in half at the sign '=' if found,
 * or just put the whole string in the 'name' field).
 * It will return NULL if the given string describes a comment.
 */
attribute *get_attribute(char data[]);

/**
 * Remove a trailing LN, or CRLN, and replace it by '\0'.
 */
void remove_crlf(char data[], int length);

int match(attribute *att, char mask[]);

#endif
