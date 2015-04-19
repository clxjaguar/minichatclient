#include "strfunctions.h"

/*
  Name:        strrep.c
  Copyright:   GPLv3
  Author:      cLx - http://clx.freehell.org/
  Date:        29/03/13 23:47
  Description: doing some search&replace into strings without recursion.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned int strrep(const char *input, char **buffer, const char *search, const char *replace){
	const char *i1, *i2;
	char *o1, *tofree=NULL;
	unsigned int n;
	size_t len_search, len_replace, len_target, len;

	if (!buffer) { return 0; } // passed NULL. absolutly invalid.

	if (input) {
		if (*buffer){ free(*buffer); *buffer = NULL; }
	}
	else {
		input = *buffer;
		tofree = *buffer;
		*buffer = NULL;
	}

	len_search = strlen(search);
	len_replace = strlen(replace);

	if (!len_search) { // passed empty search string. invalid.
		if (tofree) { // input was by the buffer
			*buffer = tofree;
		}
		else { // input was by the input string
			*buffer = malloc(strlen(input)+1);
			strcpy(*buffer, input);
		}
		return 0;
	}

	// count the number of patten into the input string.
	n=0;
	i2=input;
	do {
		i2=strstr(i2, search);
		if (i2){ i2+=len_search; n++; }
	} while (i2);

	if (!n) { // no occurence found.
		if (tofree) { // input was by the buffer
			*buffer = tofree;
		}
		else { // input was by the input string
			*buffer = malloc(strlen(input)+1);
			strcpy(*buffer, input);
		}
		return 0;
	}

	len_target = strlen(input) - n*len_search + n*len_replace;
	*buffer = malloc(len_target+1);

	i2=input; i1=input; o1=*buffer;
	do {
		i2=strstr(i2, search); // searching pattern
		if (i2){ // match found
			len = (size_t)(i2 - i1); // length of the text before the pattern
			memcpy(o1, i1, len); // copy before the found pattern
			o1+=len; // advance the output pointer
			memcpy(o1, replace, len_replace); // copy replacement pattern
			i1=i2+=len_search; // advance inputs pointers
			o1+=len_replace; // advance output pointer
		}
		else { // no more occurrences. now need to copy the remaining of the string.
			strcpy(o1, i1);
		}
	} while (i2);
	if (tofree) { free(tofree); }
	return n;
}

/* tests
int main(void){
	char *p=NULL;
	unsigned int n;

	n = strrep("peux-tu-mettre-le-beurre-au-frigo ?", &p, "beurre", "lait");
	n = strrep("peux-tu-mettre-le-beurre-au-frigo ?", NULL, "beurre", "lait");
	n = strrep("peux-tu-mettre-le-beurre-au-frigo ?", &p, "beurre", "lait");
	n = strrep("peux-tu-mettre-le-beurre-au-frigo ?", &p, "beurre", "lait");
	printf("(%d) %s\n", n, p);

	n = strrep(NULL, &p, "", " ");
	n = strrep(NULL, &p, "skjhf", "");
	n = strrep(NULL, &p, "-", " ");
	printf("(%d) %s\n", n, p);

	n = strrep(NULL, &p, "au frigo", "dans le frigo");
	printf("(%d) %s\n", n, p);

	free(p);
	//scanf("%d", &n);
	return 0;
}
*/

/*
  Name:         (all of theses following functions)
  Copyright:    GPL (but you don't want to reuse that)
  Author:       cLx (http://clx.freeshell.org/)
  Date:         09/11/11
  Description:  String transliteration founctions for dos terminal / latin-1 terminals
*/


#define B10000000 128
#define B11000000 192
#define B11100000 224
#define B11110000 240
#define B11111000 248
#define B11111100 252
#define B11111110 254
#define B11111111 255
#define B01111111 127
#define B00111111  63
#define B00011111  31
#define B00001111  15
#define B00000111   7
#define B00000011   3
#define B00000001   1
#define B00000000   0

