#define MAX_URL_SIZE 1024

#define is_web_extension(extension) ((extension[0] == '\0' || strcmp(extension, ".html") == 0 \
                    || strcmp(extension, ".htm") == 0 || strcmp(extension, ".asp") == 0 \
                    || strcmp(extension, ".aspx") == 0 || strcmp(extension, ".xml") == 0 \
                    || strcmp(extension, ".php") == 0 || strcmp(extension, ".php3") == 0 \
                    || strcmp(extension, ".shtm") == 0 || strcmp(extension, ".shtml") == 0 \
                    || strcmp(extension, ".cfm") == 0 || strcmp(extension, ".cfml") == 0))

#define is_image_extension(extension) (strcmp(extension, ".png") == 0 || strcmp(extension, ".jpg") == 0 \
                    || strcmp(extension, ".jpeg") == 0 || strcmp(extension, ".bmp") == 0 \
                    || strcmp(extension, ".webp") == 0 || strcmp(extension, ".gif") == 0)


void url_slicer(const char* url, char* host, char* extension);
char trusted_host(const char* host);
void retrieve_absolute_url(char* url, const char* reference_url);