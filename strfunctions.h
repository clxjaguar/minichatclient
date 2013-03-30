//void strrep(const char* in, char** out, const char* old, const char* new);
unsigned int strrep(const char *input, char **buffer, const char *search, const char *replace);
unsigned char* utf8_character_from_ucs_codepoint(unsigned int ucs_character);
unsigned int extract_codepoints_from_utf8(const char **in);
unsigned char transliterate_ucs_to_cp850(unsigned int ucs_codepoint);
unsigned int transliterate_cp850_to_ucs(unsigned char cp850_codepoint);
unsigned char transliterate_ucs_to_iso88591(unsigned int ucs_codepoint);
unsigned int transliterate_iso88591_to_ucs(unsigned char iso_codepoint);
