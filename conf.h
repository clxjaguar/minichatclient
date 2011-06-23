int read_conf_int(const char *key, int defaultvalue);
char *read_conf_string(const char *key, char *value, size_t valuesize);
void close_conf_file(void);