unsigned int extract_codepoints_from_utf8(const char **in){
	unsigned int value = 0;

	if (!in)          { return 0; } 	         // got a null pointer ?
	else if (!(*in))  { (*in)=NULL; return 0; }  // point to null string ?
	else if (!(**in)) { (*in)=NULL; return 0; }  // end of string reached ?

	// 1 byte (ascii) character ?
	else if (((**in)&B10000000) == B00000000) {
		value = (unsigned int)**in;
		(*in)+=1;
	}

	// 2 bytes character ?
	else if (((**in)&B11100000) == B11000000) {
		if (!*(*in+1)) { (*in)=NULL; return 0; }
		value = ((((unsigned int)*(*in+0))&B00011111)<< 6)
		      + ((((unsigned int)*(*in+1))&B00111111)    );
		(*in)+=2;
	}

	// 3 bytes character ?
	else if (((**in)&B11110000) == B11100000) {
		if (!*(*in+1)) { (*in)=NULL; return 0; }
		if (!*(*in+2)) { (*in)=NULL; return 0; }
		value = ((((unsigned int)*(*in+0))&B00001111)<<12)
		      + ((((unsigned int)*(*in+1))&B00111111)<< 6)
			  + ((((unsigned int)*(*in+2))&B00111111)    );
		(*in)+=3;
	}

	// 4 bytes character ?
	else if (((**in)&B11111000) == B11110000) {
		if (!*(*in+1)) { (*in)=NULL; return 0; }
		if (!*(*in+2)) { (*in)=NULL; return 0; }
		if (!*(*in+3)) { (*in)=NULL; return 0; }
		value = ((((unsigned int)*(*in+0))&B00000111)<<18)
		      + ((((unsigned int)*(*in+1))&B00111111)<<12)
			  + ((((unsigned int)*(*in+2))&B00111111)<< 6)
			  + ((((unsigned int)*(*in+3))&B00111111)    );
		(*in)+=4;
	}

	// 5 bytes character ?
	else if (((**in)&B11111100) == B11111000) {
		if (!*(*in+1)) { (*in)=NULL; return 0; }
		if (!*(*in+2)) { (*in)=NULL; return 0; }
		if (!*(*in+3)) { (*in)=NULL; return 0; }
		if (!*(*in+4)) { (*in)=NULL; return 0; }
		value = ((((unsigned int)*(*in+0))&B00000011)<<24)
		      + ((((unsigned int)*(*in+1))&B00111111)<<18)
		      + ((((unsigned int)*(*in+2))&B00111111)<<12)
			  + ((((unsigned int)*(*in+3))&B00111111)<< 6)
			  + ((((unsigned int)*(*in+4))&B00111111)    );
		(*in)+=5;
	}

	// 6 bytes character ?
	else if (((**in)&B11111110) == B11111100) {
		if (!*(*in+1)) { (*in)=NULL; return 0; }
		if (!*(*in+2)) { (*in)=NULL; return 0; }
		if (!*(*in+3)) { (*in)=NULL; return 0; }
		if (!*(*in+4)) { (*in)=NULL; return 0; }
		if (!*(*in+5)) { (*in)=NULL; return 0; }
		value = ((((unsigned int)*(*in+0))&B00000001)<<30)
		      + ((((unsigned int)*(*in+1))&B00111111)<<24)
		      + ((((unsigned int)*(*in+2))&B00111111)<<18)
		      + ((((unsigned int)*(*in+3))&B00111111)<<12)
			  + ((((unsigned int)*(*in+4))&B00111111)<< 6)
			  + ((((unsigned int)*(*in+5))&B00111111)    );
		(*in)+=6;
	}

	else { value = '?'; (*in)+=1; }

	return value;
}

unsigned char* utf8_character_from_ucs_codepoint(unsigned int ucs_character){
	static unsigned char buf[7];

	if (ucs_character == 0) {
		buf[0] = '\0';
	}
	else if (ucs_character <= 0x0000007F) { // 1 byte
		buf[0] = (unsigned char)((ucs_character     & B01111111)|B00000000);
		buf[1] = '\0';
	}
	else if (ucs_character <= 0x000007FF) { // 2
		buf[0] = (unsigned char)((ucs_character>> 6 & B00011111)|B11000000);
		buf[1] = (unsigned char)((ucs_character     & B00111111)|B10000000);
		buf[2] = '\0';
	}
	else if (ucs_character <= 0x0000FFFF) {
		buf[0] = (unsigned char)((ucs_character>>12 & B00001111)|B11100000);
		buf[1] = (unsigned char)((ucs_character>> 6 & B00111111)|B10000000);
		buf[2] = (unsigned char)((ucs_character     & B00111111)|B10000000);
		buf[3] = '\0';
	}
	else if (ucs_character <= 0x001FFFFF) {
		buf[0] = (unsigned char)((ucs_character>>18 & B00000111)|B11110000);
		buf[1] = (unsigned char)((ucs_character>>12 & B00111111)|B10000000);
		buf[2] = (unsigned char)((ucs_character>> 6 & B00111111)|B10000000);
		buf[3] = (unsigned char)((ucs_character     & B00111111)|B10000000);
		buf[4] = '\0';
	}
	else if (ucs_character <= 0x03FFFFFF) {
		buf[0] = (unsigned char)((ucs_character>>24 & B00000011)|B11111000);
		buf[1] = (unsigned char)((ucs_character>>18 & B00111111)|B10000000);
		buf[2] = (unsigned char)((ucs_character>>12 & B00111111)|B10000000);
		buf[3] = (unsigned char)((ucs_character>> 6 & B00111111)|B10000000);
		buf[4] = (unsigned char)((ucs_character     & B00111111)|B10000000);
		buf[5] = '\0';
	}
	else if (ucs_character <= 0x7FFFFFFF) {
		buf[0] = (unsigned char)((ucs_character>>30 & B00000001)|B11111100);
		buf[1] = (unsigned char)((ucs_character>>24 & B00111111)|B10000000);
		buf[2] = (unsigned char)((ucs_character>>18 & B00111111)|B10000000);
		buf[3] = (unsigned char)((ucs_character>>12 & B00111111)|B10000000);
		buf[4] = (unsigned char)((ucs_character>> 6 & B00111111)|B10000000);
		buf[5] = (unsigned char)((ucs_character     & B00111111)|B10000000);
		buf[6] = '\0';
	}
	else { // no way !
		buf[0] = '\0';
	}
	return buf;
}

