/*
  Name:         gotcurses.c
  Copyright:    GPL v3
  Author:       cLx - http://clx.freeshell.org/
  Date:         22/08/11 14:36
  Description:  Curses text interface for the minichatclient project.

  In order to compile under Dev-C++, please install pdcurses-3.2-1mol.DevPak
  For Debian: # apt-get install ncurses-dev
*/

#ifdef _X_OPEN_SOURCE_EXTENDED
	#include <ncursesw/curses.h>
	#include <locale.h>
#else
	#include <curses.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "display_interfaces.h" // prototypes of theses display_* fonctions
#include "commons.h" // for nicklist struct and timming value

//TODO: translation of character set ?

#define maxrows LINES
#define maxcols COLS

char* transliterate(const char *utf){
	char *target;
	unsigned int i;
	//char utf[] = "j'ai pas le droit aux fÃ©culents pour mon rÃ©gime";
	target = malloc(strlen(utf)+1);

	i=0;
	while(utf[i]){
		if(!(utf[i]&128)){
			target[i] = utf[i];
		}
		else {
			target[i] = '?';
		}
		i++;
	}
	target[i] = '\0';
	return target;
}

void destroy_win(WINDOW *lwin){
	wborder(lwin, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
	wrefresh(lwin);
	delwin(lwin);
}

typedef struct {
	WINDOW *decoration;
	WINDOW *content;
} dwin;
dwin typing_area, conversation, debug, nicklist;

void create_dwin(dwin *w, int rows, int cols, int startrow, int startcol, char *title){
	w->decoration = newwin(rows,   cols,   startrow, startcol);
	w->content    = subwin(w->decoration, rows-2, cols-4, startrow+1, startcol+2);
	box(w->decoration, 0, 0); // 0, 0 gives default characters
	if (title) {
		mvwprintw(w->decoration, 0, cols-(unsigned int)strlen(title)-4, " %s ", title);
	}
	wrefresh(w->decoration);
}

void destrow_dwin(dwin *w){
	if (w->content)    { destroy_win(w->content); }
	if (w->decoration) { destroy_win(w->decoration); }
}

// interfaces (display_)

unsigned int DEBUG_HEIGHT   = 8;
unsigned int NICKLIST_WIDTH = 15;

void display_statusbar(const char *text){
	if (text && text[0]) {
		attron(A_REVERSE);
		mvwprintw(stdscr, maxrows-1, 0, " %s ", text);
		attroff(A_REVERSE);
	}
	else {
		wmove(stdscr, maxrows-1, 0);
	}
	clrtoeol();
	wrefresh(stdscr);
}

char display_waitforchar(const char *msg){
	char rep;
	display_statusbar(msg);
	rep = getch();
	display_statusbar(NULL);
	return rep;
}

void display_debug(const char *text, int nonewline){
	if (DEBUG_HEIGHT) {
		if (!nonewline) { wprintw(debug.content, "\n"); }
		wprintw(debug.content, "%s", text);
		wrefresh(debug.content);
	}
	else {
		if (!nonewline) {
			//display_statusbar(NULL);
			wmove(stdscr, maxrows-1, 0);
		}
		attron(A_REVERSE);
		wprintw(stdscr, "%s", text);
		clrtoeol();
		attroff(A_REVERSE);
		wrefresh(stdscr);
	}
	//TODO: move physical cursor to typing_area.content more cleanly ?
	wprintw(typing_area.content, " \b");
	wrefresh(typing_area.content);
	
}

void display_conversation(const char *text){
	char *p;
	//p = transliterate(text);
	//wprintw(conversation.content, "\n%s", p);
	wprintw(conversation.content, "\n%s", text);
	wrefresh(conversation.content);
	//free(p);
	//TODO: mvcur to typing_area.content ?
}

void display_init(void){
#ifdef _X_OPEN_SOURCE_EXTENDED
	char *p = NULL;
	p = setlocale(LC_ALL, "");
#endif
	initscr(); // start curses mode
	cbreak();  // line input buffering disabled ("raw" mode)
	//nocbreak(); // ("cooked" mode)
	keypad(stdscr, TRUE); // I need that nifty F1 ?
	//intrflush(stdscr, FALSE);
	noecho();  // curses call set to no echoing
	refresh(); // m'a pas mal fait chier quand il était pas là, celui là.
	//getmaxyx(stdscr, maxrows, maxcols); // macro returning terminal's size
	create_dwin(&conversation, maxrows-DEBUG_HEIGHT-4-1, maxcols-NICKLIST_WIDTH, DEBUG_HEIGHT, 0, "chat log");
	create_dwin(&nicklist, maxrows-DEBUG_HEIGHT-4-1, NICKLIST_WIDTH, DEBUG_HEIGHT, maxcols-NICKLIST_WIDTH, "nicklist");
	create_dwin(&typing_area, 4, maxcols, maxrows-5, 0, "typing area");
	if (DEBUG_HEIGHT) {
		create_dwin(&debug, DEBUG_HEIGHT, maxcols, 0, 0, "minichatclient internals");
		scrollok(debug.content, TRUE);
		scrollok(conversation.content, TRUE);
	}
	meta(typing_area.content, TRUE);
	wtimeout(typing_area.content, WAITING_TIME_GRANOLOSITY);
#ifdef _X_OPEN_SOURCE_EXTENDED
	display_debug("Curses interface initialyzed with locale: ", 0);
	display_debug(p, 1);
	//wprintw(debug.content, "Curses interface initialyzed with locale: %s", p);
	//wrefresh(debug.content);
#endif
}



void display_nicklist(char *text[], unsigned int nbrofnicks){ // ce truc va changer
	unsigned int i;
	wclear(nicklist.content); // TODO: virer ça car ça redraw le terminal entier

	for (i=0; i<nbrofnicks; i++){
		mvwprintw(nicklist.content, i, 0, "%s", text[i]);
	}
	wrefresh(nicklist.content);
}

void display_end(void){
	if (DEBUG_HEIGHT) destrow_dwin(&debug);
	destrow_dwin(&typing_area);
	destrow_dwin(&conversation);
	destrow_dwin(&nicklist);
	endwin(); // end curses mode
}

char* display_driver(void){
	int ch;
	unsigned int j = 0;
	static unsigned int nbrofbytes=0;
	static char *buf = NULL;
	static int dbgchrs[4];

	if (!nbrofbytes && buf) { free(buf); buf=NULL; display_statusbar("Typing buffer freed after sent"); }

	while ((ch = wgetch(typing_area.content)) != ERR){
		j++;
		switch (ch){
			case 0x0a: // "enter" key
			case 0x0d: // also enter, in some OSes
				if (buf) {
					buf[nbrofbytes] = '\0';
					nbrofbytes = 0;
					// on n'utilise pas wclear() parce que lui en fait
					// il fait redessiner la fenêtre entière!
					wmove(typing_area.content, 0, 0);
					wclrtobot(typing_area.content);
					//wmove(typing_area.content, 0, 0);  clrtoeol();
					wrefresh(typing_area.content);
					return buf;
				}
				break;

			case 0x08: // ascii backspace
			case 0x7f: //  "^?" backspace
				if (buf) {
					if(nbrofbytes){
						if (!(buf[nbrofbytes-1]&128)){ // one byte char. easy.
							nbrofbytes--;
						}
						else { // this byte is a part of UTF-8 char ?
							do {
								nbrofbytes--;
							} while ((nbrofbytes != 0) && ((buf[nbrofbytes]&192) != 192)); // first UTF-8 byte? enough.
						}

						if (!nbrofbytes) { free(buf); buf = NULL; display_statusbar("Typing buffer freed"); }
						wprintw(typing_area.content, "\b \b");
					}
				}
				else { nbrofbytes = 0; }
				break;

			case 0x0c: // ^L. wanna some refresh?
				clear();
				touchwin(stdscr);
				refresh();

				wclear(typing_area.content);
				if (buf){
					buf[nbrofbytes] = '\0';
					wprintw(typing_area.content, "%s", buf);
				}

				touchwin(typing_area.decoration);
				if (DEBUG_HEIGHT) touchwin(debug.decoration);
				touchwin(conversation.decoration);
				touchwin(nicklist.decoration);

				wrefresh(typing_area.decoration);
				wrefresh(conversation.decoration);
				if (DEBUG_HEIGHT) wrefresh(debug.decoration);
				wrefresh(nicklist.decoration);
				break;

			default: // any other character?
				if (ch<0x20) {{
					char tmpchar[120]; char tmpchar2[20]; unsigned int i = 0;
					if (j>4){ break; }
					dbgchrs[j-1] = ch;
					strncpy(tmpchar, "Unknow keys show in decimal notation:", 120);

					for(i=0; i<j; i++){
						snprintf(tmpchar2, 20, " %d", dbgchrs[i]);
						strncat(tmpchar, tmpchar2, 120);
					}
					display_statusbar(tmpchar);
					break;
				}}
				if (buf == NULL){
					buf = malloc(1002);
					display_statusbar("Buffer allocated for typing...");
				}
				else {
					if (nbrofbytes>=1000){
						if (j==1) { fprintf(stderr, "\a"); }
						break;
					}
				}
				buf[nbrofbytes] = (char)ch;
				nbrofbytes++;
				wprintw(typing_area.content, "%c", ch);
				break;
		}
		//mvwprintw(debug.content, 0, 0, "%u: %s  ", nbrofcars, NULL); 
		//wprintw(debug.content, "%d ", ch);
		//wrefresh(debug.content);
		wtimeout(typing_area.content, 10);
	}
	wtimeout(typing_area.content, WAITING_TIME_GRANOLOSITY);

	if (j) {
		wrefresh(typing_area.content); 
	}
	return NULL;
}
