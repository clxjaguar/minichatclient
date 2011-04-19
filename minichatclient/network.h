void ws_init(void);
void ws_cleanup(void);
int maketcpconnexion(char* hostname, int port);
int sendstr(int s, char* buf);
int sendline(int s, char* buf);
int http_get(int s, char* req, char* host, char* referer, char* cookies, char* useragent, char *mischeaders);
int http_post(int s, char* req, char* host, char* datas, char* referer, char* cookies, char* useragent, char* mischeaders);