unsigned char transliterate_ucs_to_cp850(unsigned int ucs_codepoint){
	if (ucs_codepoint <= 0x007E) {
		return (unsigned char)ucs_codepoint;
	}
#ifdef WIN32
	else if (ucs_codepoint == 0x20AC) { return 'E'; } //euro sign => 'E'
	else if (ucs_codepoint == 0x2018) { return '\''; }//a simple quote
	else if (ucs_codepoint == 0x2019) { return '\''; }//another simple quote
	else if (ucs_codepoint == 0x201A) { return ','; } //low curved quote x.x
	else if (ucs_codepoint == 0x201C) { return '"'; } //a double quote
	else if (ucs_codepoint == 0x201D) { return '"'; } //another double quote
	else if (ucs_codepoint == 0x2013) { return '-'; }
	else if (ucs_codepoint == 0x2014) { return '-'; }
	else if (ucs_codepoint == 0x02DC) { return '~'; }

	// theses codes are from the wikipedia page about cp850
	else if (ucs_codepoint == 0x263A) { return 0x01; }
	else if (ucs_codepoint == 0x263B) { return 0x02; }
	else if (ucs_codepoint == 0x2665) { return 0x03; }
	else if (ucs_codepoint == 0x2666) { return 0x04; }
	else if (ucs_codepoint == 0x2663) { return 0x05; }
	else if (ucs_codepoint == 0x2660) { return 0x06; }
	else if (ucs_codepoint == 0x2022) { return 0x07; }
	//else if (ucs_codepoint == 0x25D8) { return 0x08; }
	//else if (ucs_codepoint == 0x25CB) { return 0x09; }
	//else if (ucs_codepoint == 0x25D9) { return 0x0A; }
	else if (ucs_codepoint == 0x2642) { return 0x0B; }
	else if (ucs_codepoint == 0x2640) { return 0x0C; }
	//else if (ucs_codepoint == 0x266A) { return 0x0D; }
	else if (ucs_codepoint == 0x266B) { return 0x0E; }
	else if (ucs_codepoint == 0x263C) { return 0x0F; }

	else if (ucs_codepoint == 0x25BA) { return 0x10; }
	else if (ucs_codepoint == 0x25C4) { return 0x11; }
	else if (ucs_codepoint == 0x2195) { return 0x12; }
	else if (ucs_codepoint == 0x203C) { return 0x13; }
	else if (ucs_codepoint == 0x00B6) { return 0x14; }
	else if (ucs_codepoint == 0x00A7) { return 0x15; }
	else if (ucs_codepoint == 0x25AC) { return 0x16; }
	else if (ucs_codepoint == 0x25AC) { return 0x17; }
	else if (ucs_codepoint == 0x2191) { return 0x18; }
	else if (ucs_codepoint == 0x2193) { return 0x19; }
	else if (ucs_codepoint == 0x2192) { return 0x1A; }
	else if (ucs_codepoint == 0x2190) { return 0x1B; }
	else if (ucs_codepoint == 0x221F) { return 0x1C; }
	else if (ucs_codepoint == 0x2194) { return 0x1D; }
	else if (ucs_codepoint == 0x25B2) { return 0x1E; }
	else if (ucs_codepoint == 0x25BC) { return 0x1F; }

	else if (ucs_codepoint == 0x2302) { return 0x7F; }

	else if (ucs_codepoint == 0x00C7) { return 0x80; }
	else if (ucs_codepoint == 0x00FC) { return 0x81; }
	else if (ucs_codepoint == 0x00E9) { return 0x82; }
	else if (ucs_codepoint == 0x00E2) { return 0x83; }
	else if (ucs_codepoint == 0x00E4) { return 0x84; }
	else if (ucs_codepoint == 0x00E0) { return 0x85; }
	else if (ucs_codepoint == 0x00E5) { return 0x86; }
	else if (ucs_codepoint == 0x00E7) { return 0x87; }
	else if (ucs_codepoint == 0x00EA) { return 0x88; }
	else if (ucs_codepoint == 0x00EB) { return 0x89; }
	else if (ucs_codepoint == 0x00E8) { return 0x8A; }
	else if (ucs_codepoint == 0x00EF) { return 0x8B; }
	else if (ucs_codepoint == 0x00EE) { return 0x8C; }
	else if (ucs_codepoint == 0x00EC) { return 0x8D; }
	else if (ucs_codepoint == 0x00C4) { return 0x8E; }
	else if (ucs_codepoint == 0x00C5) { return 0x8F; }

	else if (ucs_codepoint == 0x00C9) { return 0x90; }
	else if (ucs_codepoint == 0x00E6) { return 0x91; }
	else if (ucs_codepoint == 0x00C6) { return 0x92; }
	else if (ucs_codepoint == 0x00F4) { return 0x93; }
	else if (ucs_codepoint == 0x00F6) { return 0x94; }
	else if (ucs_codepoint == 0x00F2) { return 0x95; }
	else if (ucs_codepoint == 0x00FB) { return 0x96; }
	else if (ucs_codepoint == 0x00F9) { return 0x97; }
	else if (ucs_codepoint == 0x00FF) { return 0x98; }
	else if (ucs_codepoint == 0x00D6) { return 0x99; }
	else if (ucs_codepoint == 0x00DC) { return 0x9A; }
	else if (ucs_codepoint == 0x00F8) { return 0x9B; }
	else if (ucs_codepoint == 0x00A3) { return 0x9C; }
	else if (ucs_codepoint == 0x00D8) { return 0x9D; }
	else if (ucs_codepoint == 0x00D7) { return 0x9E; }
	else if (ucs_codepoint == 0x0192) { return 0x9F; }

	else if (ucs_codepoint == 0x00E1) { return 0xA0; }
	else if (ucs_codepoint == 0x00ED) { return 0xA1; }
	else if (ucs_codepoint == 0x00F3) { return 0xA2; }
	else if (ucs_codepoint == 0x00FA) { return 0xA3; }
	else if (ucs_codepoint == 0x00F1) { return 0xA4; }
	else if (ucs_codepoint == 0x00D1) { return 0xA5; }
	else if (ucs_codepoint == 0x00AA) { return 0xA6; }
	else if (ucs_codepoint == 0x00BA) { return 0xA7; }
	else if (ucs_codepoint == 0x00BF) { return 0xA8; }
	else if (ucs_codepoint == 0x00AE) { return 0xA9; }
	else if (ucs_codepoint == 0x00AC) { return 0xAA; }
	else if (ucs_codepoint == 0x00BD) { return 0xAB; }
	else if (ucs_codepoint == 0x00BC) { return 0xAC; }
	else if (ucs_codepoint == 0x00A1) { return 0xAD; }
	else if (ucs_codepoint == 0x00AB) { return 0xAE; }
	else if (ucs_codepoint == 0x00BB) { return 0xAF; }

	else if (ucs_codepoint == 0x2591) { return 0xB0; }
	else if (ucs_codepoint == 0x2592) { return 0xB1; }
	else if (ucs_codepoint == 0x2593) { return 0xB2; }
	else if (ucs_codepoint == 0x2502) { return 0xB3; }
	else if (ucs_codepoint == 0x2524) { return 0xB4; }
	else if (ucs_codepoint == 0x00C1) { return 0xB5; }
	else if (ucs_codepoint == 0x00C2) { return 0xB6; }
	else if (ucs_codepoint == 0x00C0) { return 0xB7; }
	else if (ucs_codepoint == 0x00A9) { return 0xB8; }
	else if (ucs_codepoint == 0x2563) { return 0xB9; }
	else if (ucs_codepoint == 0x2563) { return 0xBA; }
	else if (ucs_codepoint == 0x2557) { return 0xBB; }
	else if (ucs_codepoint == 0x255D) { return 0xBC; }
	else if (ucs_codepoint == 0x00A2) { return 0xBD; }
	else if (ucs_codepoint == 0x00A5) { return 0xBE; }
	else if (ucs_codepoint == 0x2510) { return 0xBF; }

	else if (ucs_codepoint == 0x2514) { return 0xC0; }
	else if (ucs_codepoint == 0x2534) { return 0xC1; }
	else if (ucs_codepoint == 0x252C) { return 0xC2; }
	else if (ucs_codepoint == 0x251C) { return 0xC3; }
	else if (ucs_codepoint == 0x2500) { return 0xC4; }
	else if (ucs_codepoint == 0x253C) { return 0xC5; }
	else if (ucs_codepoint == 0x00E3) { return 0xC6; }
	else if (ucs_codepoint == 0x00C3) { return 0xC7; }
	else if (ucs_codepoint == 0x255A) { return 0xC8; }
	else if (ucs_codepoint == 0x2554) { return 0xC9; }
	else if (ucs_codepoint == 0x2569) { return 0xCA; }
	else if (ucs_codepoint == 0x2566) { return 0xCB; }
	else if (ucs_codepoint == 0x2560) { return 0xCC; }
	else if (ucs_codepoint == 0x2550) { return 0xCD; }
	else if (ucs_codepoint == 0x256C) { return 0xCE; }
	else if (ucs_codepoint == 0x00A4) { return 0xCF; }

	else if (ucs_codepoint == 0x00F0) { return 0xD0; }
	else if (ucs_codepoint == 0x00D0) { return 0xD1; }
	else if (ucs_codepoint == 0x00CA) { return 0xD2; }
	else if (ucs_codepoint == 0x00CB) { return 0xD3; }
	else if (ucs_codepoint == 0x00C8) { return 0xD4; }
	else if (ucs_codepoint == 0x0131) { return 0xD5; }
	else if (ucs_codepoint == 0x00CD) { return 0xD6; }
	else if (ucs_codepoint == 0x00CE) { return 0xD7; }
	else if (ucs_codepoint == 0x00CF) { return 0xD8; }
	else if (ucs_codepoint == 0x2518) { return 0xD9; }
	else if (ucs_codepoint == 0x250C) { return 0xDA; }
	else if (ucs_codepoint == 0x2588) { return 0xDB; }
	else if (ucs_codepoint == 0x2584) { return 0xDC; }
	else if (ucs_codepoint == 0x00A6) { return 0xDD; }
	else if (ucs_codepoint == 0x00CC) { return 0xDE; }
	else if (ucs_codepoint == 0x2580) { return 0xDF; }

	else if (ucs_codepoint == 0x00D3) { return 0xE0; }
	else if (ucs_codepoint == 0x00DF) { return 0xE1; }
	else if (ucs_codepoint == 0x00D4) { return 0xE2; }
	else if (ucs_codepoint == 0x00D2) { return 0xE3; }
	else if (ucs_codepoint == 0x00F5) { return 0xE4; }
	else if (ucs_codepoint == 0x00D5) { return 0xE5; }
	else if (ucs_codepoint == 0x00B5) { return 0xE6; }
	else if (ucs_codepoint == 0x00FE) { return 0xE7; }
	else if (ucs_codepoint == 0x00DE) { return 0xE8; }
	else if (ucs_codepoint == 0x00DA) { return 0xE9; }
	else if (ucs_codepoint == 0x00DB) { return 0xEA; }
	else if (ucs_codepoint == 0x00D9) { return 0xEB; }
	else if (ucs_codepoint == 0x00FD) { return 0xEC; }
	else if (ucs_codepoint == 0x00DD) { return 0xED; }
	else if (ucs_codepoint == 0x00AF) { return 0xEE; }
	else if (ucs_codepoint == 0x00B4) { return 0xEF; }

	else if (ucs_codepoint == 0x00AD) { return 0xF0; }
	else if (ucs_codepoint == 0x00B1) { return 0xF1; }
	else if (ucs_codepoint == 0x2017) { return 0xF2; }
	else if (ucs_codepoint == 0x00BE) { return 0xF3; }
	else if (ucs_codepoint == 0x00B6) { return 0xF4; }
	else if (ucs_codepoint == 0x00A7) { return 0xF5; }
	else if (ucs_codepoint == 0x00F7) { return 0xF6; }
	else if (ucs_codepoint == 0x00B8) { return 0xF7; }
	else if (ucs_codepoint == 0x00B0) { return 0xF8; }
	else if (ucs_codepoint == 0x00A8) { return 0xF9; }
	else if (ucs_codepoint == 0x00B7) { return 0xFA; }
	else if (ucs_codepoint == 0x00B9) { return 0xFB; }
	else if (ucs_codepoint == 0x00B3) { return 0xFC; }
	else if (ucs_codepoint == 0x00B2) { return 0xFD; }
	else if (ucs_codepoint == 0x25A0) { return 0xFE; }
	else if (ucs_codepoint == 0x00A0) { return 0xFF; }
#endif
	return '?';
}

