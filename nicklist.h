void nicklist_init(void);
void nicklist_destroy(void);
void nicklist_msg_update(const char *username, const char *profil_url, const char *icon_url);
void nicklist_recup_start(void);
void nicklist_recup_end(void);
void nicklist_recup_name(const char* nickname);
void nicklist_topic(const char *string);
