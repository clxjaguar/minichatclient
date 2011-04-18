/*
  Name:        cookies-test.c
  Copyright:   cLx (cc-by-nc) 2011
  Author:      cLx
  Date:        08/04/11 13:15
  Description: Test débile pour cookies.c
*/

#include <stdio.h>
#include "cookies.h"
#include <string.h>

int main(void){    
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
