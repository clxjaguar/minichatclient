typedef struct {
	char *username;
	char *message;
	char *usericonurl;
	char *userprofileurl;
	char msgid[30];
} message_t;

int parser_freerules(void);
int parser_loadrules(void);
unsigned int parse_minichat_mess(char *input, signed int bytes, message_t *msg, int reset);
