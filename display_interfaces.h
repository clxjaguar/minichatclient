/* include file for gotcurses.c and possibles others display interfaces
   for the minichatclient project.

   if something has been written from the keyboard, that fonction malloc 
   and return a pointer, else it returns NULL. Please do not free() on 
   what have been returned, as the fonction does it itself on the next call.
   Oh, and it also take care of the 250ms delay before returning. */

char* display_driver(void);

// others interfaces fonctions

void display_init(void);
void display_debug(const char *text, int nonewline);
void display_statusbar(const char*);
void display_conversation(const char *text);
void display_nicklist(char *text);
void display_end(void);
char display_waitforchar(const char *msg);