unsigned int transliterate_cp850_to_ucs(unsigned char cp850_codepoint){
	if ((cp850_codepoint >= 0x0020) && (cp850_codepoint <= 0x007E)) {
	//if (cp850_codepoint <= 0x007E) {
		return (unsigned int)cp850_codepoint;
	}
#ifdef WIN32
	// all theses codes are from the wikipedia page about cp850
	else if (cp850_codepoint == 0x00) { return 0x0000; }
	else if (cp850_codepoint == 0x01) { return 0x263A; }
	else if (cp850_codepoint == 0x02) { return 0x263B; }
	else if (cp850_codepoint == 0x03) { return 0x2665; }
	else if (cp850_codepoint == 0x04) { return 0x2666; }
	else if (cp850_codepoint == 0x05) { return 0x2663; }
	else if (cp850_codepoint == 0x06) { return 0x2660; }
	else if (cp850_codepoint == 0x07) { return 0x2022; }
	else if (cp850_codepoint == 0x08) { return 0x25D8; }
	else if (cp850_codepoint == 0x09) { return 0x25CB; }
	//else if (cp850_codepoint == 0x0A) { return 0x25D9; }
	else if (cp850_codepoint == 0x0B) { return 0x2642; }
	else if (cp850_codepoint == 0x0C) { return 0x2640; }
	//else if (cp850_codepoint == 0x0D) { return 0x266A; }
	else if (cp850_codepoint == 0x0E) { return 0x266B; }
	else if (cp850_codepoint == 0x0F) { return 0x263C; }

	else if (cp850_codepoint == 0x10) { return 0x25BA; }
	else if (cp850_codepoint == 0x11) { return 0x25C4; }
	else if (cp850_codepoint == 0x12) { return 0x2195; }
	else if (cp850_codepoint == 0x13) { return 0x203C; }
	else if (cp850_codepoint == 0x14) { return 0x00B6; }
	else if (cp850_codepoint == 0x15) { return 0x00A7; }
	else if (cp850_codepoint == 0x16) { return 0x25AC; }
	else if (cp850_codepoint == 0x17) { return 0x25AC; }
	else if (cp850_codepoint == 0x18) { return 0x2191; }
	else if (cp850_codepoint == 0x19) { return 0x2193; }
	else if (cp850_codepoint == 0x1A) { return 0x2192; }
	else if (cp850_codepoint == 0x1B) { return 0x2190; }
	else if (cp850_codepoint == 0x1C) { return 0x221F; }
	else if (cp850_codepoint == 0x1D) { return 0x2194; }
	else if (cp850_codepoint == 0x1E) { return 0x25B2; }
	else if (cp850_codepoint == 0x1F) { return 0x25BC; }

	else if (cp850_codepoint == 0x7F) { return 0x2302; }

	else if (cp850_codepoint == 0x80) { return 0x00C7; }
	else if (cp850_codepoint == 0x81) { return 0x00FC; }
	else if (cp850_codepoint == 0x82) { return 0x00E9; }
	else if (cp850_codepoint == 0x83) { return 0x00E2; }
	else if (cp850_codepoint == 0x84) { return 0x00E4; }
	else if (cp850_codepoint == 0x85) { return 0x00E0; }
	else if (cp850_codepoint == 0x86) { return 0x00E5; }
	else if (cp850_codepoint == 0x87) { return 0x00E7; }
	else if (cp850_codepoint == 0x88) { return 0x00EA; }
	else if (cp850_codepoint == 0x89) { return 0x00EB; }
	else if (cp850_codepoint == 0x8A) { return 0x00E8; }
	else if (cp850_codepoint == 0x8B) { return 0x00EF; }
	else if (cp850_codepoint == 0x8C) { return 0x00EE; }
	else if (cp850_codepoint == 0x8D) { return 0x00EC; }
	else if (cp850_codepoint == 0x8E) { return 0x00C4; }
	else if (cp850_codepoint == 0x8F) { return 0x00C5; }

	else if (cp850_codepoint == 0x90) { return 0x00C9; }
	else if (cp850_codepoint == 0x91) { return 0x00E6; }
	else if (cp850_codepoint == 0x92) { return 0x00C6; }
	else if (cp850_codepoint == 0x93) { return 0x00F4; }
	else if (cp850_codepoint == 0x94) { return 0x00F6; }
	else if (cp850_codepoint == 0x95) { return 0x00F2; }
	else if (cp850_codepoint == 0x96) { return 0x00FB; }
	else if (cp850_codepoint == 0x97) { return 0x00F9; }
	else if (cp850_codepoint == 0x98) { return 0x00FF; }
	else if (cp850_codepoint == 0x99) { return 0x00D6; }
	else if (cp850_codepoint == 0x9A) { return 0x00DC; }
	else if (cp850_codepoint == 0x9B) { return 0x00F8; }
	else if (cp850_codepoint == 0x9C) { return 0x00A3; }
	else if (cp850_codepoint == 0x9D) { return 0x00D8; }
	else if (cp850_codepoint == 0x9E) { return 0x00D7; }
	else if (cp850_codepoint == 0x9F) { return 0x0192; }

	else if (cp850_codepoint == 0xA0) { return 0x00E1; }
	else if (cp850_codepoint == 0xA1) { return 0x00ED; }
	else if (cp850_codepoint == 0xA2) { return 0x00F3; }
	else if (cp850_codepoint == 0xA3) { return 0x00FA; }
	else if (cp850_codepoint == 0xA4) { return 0x00F1; }
	else if (cp850_codepoint == 0xA5) { return 0x00D1; }
	else if (cp850_codepoint == 0xA6) { return 0x00AA; }
	else if (cp850_codepoint == 0xA7) { return 0x00BA; }
	else if (cp850_codepoint == 0xA8) { return 0x00BF; }
	else if (cp850_codepoint == 0xA9) { return 0x00AE; }
	else if (cp850_codepoint == 0xAA) { return 0x00AC; }
	else if (cp850_codepoint == 0xAB) { return 0x00BD; }
	else if (cp850_codepoint == 0xAC) { return 0x00BC; }
	else if (cp850_codepoint == 0xAD) { return 0x00A1; }
	else if (cp850_codepoint == 0xAE) { return 0x00AB; }
	else if (cp850_codepoint == 0xAF) { return 0x00BB; }

	else if (cp850_codepoint == 0xB0) { return 0x2591; }
	else if (cp850_codepoint == 0xB1) { return 0x2592; }
	else if (cp850_codepoint == 0xB2) { return 0x2593; }
	else if (cp850_codepoint == 0xB3) { return 0x2502; }
	else if (cp850_codepoint == 0xB4) { return 0x2524; }
	else if (cp850_codepoint == 0xB5) { return 0x00C1; }
	else if (cp850_codepoint == 0xB6) { return 0x00C2; }
	else if (cp850_codepoint == 0xB7) { return 0x00C0; }
	else if (cp850_codepoint == 0xB8) { return 0x00A9; }
	else if (cp850_codepoint == 0xB9) { return 0x2563; }
	else if (cp850_codepoint == 0xBA) { return 0x2563; }
	else if (cp850_codepoint == 0xBB) { return 0x2557; }
	else if (cp850_codepoint == 0xBC) { return 0x255D; }
	else if (cp850_codepoint == 0xBD) { return 0x00A2; }
	else if (cp850_codepoint == 0xBE) { return 0x00A5; }
	else if (cp850_codepoint == 0xBF) { return 0x2510; }

	else if (cp850_codepoint == 0xC0) { return 0x2514; }
	else if (cp850_codepoint == 0xC1) { return 0x2534; }
	else if (cp850_codepoint == 0xC2) { return 0x252C; }
	else if (cp850_codepoint == 0xC3) { return 0x251C; }
	else if (cp850_codepoint == 0xC4) { return 0x2500; }
	else if (cp850_codepoint == 0xC5) { return 0x253C; }
	else if (cp850_codepoint == 0xC6) { return 0x00E3; }
	else if (cp850_codepoint == 0xC7) { return 0x00C3; }
	else if (cp850_codepoint == 0xC8) { return 0x255A; }
	else if (cp850_codepoint == 0xC9) { return 0x2554; }
	else if (cp850_codepoint == 0xCA) { return 0x2569; }
	else if (cp850_codepoint == 0xCB) { return 0x2566; }
	else if (cp850_codepoint == 0xCC) { return 0x2560; }
	else if (cp850_codepoint == 0xCD) { return 0x2550; }
	else if (cp850_codepoint == 0xCE) { return 0x256C; }
	else if (cp850_codepoint == 0xCF) { return 0x00A4; }

	else if (cp850_codepoint == 0xD0) { return 0x00F0; }
	else if (cp850_codepoint == 0xD1) { return 0x00D0; }
	else if (cp850_codepoint == 0xD2) { return 0x00CA; }
	else if (cp850_codepoint == 0xD3) { return 0x00CB; }
	else if (cp850_codepoint == 0xD4) { return 0x00C8; }
	else if (cp850_codepoint == 0xD5) { return 0x0131; }
	else if (cp850_codepoint == 0xD6) { return 0x00CD; }
	else if (cp850_codepoint == 0xD7) { return 0x00CE; }
	else if (cp850_codepoint == 0xD8) { return 0x00CF; }
	else if (cp850_codepoint == 0xD9) { return 0x2518; }
	else if (cp850_codepoint == 0xDA) { return 0x250C; }
	else if (cp850_codepoint == 0xDB) { return 0x2588; }
	else if (cp850_codepoint == 0xDC) { return 0x2584; }
	else if (cp850_codepoint == 0xDD) { return 0x00A6; }
	else if (cp850_codepoint == 0xDE) { return 0x00CC; }
	else if (cp850_codepoint == 0xDF) { return 0x2580; }

	else if (cp850_codepoint == 0xE0) { return 0x00D3; }
	else if (cp850_codepoint == 0xE1) { return 0x00DF; }
	else if (cp850_codepoint == 0xE2) { return 0x00D4; }
	else if (cp850_codepoint == 0xE3) { return 0x00D2; }
	else if (cp850_codepoint == 0xE4) { return 0x00F5; }
	else if (cp850_codepoint == 0xE5) { return 0x00D5; }
	else if (cp850_codepoint == 0xE6) { return 0x00B5; }
	else if (cp850_codepoint == 0xE7) { return 0x00FE; }
	else if (cp850_codepoint == 0xE8) { return 0x00DE; }
	else if (cp850_codepoint == 0xE9) { return 0x00DA; }
	else if (cp850_codepoint == 0xEA) { return 0x00DB; }
	else if (cp850_codepoint == 0xEB) { return 0x00D9; }
	else if (cp850_codepoint == 0xEC) { return 0x00FD; }
	else if (cp850_codepoint == 0xED) { return 0x00DD; }
	else if (cp850_codepoint == 0xEE) { return 0x00AF; }
	else if (cp850_codepoint == 0xEF) { return 0x00B4; }

	else if (cp850_codepoint == 0xF0) { return 0x00AD; }
	else if (cp850_codepoint == 0xF1) { return 0x00B1; }
	else if (cp850_codepoint == 0xF2) { return 0x2017; }
	else if (cp850_codepoint == 0xF3) { return 0x00BE; }
	else if (cp850_codepoint == 0xF4) { return 0x00B6; }
	else if (cp850_codepoint == 0xF5) { return 0x00A7; }
	else if (cp850_codepoint == 0xF6) { return 0x00F7; }
	else if (cp850_codepoint == 0xF7) { return 0x00B8; }
	else if (cp850_codepoint == 0xF8) { return 0x00B0; }
	else if (cp850_codepoint == 0xF9) { return 0x00A8; }
	else if (cp850_codepoint == 0xFA) { return 0x00B7; }
	else if (cp850_codepoint == 0xFB) { return 0x00B9; }
	else if (cp850_codepoint == 0xFC) { return 0x00B3; }
	else if (cp850_codepoint == 0xFD) { return 0x00B2; }
	else if (cp850_codepoint == 0xFE) { return 0x25A0; }
	else if (cp850_codepoint == 0xFF) { return 0x00A0; }
#endif
	return '?';
}

