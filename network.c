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
    #ifndef _WIN32_WINNT
        #define _WIN32_WINNT 0x501
    #endif
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#define close(s) closesocket(s)
	#define ioctl(a,b,c) ioctlsocket(a,b,c)
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

		//display_debug("ok.", 1);
		freeaddrinfo(res);
		return sockfd6;
	}
	return 0;
}

#if defined (WIN32)
// inet_ntop for windows systems previous vista or seven...
// stolen from http://memset.wordpress.com/2010/10/09/inet_ntop-for-win32/
const char* inet_ntop(int af, const void* src, char* dst, int cnt){
	struct sockaddr_in srcaddr;
	DWORD rv;
	memset(&srcaddr, 0, sizeof(struct sockaddr_in));
	memcpy(&(srcaddr.sin_addr), src, sizeof(srcaddr.sin_addr));
	srcaddr.sin_family = af;
	if (WSAAddressToString((struct sockaddr*) &srcaddr, sizeof(struct sockaddr_in), 0, dst, (LPDWORD) &cnt) != 0) {
		rv = WSAGetLastError();
		snprintf(dst, cnt, "[ERR %ld]", rv);
		return NULL;
	}
	return dst;
}
#endif

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
	//snprintf(buf, 200, "GET http://%s%s%s", host, req[0]=='/'?"":"/", req);
	snprintf(buf, 200, "GET %s%s", req[0]=='/'?"":"/", req);
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
	//sendline(s, "Connection: Keep-Alive");
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
	//snprintf(buf, 200, "POST http://%s%s%s|%s", host, req[0]=='/'?"":"/", req, datas);
	snprintf(buf, 200, "POST %s%s %s", req[0]=='/'?"":"/", req, datas);
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
	sendline(s, "Connection: close");
	//sendline(s, "Connection: Keep-Alive");
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

