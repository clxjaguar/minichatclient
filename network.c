/*
  Name:         network.c
  Copyright:    GPL
  Author:       cLx
  Date:         08/04/11 14:55
  Description:  ce qu'il faut pour se connecter vers des trucs en TCP.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "display_interfaces.h"
#include "network.h"

#if defined (WIN32)
	#include <winsock2.h>
	#define close(s) closesocket(s)
#else
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netdb.h>
#endif

void ws_init(void){
#if defined (WIN32)
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2,2), &WSAData);
#endif
}

void ws_cleanup(void){
#if defined (WIN32)
	WSACleanup();
#endif
}

int maketcpconnexion(const char* hostname, unsigned int port){
	struct hostent	 *he;
	struct sockaddr_in  server;
	int sockfd;
	char *p;

	display_debug("Resolving ", 0);
	display_debug(hostname, 1);
	display_debug(" ... ", 1);

	/* resolve host to an IP */
	if ((he = (void *)gethostbyname(hostname)) == NULL) { // deprécié !
		display_debug("Error resolving hostname.", 1);
		return 0;
	}

	if (he->h_addrtype == AF_INET) {
		// ok, it's IPv4, we're good for now.
	}
	else {
		// we're in IPv6 troubles.
		display_debug("huh? not AF_INET?", 1);
	}

	/*
	 * copy the network address part of the structure to the
	 * sockaddr_in structure which is passed to connect()
	 */
	server.sin_family = AF_INET;
#ifdef WIN32
	server.sin_port = htons(port);
#else
	server.sin_port = htons((uint16_t)port);
#endif
	server.sin_addr.s_addr = INADDR_ANY;

	memcpy(&(server.sin_addr.s_addr), he->h_addr, (size_t)he->h_length);

	/* open socket */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		display_debug("Error: unable to open socket.", 0);
		return 0;
	}

	/* connect */
	if (connect(sockfd, (struct sockaddr *)&server, sizeof(server))) {
		p = malloc(200);
		snprintf(p, 200, "Error connecting to %s:%d", he->h_name, port);
		display_debug(p, 0);
		free(p); p=NULL;
		close(sockfd);
		return 0;
	}

	p = malloc(200);
	snprintf(p, 200, "Connected to %s:%d", inet_ntoa(server.sin_addr), port);
	display_debug(p, 0);
	free(p); p=NULL;
	return sockfd;
}

//////////////////////////////////////////////////////////////////////////////

int sendstr(int s, const char* buf){
#ifdef DEBUG
	fprintf(stderr, "%s", buf);
#endif
	send(s, buf, strlen(buf), 0);
	return 0;
}

int sendline(int s, const char* buf){
#ifdef DEBUG
	fprintf(stderr, "%s\n", buf);
#endif
	send(s, buf, strlen(buf), 0);
	send(s, "\r\n", 2, 0);
	return 0;
}

int http_get(int s, const char* req, const char* host, const char* referer, const char* cookies, const char* useragent, const char* mischeaders){
	char buf[200];
	snprintf(buf, 200, "GET http://%s%s%s", host, req[0]=='/'?"":"/", req);
	display_debug(buf, 0);

	sendstr(s, "GET ");
	sendstr(s, req);
	sendline(s, " HTTP/1.1");
	if (host) {
		sendstr(s, "Host: ");
		sendline(s, host);
	}
	if (useragent){
		sendstr(s, "User-Agent: ");
		sendline(s, useragent);
	}
	sendline(s, "Connection: close");
	if (referer) {
		sendstr(s, "Referer: ");
		sendline(s, referer);
	}
	if (cookies && cookies[0] != '\0') {
		sendstr(s, "Cookie: ");
		sendline(s, cookies);
	}
	if (mischeaders && mischeaders[0] != '\0'){ sendline(s, mischeaders); }
	sendline(s, ""); // une ligne vide signifie au serveur HTTP qu'on a fini avec les headers, c'est a lui maintenant
	return 0;
}

int http_post(int s, const char* req, const char* host, const char* datas, const char* referer, const char* cookies, const char* useragent, const char* mischeaders){
	char buf[200];
	snprintf(buf, 200, "POST http://%s%s%s|%s", host, req[0]=='/'?"":"/", req, datas);
	display_debug(buf, 0);

	sendstr(s, "POST ");
	sendstr(s, req);
	sendline(s, " HTTP/1.1");
	if (host) {
		sendstr(s, "Host: ");
		sendline(s, host);
	}
	if (useragent){
		sendstr(s, "User-Agent: ");
		sendline(s, useragent);
	}
	//sendline(s, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
	//sendline(s, "Accept-Language: en,en-us;q=0.5");
	//sendline(s, "Accept-Encoding: gzip, deflate");
	//sendline(s, "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7");
	sendline(s, "Connection: close");
	if (referer) {
		sendstr(s, "Referer: ");
		sendline(s, referer);
	}
	if (cookies) {
		sendstr(s, "Cookie: ");
		sendline(s, cookies);
	}
	sendline(s, "Content-Type: application/x-www-form-urlencoded");
	snprintf(buf, 200, "Content-Length: %lu", (unsigned long)strlen(datas));
	sendline(s, buf);
	if (mischeaders && mischeaders[0] != '\0') { sendline(s, mischeaders); }
	sendline(s, ""); // une ligne vide signifie au serveur qu'on a fini avec les headers, c'est a lui maintenant
	sendline(s, datas);

	return 0;
}