unsigned char transliterate_ucs_to_iso88591(unsigned int ucs_codepoint){
	if (ucs_codepoint <= 0x007E) {
		return (unsigned char)ucs_codepoint;
	}
	else if (ucs_codepoint == 0x20AC) { return 128; } //euro sign (windows)
	else if (ucs_codepoint == 0x2018) { return '\''; }//a simple quote
	else if (ucs_codepoint == 0x2019) { return '\''; }//another simple quote
	else if (ucs_codepoint == 0x201A) { return ','; } //low curved quote x.x
	else if (ucs_codepoint == 0x201C) { return '"'; } //a double quote
	else if (ucs_codepoint == 0x201D) { return '"'; } //another double quote
	else if (ucs_codepoint == 0x2013) { return '-'; }
	else if (ucs_codepoint == 0x2014) { return '-'; }
	else if (ucs_codepoint == 0x02DC) { return '~'; }
	else if (ucs_codepoint == 0x2502) { return '|'; }

	else if (ucs_codepoint >= 0x00A0 && ucs_codepoint <= 0x00FF) {
		return (unsigned char)ucs_codepoint;
	}
	return '?';
}

unsigned int transliterate_iso88591_to_ucs(unsigned char iso_codepoint){
	if (iso_codepoint <= 127) { // base iso8859-1 from ascii
		return (unsigned int)iso_codepoint;
	}
	else if (iso_codepoint >= 160) { // iso8859-1 from 0xA0 up to 0xFF
		return (unsigned int)iso_codepoint;
	}

	// and some from Windows-1252 ...
	else if (iso_codepoint == 128) { return 0x20AC; }
	else if (iso_codepoint == 130) { return 0x201A; }
	else if (iso_codepoint == 131) { return 0x0192; }
	else if (iso_codepoint == 132) { return 0x201E; }
	else if (iso_codepoint == 133) { return 0x2026; }
	else if (iso_codepoint == 134) { return 0x2020; }
	else if (iso_codepoint == 135) { return 0x2021; }
	else if (iso_codepoint == 136) { return 0x02C6; }
	else if (iso_codepoint == 137) { return 0x2030; }
	else if (iso_codepoint == 138) { return 0x0160; }
	else if (iso_codepoint == 139) { return 0x2039; }
	else if (iso_codepoint == 140) { return 0x0152; }
	else if (iso_codepoint == 142) { return 0x017D; }
	else if (iso_codepoint == 145) { return 0x2018; }
	else if (iso_codepoint == 146) { return 0x2019; }
	else if (iso_codepoint == 147) { return 0x201C; }
	else if (iso_codepoint == 148) { return 0x201D; }
	else if (iso_codepoint == 149) { return 0x2022; }
	else if (iso_codepoint == 150) { return 0x2013; }
	else if (iso_codepoint == 151) { return 0x2014; }
	else if (iso_codepoint == 152) { return 0x02DC; }
	else if (iso_codepoint == 153) { return 0x2122; }
	else if (iso_codepoint == 154) { return 0x0161; }
	else if (iso_codepoint == 155) { return 0x203A; }
	else if (iso_codepoint == 156) { return 0x0153; }
	else if (iso_codepoint == 158) { return 0x017E; }
	else if (iso_codepoint == 159) { return 0x0178; }

	return '?';
}

