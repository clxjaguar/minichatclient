#ifndef PARSEHTML_H_INCLUDED
#define PARSEHTML_H_INCLUDED

typedef struct {
    char *username;
    char *message;
    char *usericonurl;
    char *userprofileurl;
    char msgid[30];
} message_t;

int parser_freerules(void);
int parser_loadrules(void);
unsigned int parse_minichat_mess(char *input, unsigned int bytes, message_t *msg, int reset);
void minichat_message(char* username, char* message, char *usericonurl, char *userprofileurl);
void minichat_users_at_this_moment(char *string);

#endif // MESSAGE_H_INCLUDED
