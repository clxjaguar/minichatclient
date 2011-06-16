/*
  Name:        cstring_p.h
  Copyright:   niki (cc-by-nc) 2011
  Author:      niki
  Date:        2011-06-16
  Description: The 'private' header for cstring.c, which you don't need to use cstring.c
*/


#ifndef CSTRING_P_H
#define CSTRING_P_H

#define BUFFER_SIZE 81

struct cstring_private_struct {
	size_t buffer_length;
};

#endif
