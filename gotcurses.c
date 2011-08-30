/*
  Name:         gotcurses.c
  Copyright:    GPL v3        
  Author:       cLx - http://clx.freeshell.org/
  Date:         22/08/11 14:36
  Description:  Curses text interface for the minichatclient project.
		
  In order to compile under Dev-C++, please install pdcurses-3.2-1mol.DevPak
  For Debian: # apt-get install ncurses-dev
*/

#include <stdlib.h>
#include <string.h>
#include <curses.h>

#include "display_interfaces.h" // prototypes of theses display_* fonctions
#include "commons.h" // for nicklist struct and timming value

//TODO: translation of character set ?

int maxrows = 0, maxcols = 0;
WINDOW *create_newwin(int height, int width, int starty, int startx){
	WINDOW *lwin;
	lwin = newwin(height, width, starty, startx);
	wrefresh(lwin); 
	return lwin;
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
	w->decoration = create_newwin(rows,   cols,   startrow, startcol);
	w->content    = create_newwin(rows-2, cols-4, startrow+1, startcol+2);
	box(w->decoration, 0, 0); // 0, 0 gives default characters
	if (title) {
		mvwprintw(w->decoration, 0, cols-strlen(title)-4, " %s ", title);
	}
	wrefresh(w->decoration);
}

void destrow_dwin(dwin *w){
	if (w->content)    { destroy_win(w->content); }
	if (w->decoration) { destroy_win(w->decoration); }
}

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


// interfaces

#define NICKLIST_WIDTH 19
#define DEBUG_HEIGHT   15

void display_init(void){
	initscr(); // start curses mode
	cbreak();  // line input buffering disabled ("raw" mode)
	//nocbreak(); // ("cooked" mode)
	keypad(stdscr, TRUE); // I need that nifty F1 ?
	//intrflush(stdscr, FALSE);
	noecho();  // curses call set to no echoing
	refresh(); // m'a pas mal fait chier quand il était pas là, celui là.
	getmaxyx(stdscr, maxrows, maxcols); // macro returning terminal's size
	
	create_dwin(&debug, DEBUG_HEIGHT, maxcols, 0, 0, "minichatclient internals");
	create_dwin(&conversation, maxrows-DEBUG_HEIGHT-4-1, maxcols-NICKLIST_WIDTH, DEBUG_HEIGHT, 0, "chat log");
	create_dwin(&nicklist, maxrows-DEBUG_HEIGHT-4-1, NICKLIST_WIDTH, DEBUG_HEIGHT, maxcols-NICKLIST_WIDTH, "nicklist");
	create_dwin(&typing_area, 4, maxcols, maxrows-5, 0, "typing area");
	scrollok(debug.content, TRUE);
	scrollok(conversation.content, TRUE);
	meta(typing_area.content, TRUE);
	wtimeout(typing_area.content, WAITING_TIME_GRANOLOSITY);
}

char display_waitforchar(const char *msg){
	char rep;
	display_statusbar(msg);
	rep = getch();
	display_statusbar(NULL);
	return rep;
}

void display_debug(const char *text, int nonewline){
	if (!nonewline) { wprintw(debug.content, "\n"); }
	wprintw(debug.content, "%s", text);
	wrefresh(debug.content); 
}

void display_conversation(const char *text){
	wprintw(conversation.content, "\n%s", text);
	wrefresh(conversation.content);
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
	destrow_dwin(&debug);
	destrow_dwin(&typing_area);
	destrow_dwin(&conversation);
	destrow_dwin(&nicklist);
	endwin(); // end curses mode
}

char* display_driver(void){
	int ch;
	unsigned int j = 0;
	static unsigned int nbrofcars=0; //, nbrofbytes=0, currentcar = 0;
	static char *buf = NULL;
	
	if (!nbrofcars && buf) { free(buf); buf=NULL; }
	
	while ((ch = wgetch(typing_area.content)) != ERR){
		j++;
		switch (ch){
			case 0x0a: // "enter" key
			case 0x0d: // also enter, in some OSes
				if (buf) {
					buf[nbrofcars] = '\0';
					nbrofcars = 0;
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
					if(nbrofcars){
						if (!--nbrofcars) { free(buf); buf = NULL; }
						wprintw(typing_area.content, "\b \b");
					}
				}
				else { nbrofcars = 0; }
				break;
			
			case 0x0c: // ^L. wanna some refresh?
				wclear(typing_area.content);
				if (buf){
					buf[nbrofcars] = '\0';
					wprintw(typing_area.content, "%s", buf);
				}	
				wrefresh(typing_area.content);
				refresh();
				break;
				
			default: // any other character?
				if (ch<0x20) { break; }
				if (buf == NULL){
					buf = malloc(1002);
					//display_statusbar("Buffer created...");
				}
				else {
					if (nbrofcars>=1000){
						if (j==1) { fprintf(stderr, "\a"); }
						break;
					}
				}
				buf[nbrofcars] = ch;
				nbrofcars++;
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
