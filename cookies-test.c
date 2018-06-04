/*
  Name:        cookies-test.c
  Copyright:   cLx (cc-by-nc) 2011
  Author:      cLx
  Description: Test debile pour cookies.c
  Date:        cLx: 08/04/11 (initial release)
               cLx: 20/08/13 (ajout sys/types.h)
*/

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include "cookies.h"

#ifndef MAXCOOKIES
#define MAXCOOKIES 10
#endif

int main() {
	cookie_t cookies[MAXCOOKIES];
	memset(cookies, 0, sizeof(cookies));

	// on fout bien la merde pour tester! :D
	storecookie(cookies, "moncookie", "plein de sucre");
	storecookie(cookies, "toncookie", "avec des pepites de chocolat");
	storecookie(cookies, "moncookie", "plein de sucre et de la farine");
	storecookie(cookies, "", "cookie sans nom");
	storecookie(cookies, "un cookie vide", "");
	storecookie(cookies, "un autre cookie vide", "");
	storecookie(cookies, "", "cookie toujours sans nom");
	storecookie(cookies, "un cookie vide", "");
	storecookie(cookies, "un autre cookie vide", "mais maintenant plein!");

	// listing them for debugging
	listcookies(cookies);

	// using them !
	printf("\nCookie supreme : %s %s :)\n", getcookie(cookies, "moncookie"), getcookie(cookies, "toncookie"));

	// freeing
	freecookies(cookies);
	return 0;
}
