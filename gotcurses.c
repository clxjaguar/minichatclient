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
#ifndef WIN32
	#include <signal.h>
#endif

#include "display_interfaces.h" // prototypes of theses display_* fonctions
#include "commons.h"            // for nicklist struct and timming value
#include "strfunctions.h"       // for eventuals transliterations
#include "conf.h"               // for nicklist width and debug height
#include "nicklist.h"           // for nicklist_showlist()
#include "main.h"

typedef enum {
	NATIVE_UTF8 = 0,
	ISO8859_1,
	CP850
} ttranslit;
ttranslit transliterating;

const char* transliterate_from_utf8(const char* in){
	static char *tmp = NULL;
	const char *p = NULL;
	unsigned int c; // not "char" !
	unsigned int i;

	if (tmp != NULL) { free(tmp); tmp = NULL; }
	if (in == NULL) { return NULL; }

	switch (transliterating) {
		case NATIVE_UTF8:
			return in;
			break;

		case ISO8859_1:
		default:
			tmp = malloc(strlen(in)+1); // la source utf8 sera toujours plus longue!
			p = (const char*)in;

			i = 0;
			while((c = extract_codepoints_from_utf8(&p))){
				tmp[i++] = (char)transliterate_ucs_to_iso88591(c);
			}
			tmp[i] = '\0';
			return (const char*)tmp; // please do not try to free() the returned pointer.
			break;

		case CP850:
			tmp = malloc(strlen(in)+1); // la source utf8 sera toujours plus longue!
			p = (const char*)in;

			i = 0;
			while((c = extract_codepoints_from_utf8(&p))){
				tmp[i++] = (char)transliterate_ucs_to_cp850(c);
			}
			tmp[i] = '\0';
			return (const char*)tmp; // please do not try to free() the returned pointer.
			break;
	}
	return in;
}

#define maxrows LINES
#define maxcols COLS
int ncurses_initialyzed=0;

