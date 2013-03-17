void ws_init(void);
void ws_cleanup(void);
int maketcpconnexion(const char* hostname, unsigned int port);
int sendstr(int s, const char* buf);
int sendline(int s, const char* buf);
int http_get(int s, const char* req, const char* host, const char* referer, const char* cookies, const char* useragent, const char *mischeaders);
int http_post(int s, const char* req, const char* host, const char* datas, const char* referer, const char* cookies, const char* useragent, const char* mischeaders);
