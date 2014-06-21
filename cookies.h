#define MAXCOOKIES 10

typedef struct {
    char *name;
    char *value;
} cookie_t;

int storecookie(cookie_t *cookies, const char *name, const char *value);
int listcookies(cookie_t *cookies);
char* getcookie(cookie_t *cookies, char *name);
char* generate_cookies_string(cookie_t *cookies, char *buf, unsigned int buflen);
int freecookies(cookie_t *cookies);
int parsehttpheadersforgettingcookies(cookie_t *cookies, const char *httpheaders, ssize_t bytes);

void set_creation_time(char *string);
char *get_creation_time(void);
void set_form_token(char *string);
char *get_form_token(void);
