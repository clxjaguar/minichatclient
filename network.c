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

// apt-get install libssl-dev
#include <openssl/ssl.h>
#include <openssl/err.h>
const SSL_METHOD *method;
SSL_CTX *ctx;
SSL *ssl;

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

int network_init(int use_ssl){
#if defined (WIN32)
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2,2), &WSAData);
#endif

	if (!use_ssl) { return 0; }

	// These function calls initialize openssl for correct work.
	OpenSSL_add_all_algorithms();
	//ERR_load_BIO_strings();
	ERR_load_crypto_strings();
	SSL_load_error_strings();

	if(SSL_library_init() < 0){
		display_debug("Could not initialize the OpenSSL library ! :(", 0);
		return 1;
	}
	// Set SSLv2 client hello, also announce SSLv3 and TLSv1
	method = SSLv23_client_method();

	// Try to create a new SSL context
	if ((ctx = SSL_CTX_new(method)) == NULL) {
		display_debug("Unable to create a new SSL context structure :(", 0);
		return 1;
	}

	// Disabling SSLv2 will leave v3 and TSLv1 for negotiation
	SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2);

	// Create new SSL connection state object
	ssl = SSL_new(ctx);
	return 0;
}

int network_cleanup(void){
#if defined (WIN32)
	WSACleanup();
#endif
	SSL_free(ssl);
	SSL_CTX_free(ctx);
	return 0;
}

// SSL code stolen from http://fm4dd.com/openssl/sslconnect.htm
void ssl_print_certificate(SSL *pssl){
	char buf[1000];
	X509 *cert = NULL;
	X509_NAME *certname = NULL;

	display_debug(SSL_get_cipher(ssl), 0);

	// Get the remote certificate into the X509 structure
	cert = SSL_get_peer_certificate(pssl);
	if (cert == NULL) {
		display_debug("Error: Could not get a certificate!", 0);
		return;
	}

	// Extract various certificate information
	certname = X509_get_subject_name(cert);
	X509_NAME_oneline(certname, buf, 1000);
	display_debug(buf, 1);
	X509_free(cert);
}

char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen);
int maketcpconnexion(const char* hostname, const char *service, int use_ssl){
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

		display_debug("Connecting to ", 1);
		display_debug(get_ip_str((struct sockaddr *)r->ai_addr, buf, sizeof(buf)), 1);
		display_debug(":", 1);
		display_debug(service, 1);
		display_debug("... ", 1);

		if (connect(sockfd6, r->ai_addr, r->ai_addrlen) < 0) {
			display_debug("failed. ", 1);
			close(sockfd6);
			continue;
		}
		freeaddrinfo(res);
		if (!use_ssl) { return sockfd6; }

		// Reset ssl to allow another connection. All settings (method, ciphers, BIOs) are kept.
		// If forgotten, using SSL_read() or SSL_write() on news connections may fail!
		if (!SSL_clear(ssl)) {
			display_debug("Warning: The SSL_clear() operation could not be performed. ", 0);
		}

		// Attach the SSL session to the socket descriptor
		SSL_set_fd(ssl, sockfd6);

		// Try to SSL-connect here, returns 1 for success
		if (SSL_connect(ssl) != 1) {
			display_debug("Error: Could not build a SSL session!", 0);
			return 0;
		}

		// Get the remote certificate & show various certificate information
		ssl_print_certificate(ssl);

		return sockfd6;
	}
	freeaddrinfo(res);
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
			inet_ntop(AF_INET, &(((const struct sockaddr_in *)sa)->sin_addr), s, (socklen_t)maxlen);
			break;

		case AF_INET6:
			inet_ntop(AF_INET6, &(((const struct sockaddr_in6 *)sa)->sin6_addr), s, (socklen_t)maxlen);
			break;

		default:
			strncpy(s, "Unknown AF", maxlen);
			return s;
	}
	return s;
}

//////////////////////////////////////////////////////////////////////////////


ssize_t network_recv(int s, int use_ssl, void *buf, size_t len, int flags){
	ssize_t ret;
	int err;
	if (!use_ssl) {
		return recv(s, buf, len, flags);
	}
	memset(buf, 0, len);
	ret = SSL_read(ssl, buf, (int)len);
	if (ret<=0) {
		err = SSL_get_error(ssl, ret);
		if (err != SSL_ERROR_ZERO_RETURN) {
			display_debug("network_recv() failed!", 0);
		}
	}
	return ret;
}

