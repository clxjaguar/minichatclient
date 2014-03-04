int irc_init(const char *host, const char *port, const char *fakehost, const char *channel_name, const char *forum_username);
const char* irc_driver(void);
int irc_destroy(void);
void irc_join(const char *nickname, const char *ident);
void irc_part(const char *nickname, const char *ident, const char *partmsg);
void irc_message(const char *nickname, const char *ident, const char *message);
void irc_topic(const char *topic);
void irc_set_topic_mode(int mode);

