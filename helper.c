#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <strings.h>

void to_lower(char s[]) {
	for (int i = 0; i < strlen(s); i++) {
        s[i] = tolower(s[i]);
    }
}

void search_string(char *line, char *s, int length, char message[], int message_size) {
    if (strncasecmp(line, s, length) == 0) {
        strncpy(message, line+length, message_size);
        message[message_size] = '\0';
    }
}