void destroy_win(WINDOW *lwin){
	wborder(lwin, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
	delwin(lwin);
}

typedef struct {
	WINDOW *decoration;
	WINDOW *content;
} dwin;
dwin typing_area, conversation, debug, nicklist;

void create_dwin(dwin *w, int rows, int cols, int startrow, int startcol, const char *title){
	w->decoration = newwin(               rows,   cols,   startrow,   startcol);
	w->content    = subwin(w->decoration, rows-2, cols-4, startrow+1, startcol+2);
	box(w->decoration, 0, 0); // 0, 0 gives default characters
	if (title) {
		mvwprintw(w->decoration, 0, (cols-(int)strlen(title)-4), " %s ", title);
	}
	wrefresh(w->decoration);
}

void destrow_dwin(dwin *w){
	if (w->content)    { destroy_win(w->content); }
	if (w->decoration) { destroy_win(w->decoration); }
}

// interfaces (display_)

int debug_height   = 7;
int nicklist_width = 15;
int resize_needed  = 0;

void display_statusbar(const char *text){
	if (text && text[0]) {
		attron(A_REVERSE);
		mvwprintw(stdscr, maxrows-1, 0, " %s ", transliterate_from_utf8(text));
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
	display_statusbar(transliterate_from_utf8(msg));
	rep = (char)getch();
	display_statusbar(NULL);
	return rep;
}

void display_debug(const char *text, int nonewline){
	if (!ncurses_initialyzed) {
		if (!nonewline) { printf("\n"); }
		printf("%s", transliterate_from_utf8(text));
		return;
	}

	if (debug_height) {
		if (!nonewline) { wprintw(debug.content, "\n"); }
		wprintw(debug.content, "%s", transliterate_from_utf8(text));
		wrefresh(debug.content);
	}
	else {
		if (!nonewline) {
			wmove(stdscr, maxrows-1, 0);
		}
		attron(A_REVERSE);
		wprintw(stdscr, "%s", transliterate_from_utf8(text));
		clrtoeol();
		attroff(A_REVERSE);
		wrefresh(stdscr);
	}
	//TODO: move physical cursor to typing_area.content more cleanly ?
	wprintw(typing_area.content, " \b");
	wrefresh(typing_area.content);
}

void display_conversation(const char *text){
	if (!ncurses_initialyzed) { return; }
	wprintw(conversation.content, "\n%s", transliterate_from_utf8(text));
	wrefresh(conversation.content);
}

void display_nicklist(const char *text){
	static signed int i=0;
	char *p = NULL;
	if (!nicklist_width) return;
	if (!ncurses_initialyzed) { return; }

	if (!text) { //reset !
		// il ne faut pas utiliser wclear(nicklist.content) car ça redraw le terminal entier
		p = malloc((size_t)(nicklist_width-4+1));
		memset(p, ' ', (size_t)(nicklist_width-4));
		p[nicklist_width-4]='\0';
		//create_dwin(&nicklist, maxrows-debug_height-4-1, nicklist_width, debug_height, maxcols-nicklist_width, "nicklist");
		//void create_dwin(dwin *w, int rows, int cols, int startrow, int startcol, const char *title){
		//  w->content    = subwin(w->decoration, rows-2, cols-4, startrow+1, startcol+2);
		for(i=0; i<maxrows-debug_height-4-1-2; i++){
			mvwprintw(nicklist.content, (int)i, 0, "%s", p);
		}
		free(p); p = NULL; i=0;
	}
	else {
		mvwprintw(nicklist.content, (int)i++, 0, "%s", transliterate_from_utf8(text));
	}

	wrefresh(nicklist.content);
}

static void disp_resized(int sig){
	switch(sig){
		case SIGWINCH:
			resize_needed++;
			break;
	}
}

void display_init(void){
	char *p = NULL;
#ifdef _X_OPEN_SOURCE_EXTENDED
	display_debug("Initialyzing curses (widechar support)...", 0);
	p = setlocale(LC_ALL, "");
	transliterating = ISO8859_1;
	if (p){
		if (strstr(p, "UTF-8") || strstr(p, "UTF8") || strstr(p, "utf-8") || strstr(p, "utf8")){
			transliterating = NATIVE_UTF8;
		}
	}
#else
	transliterating = ISO8859_1;
#ifdef WIN32
	display_debug("Initialyzing curses (windows mode)...", 0);
	transliterating = CP850;
#else
	display_debug("Initialyzing curses (legacy)...", 0);
#endif
#endif
	debug_height   = read_conf_int("debug_height",   debug_height);
	nicklist_width = read_conf_int("nicklist_width", nicklist_width);

	display_debug("", 0);
	initscr(); // start curses mode, LINES and ROWS not valids before

	if (debug_height < 3) { debug_height = 0; } // pour niki :P
	if (debug_height > maxrows-8) { debug_height = maxrows-8; }

	if (nicklist_width < 5) { nicklist_width = 0; }
	if (nicklist_width > maxcols-5) { nicklist_width = maxcols-5; }

	cbreak();  // line input buffering disabled ("raw" mode)
	//nocbreak(); // ("cooked" mode)
	keypad(stdscr, TRUE); // I need that nifty F1 ?
	//intrflush(stdscr, FALSE);
	noecho();  // curses call set to no echoing
	refresh(); // m'a pas mal fait chier quand il était pas là, celui là.
	//getmaxyx(stdscr, maxrows, maxcols); // macro returning terminal's size
	create_dwin(&conversation, maxrows-debug_height-4-1, maxcols-nicklist_width, debug_height, 0, "chat log");
	scrollok(conversation.content, TRUE);
	if (nicklist_width){
		create_dwin(&nicklist, maxrows-debug_height-4-1, nicklist_width, debug_height, maxcols-nicklist_width, "nicklist");
	}
	create_dwin(&typing_area, 4, maxcols, maxrows-5, 0, "typing area");
	if (debug_height) {
		create_dwin(&debug, debug_height, maxcols, 0, 0, "minichatclient internals");
		scrollok(debug.content, TRUE);
	}
	meta(typing_area.content, TRUE);
	ncurses_initialyzed = 1;
	wtimeout(typing_area.content, WAITING_TIME_GRANOLOSITY);
	if (p){
		display_debug("Curses interface initialyzed with locale: ", 0);
		display_debug(p, 1);
	}
	display_debug("Recognized mode: ", 0);
	switch (transliterating) {
		case NATIVE_UTF8:
			display_debug("Native UTF-8 (best)", 1);
			break;

		case ISO8859_1:
			display_debug("ISO-8859-1 transliteration", 1);
			break;

		case CP850:
			display_debug("CP850 (Windows console)", 1);
			break;

		default:
			display_debug("???", 1);
	}
#ifndef WIN32
	signal(SIGWINCH, disp_resized);
	resize_needed=0;
#endif
}

void display_end(void){
	destrow_dwin(&typing_area);
	if (nicklist_width) { destrow_dwin(&nicklist); }
	if (debug_height)   { destrow_dwin(&debug); }
	destrow_dwin(&conversation);
	clear();
	refresh();
	endwin(); // end curses mode
	ncurses_initialyzed = 0;
}

void display_softrefresh(unsigned int nbrofbytes, char *buf){
	clear();
	touchwin(stdscr);
	refresh();

	wclear(typing_area.content);
	if (buf){
		buf[nbrofbytes] = '\0';
		wprintw(typing_area.content, "%s", transliterate_from_utf8(buf));
	}

	touchwin(typing_area.decoration);
	if (debug_height) touchwin(debug.decoration);
	touchwin(conversation.decoration);
	if (nicklist_width) touchwin(nicklist.decoration);

	wrefresh(typing_area.decoration);
	wrefresh(conversation.decoration);
	if (debug_height)   wrefresh(debug.decoration);
	if (nicklist_width) wrefresh(nicklist.decoration);
}

const char* display_driver(void){
	int ch;
	unsigned int j = 0;
	static unsigned int nbrofbytes=0;
	static char *buf = NULL;
	static int dbgchrs[4];

	if (resize_needed){
		display_end();
		resizeterm(LINES, COLS);
		display_init();
		wprintw(conversation.content, "*** window resized ***");
		force_polling();
		display_softrefresh(nbrofbytes, buf);
		display_statusbar("Terminal resized, refreshing...");
		resize_needed=0;
	}

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

						if (!nbrofbytes) {
							free(buf); buf = NULL;
							//display_statusbar("Typing buffer freed");
						}
						wprintw(typing_area.content, "\b \b");
					}
				}
				else { nbrofbytes = 0; }
				break;

			case KEY_RESIZE:
				break;

			case 0x0c: // ^L. wanna some screen refresh?
				display_softrefresh(nbrofbytes, buf);
				break;

			case 18: // ^R
				force_polling();
				break;

			case 14: // ^N
				nicklist_showlist();
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
				if (buf == NULL){ // allocating typing buffer
					//display_statusbar("You're typing...");
					buf = malloc(1002);
				}
				else {
					if (nbrofbytes>=1000){ // typing buffer full ! too long.
						if (j==1) { display_statusbar("\aYour message line can't be longer !"); }
						break;
					}
				}

				if (transliterating == NATIVE_UTF8) {
					buf[nbrofbytes++] = (char)ch;
				}
				else {
					int i = 0;
					unsigned char *p;
					if (transliterating == CP850) {
						p = utf8_character_from_ucs_codepoint(transliterate_cp850_to_ucs((unsigned char)ch));
					}
					else if (transliterating == ISO8859_1) {
						p = utf8_character_from_ucs_codepoint(transliterate_iso88591_to_ucs((unsigned char)ch));
					}
					else {
						break; // bug ?
					}
					while(p[i]){ buf[nbrofbytes++] = (char)p[i++]; }
				}

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
