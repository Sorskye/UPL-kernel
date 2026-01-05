#ifndef STRING_H
#define STRING_H
#include "types.h"
void strcpy(char *dest, const char *src, size_t length);
size_t strlen(char *str);
char* strconcat(char *out, const char *fmt, ...);

char* strstr(const char* haystack, const char* needle);

#endif