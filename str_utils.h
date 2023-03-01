#define relu(x) x < 0 ? 0 : x

void int_to_string(int n, char s[]);
void reverse_string(char s[]);
void bytescpy(char* dest, const char* src, int n);
int stristr(const char* string, const char* expr);
char starts_with(const char* str, const char* ref);
char char_in_str(const char* str, char c);
char starts_with_case_unsensitive(const char* str, const char* ref);
char* strtrim(char* str);