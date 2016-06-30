int network_init(int use_ssl);
int network_cleanup(void);
int maketcpconnexion(const char* hostname, const char *service, int use_ssl);
ssize_t network_recv(int sockfd, int use_ssl, void *buf, size_t len, int flags);
int sendstr(int s, int use_ssl, const char* buf);
int sendline(int s, int use_ssl, const char* buf);
int http_get(int s, int use_ssl, const char* req, const char* host, const char* referer, const char* cookies, const char* useragent, const char *mischeaders);
int http_post(int s, int use_ssl, const char* req, const char* host, const char* datas, const char* referer, const char* cookies, const char* useragent, const char* mischeaders);