ssize_t network_send(int s, int use_ssl, const void *buf, size_t len, int flags){
	ssize_t ret;
	int err;
	if(len<=0) { display_debug("network_send() with len=0!", 0); }
	if (!use_ssl) {
		return send(s, buf, len, flags);
	}
	ret = SSL_write(ssl, buf, (int)len);
	if (ret<=0) {
		err = SSL_get_error(ssl, ret);
		if (err == SSL_ERROR_ZERO_RETURN) {
			display_debug("network_send() : connection terminated by the server", 0);
		}
		else {
			display_debug("network_send() failed!", 0);
		}
	}
	return ret;
}


int sendstr(int s, int use_ssl, const char* buf){
#ifdef DEBUG
	fprintf(stderr, "%s", buf);
#endif
	if (strlen(buf)) { network_send(s, use_ssl, buf, strlen(buf), 0); }
	return 0;
}

int sendline(int s, int use_ssl, const char* buf){
#ifdef DEBUG
	fprintf(stderr, "%s\n", buf);
#endif
	if (strlen(buf)) { network_send(s, use_ssl, buf, strlen(buf), 0); }
	network_send(s, use_ssl, "\r\n", 2, 0);
	return 0;
}

int http_get(int s, int use_ssl, const char* req, const char* host, const char* referer, const char* cookies, const char* useragent, const char* mischeaders){
	char buf[200];
	snprintf(buf, 200, "GET %s%s", req[0]=='/'?"":"/", req);
	display_debug(buf, 0);

	sendstr(s, use_ssl, "GET ");
	sendstr(s, use_ssl, req);
	sendline(s, use_ssl, " HTTP/1.1");
	if (host) {
		sendstr(s, use_ssl, "Host: ");
		sendline(s, use_ssl, host);
	}
	if (useragent){
		sendstr(s, use_ssl, "User-Agent: ");
		sendline(s, use_ssl, useragent);
	}
	sendline(s, use_ssl, "Connection: close");

	if (referer) {
		sendstr(s, use_ssl, "Referer: ");
		sendline(s, use_ssl, referer);
	}
	if (cookies && cookies[0] != '\0') {
		sendstr(s, use_ssl, "Cookie: ");
		sendline(s, use_ssl, cookies);
	}
	if (mischeaders && mischeaders[0] != '\0'){ sendline(s, use_ssl, mischeaders); }
	sendline(s, use_ssl, ""); // une ligne vide signifie au serveur HTTP qu'on a fini avec les headers, c'est a lui maintenant
	return 0;
}

int http_post(int s, int use_ssl, const char* req, const char* host, const char* datas, const char* referer, const char* cookies, const char* useragent, const char* mischeaders){
	char buf[200];
	snprintf(buf, 200, "POST %s%s %s", req[0]=='/'?"":"/", req, datas);
	display_debug(buf, 0);

	sendstr(s, use_ssl, "POST ");
	sendstr(s, use_ssl, req);
	sendline(s, use_ssl, " HTTP/1.1");
	if (host) {
		sendstr(s, use_ssl, "Host: ");
		sendline(s, use_ssl, host);
	}
	if (useragent){
		sendstr(s, use_ssl, "User-Agent: ");
		sendline(s, use_ssl, useragent);
	}
	sendline(s, use_ssl, "Connection: close");

	if (referer) {
		sendstr(s, use_ssl, "Referer: ");
		sendline(s, use_ssl, referer);
	}
	if (cookies) {
		sendstr(s, use_ssl, "Cookie: ");
		sendline(s, use_ssl, cookies);
	}
	sendline(s, use_ssl, "Content-Type: application/x-www-form-urlencoded");
	snprintf(buf, 200, "Content-Length: %lu", (unsigned long)strlen(datas));
	sendline(s, use_ssl, buf);
	if (mischeaders && mischeaders[0] != '\0') { sendline(s, use_ssl, mischeaders); }
	sendline(s, use_ssl, ""); // une ligne vide signifie au serveur qu'on a fini avec les headers, c'est a lui maintenant
	sendline(s, use_ssl, datas);

	return 0;
}

