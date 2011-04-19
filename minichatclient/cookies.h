#define MAXCOOKIES 10

typedef struct {
    char *name;
    char *value;
} cookie_t;

int storecookie(cookie_t *cookies, char* name, char* value);
int listcookies(cookie_t *cookies);
char* getcookie(cookie_t *cookies, char *name);
int generate_cookies_string(cookie_t *cookies, char *buf, int maxbuf);
int freecookies(cookie_t *cookies);
int parsehttpheadersforgettingcookies(cookie_t *cookies, const char *httpheaders);
