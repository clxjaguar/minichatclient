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

#if !(defined (WIN32))
// linux/bsd/whatever version
char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen);
int maketcpconnexion(const char* hostname, const char *service){
	char buf[200];
	int sockfd6;
	struct addrinfo hints, *res, *r;
	int rval;

	display_debug("Resolving ", 0);
	display_debug(hostname, 1);
	display_debug("... ", 1);

	memset(&hints, 0, sizeof(hints));
	hints.ai_family=PF_UNSPEC;
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_protocol=IPPROTO_TCP;
	if ((rval = getaddrinfo(hostname, service, &hints, &res)) != 0) {
		display_debug("getaddrinfo() failed !", 1);
		return 0;
	}

	for (r=res; r; r = r->ai_next) {
		sockfd6 = socket(r->ai_family, r->ai_socktype, r->ai_protocol);
		if (sockfd6==-1){ continue; }

		display_debug("connecting to ", 1);
		display_debug(get_ip_str((struct sockaddr *)r->ai_addr, buf, sizeof(buf)), 1);
		display_debug(":", 1);
		display_debug(service, 1);
		display_debug("... ", 1);

		if (connect(sockfd6, r->ai_addr, r->ai_addrlen) < 0) {
			display_debug("failed.", 1);
			close(sockfd6);
			continue;
		}

		display_debug("ok.", 1);
		freeaddrinfo(res);
		return sockfd6;
	}
	return 0;
}
// code stolen from <http://owend.corp.he.net/ipv6/>
char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen){
	switch(sa->sa_family) {
		case AF_INET:
			inet_ntop(AF_INET, &(((const struct sockaddr_in *)sa)->sin_addr), s, maxlen);
			break;

		case AF_INET6:
			inet_ntop(AF_INET6, &(((const struct sockaddr_in6 *)sa)->sin6_addr), s, maxlen);
			break;

		default:
			strncpy(s, "Unknown AF", maxlen);
			return s;
	}
	return s;
}
#else 
// Windows version here (old code IPv4 compatible only)
// http://mingw.5.n7.nabble.com/Undefined-reference-to-getaddrinfo-td5694.html
int maketcpconnexion(const char* hostname, const char *service){
	struct hostent *he;
	struct sockaddr_in  server;
	char buf[200];
	int sockfd;

	// gethostbyname is deprecated ! go see <http://owend.corp.he.net/ipv6/Porting/PortMeth.pdf> for documentation.
	if ((he = (void *)gethostbyname(hostname)) == NULL) {
		display_debug("Error resolving hostname.", 1);
		return 0;
	}

	if (he->h_addrtype == AF_INET) {
		// ok, it's IPv4, we're good for now.
	}
	else {
		// we're in IPv6 troubles.
		display_debug("Sorry, only linux version is IPv6 compatible.", 0);
	}

	// copy the network address part of the structure to the
	// sockaddr_in structure which is passed to connect()

	server.sin_family = AF_INET;
	server.sin_port = htons(atoi(service));
	server.sin_addr.s_addr = INADDR_ANY;

	memcpy(&(server.sin_addr.s_addr), he->h_addr, (size_t)he->h_length);

	// open socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		display_debug("Error: unable to open socket.", 0);
		return 0;
	}

	// connect
	if (connect(sockfd, (struct sockaddr *)&server, sizeof(server))) {
		snprintf(buf, sizeof(buf), "Error connecting to %s:%d", he->h_name, atoi(service));
		display_debug(buf, 0);
		close(sockfd);
		return 0;
	}


	snprintf(buf, sizeof(buf), "Connected to %s:%d", inet_ntoa(server.sin_addr), atoi(service));
	display_debug(buf, 0);
	return sockfd;
}
#endif

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

