#include <stdio.h>
#include <ctype.h>

/* sets all chars of string to lower case*/
void to_lower(char s[]);

/* searches for a string and places the message after the string into a "message" to send off */
void search_string(char *line, char *s, int length, char message[], int message_size);