/*
  Name:         network.c
  Copyright:    GPL
  Author:       cLx
  Date:         08/04/11 14:55
  Description:  ce qu'il faut pour se connecter vers des trucs en réseau.

  05/04/11 : cLx       Début du programme (connexions vers un serveur HTTP), fonctionne sous Debian
  05/04/11 : Nejaa     Portabilisation du code existant vers Windows (testé sous Seven x86)
  06/04/11 : cLx       Test sous XP => OK!
  11/04/11 : cLx       Légeres modifications, entre autre afin que ça fonctionne aussi sous BSD (suffit de ne pas faire de #if defined (linux) en fait :D)
*/

#define DEBUG 0

#if defined (WIN32)
    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <winsock2.h>
    #define close(s) closesocket(s)
#else
    #include <stdio.h>
    #include <arpa/inet.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <string.h>
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

int maketcpconnexion(char* hostname, int port){
	struct hostent	 *he;
	struct sockaddr_in  server;
	int sockfd;

    fprintf(stderr, "\n");
	/* resolve host to an IP */
	if ((he = (void *)gethostbyname(hostname)) == NULL) { // deprécié !
		fprintf(stderr, "Error resolving hostname.\n");
		return 0;
	}

	if (he->h_addrtype == AF_INET) {
		//fprintf(stderr, "%s => %u.%u.%u.%u (IPv4)\n", he->h_name, (unsigned char)he->h_addr[0], (unsigned char)he->h_addr[1], (unsigned char)he->h_addr[2], (unsigned char)he->h_addr[3]);
	}

	/*
	 * copy the network address part of the structure to the
	 * sockaddr_in structure which is passed to connect()
	 */
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = INADDR_ANY;

	memcpy(&(server.sin_addr.s_addr), he->h_addr, he->h_length);

	/* open socket */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "Error: unable to open socket\n");
		return 0;
	}

	/* connect */
	if (connect(sockfd, (struct sockaddr *)&server, sizeof(server))) {
		fprintf(stderr, "Error connecting to %s:%d\n", he->h_name, port);
		close(sockfd);
		return 0;
	}

	fprintf(stderr, "Connected to %s:%d\n", inet_ntoa(server.sin_addr), port);
	return sockfd;
}

//////////////////////////////////////////////////////////////////////////////

int sendstr(int s, char* buf){
	if (DEBUG) fprintf(stderr, "%s", buf);
	send(s, buf, strlen(buf), 0);
	return 0;
}

int sendline(int s, char* buf){
	if (DEBUG) fprintf(stderr, "%s\n", buf);
	send(s, buf, strlen(buf), 0);
	send(s, "\r\n", 2, 0);
	return 0;
}

int http_get(int s, char* req, char* host, char* referer, char* cookies, char* useragent, char* mischeaders){
    fprintf(stderr, "GET http://%s%s%s\n", host, req[0]=='/'?"":"/", req);
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
	//sendline(s, "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");
	//sendline(s, "Accept-Language: en,en-us;q=0.5");
	//sendline(s, "Accept-Encoding: gzip, deflate");
	//sendline(s, "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7");
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

int http_post(int s, char* req, char* host, char* datas, char* referer, char* cookies, char* useragent, char* mischeaders){
	char buf[200];

    fprintf(stderr, "POST http://%s%s%s [%s]\n", host, req[0]=='/'?"":"/", req, datas);
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
	sprintf(buf, "Content-Length: %d", strlen(datas));
	sendline(s, buf);
	if (mischeaders && mischeaders[0] != '\0') { sendline(s, mischeaders); }
	sendline(s, ""); // une ligne vide signifie au serveur qu'on a fini avec les headers, c'est a lui maintenant
	sendline(s, datas);

	return 0;
}