// misc things

/*
	//NB: this is more fast but not standard. May need to be added here.
	//<man>This function is not part of the C or POSIX.1 standards, and is not customary on Unix systems, but is not a GNU invention
    //either.  Perhaps it comes from MS-DOS.  Nowadays, it is also present on the BSDs.</man>
    char *stpcpy(char *d, const char *s) {
        while((*d++ = *s++));
        return d-1;
    }
	// (<51c6f8bb$0$2577$426a34cc@news.free.fr>)
*/

#if defined (WIN32) 
char *stpcpy(char *d, const char *s) {
	while((*d++ = *s++));
	return d-1;
}
#endif

#include <stdarg.h>
char *mconcat(unsigned int strings_nbr, ...){
	char *dest, *p;
	unsigned int i;
	va_list argp;
	
	// determine the cumulated length of the strings 
	// (starting from one for the 'end of string')
	size_t bytes = 1;
	va_start(argp, strings_nbr);	
	for(i=0; i<strings_nbr; i++){ bytes+=strlen(va_arg(argp, char *)); }
	va_end(argp);
	
	// allocating a memory block of that size (so don't forget to free!)
	dest = malloc(bytes);
	if (!dest){ return NULL; }
	
	// now concatenating all the strings into one and we're done!
	va_start(argp, strings_nbr); p = dest;
	for(i=0; i<strings_nbr; i++){ p = stpcpy(p, va_arg(argp, char *)); }
	va_end(argp);
	return dest;
}

int html_strip_tags(char *txt){
	int intag=0; char *i, *o;
	i=txt; o=txt;
	while (*i){
		if (*i == '<') { intag=1; }
		else if (*i == '>') { intag=0; }
		else if (!intag) {
			*(o++)=*i;
		}
		i++;	
	}
	*o='\0';
	return 0;
}
