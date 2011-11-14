void strrep(const char* in, char** out, char* old, char* new);
unsigned char transliterate_ucs_to_cp850(unsigned int ucs_codepoint);
unsigned char* utf8_character_from_ucs_codepoint(unsigned int ucs_character);
unsigned int extract_codepoints_from_utf8(char **in);
unsigned int transliterate_cp850_to_ucs(unsigned char cp850_codepoint);
