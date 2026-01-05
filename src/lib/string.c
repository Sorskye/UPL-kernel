#include "string.h"
#include "types.h"

size_t strlen(char *str) {
    size_t length = 0;
    while (str[length] != '\0') {
        length++;
    }
    return length;
}

void strcpy(char *dest, const char *src, size_t length) {
    if (length == 0) return; 

    size_t i;
    for (i = 0; i < length && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    dest[i] = '\0';  
}


static char *write_int(char *out, long long v) {
    char buf[32];
    int pos = 0;

    if (v < 0) {
        *out++ = '-';
        v = -v;
    }

    if (v == 0) {
        *out++ = '0';
        return out;
    }

    while (v > 0) {
        buf[pos++] = '0' + (v % 10);
        v /= 10;
    }

    while (pos--) {
        *out++ = buf[pos];
    }

    return out;
}

static char *write_uint(char *out, unsigned long long v) {
    char buf[32];
    int pos = 0;

    if (v == 0) {
        *out++ = '0';
        return out;
    }

    while (v > 0) {
        buf[pos++] = '0' + (v % 10);
        v /= 10;
    }

    while (pos--) {
        *out++ = buf[pos];
    }

    return out;
}

static char *write_hex(char *out, unsigned long long v, int width) {
    char buf[32];
    int pos = 0;

    if (v == 0 && width == 0) {
        *out++ = '0';
        return out;
    }

    while (v > 0) {
        unsigned digit = v & 0xF;
        buf[pos++] = (digit < 10) ? ('0' + digit) : ('a' + (digit - 10));
        v >>= 4;
    }

    while (pos < width) {
        *out++ = '0';
        width--;
    }

    while (pos--) {
        *out++ = buf[pos];
    }

    return out;
}

static char *write_str(char *out, const char *s) {
    while (*s) {
        *out++ = *s++;
    }
    return out;
}

/* --- main format function --- */

char *strconcat(char *out, const char *fmt, ...) {
    char *start = out;
    va_list args;
    va_start(args, fmt);

    for (size_t i = 0; fmt[i] != '\0'; ++i) {
        if (fmt[i] != '%') {
            *out++ = fmt[i];
            continue;
        }

        i++;

        /* width: %0NNN */
        int width = 0;
        if (fmt[i] == '0') {
            i++;
            while (fmt[i] >= '0' && fmt[i] <= '9') {
                width = width * 10 + (fmt[i] - '0');
                i++;
            }
        }

        /* %ll */
        int is_ll = 0;
        if (fmt[i] == 'l' && fmt[i+1] == 'l') {
            is_ll = 1;
            i += 2;
        }

        switch (fmt[i]) {
            case 'd': {
                if (is_ll)
                    out = write_int(out, va_arg(args, long long));
                else
                    out = write_int(out, va_arg(args, int));
                break;
            }

            case 'u': {
                if (is_ll)
                    out = write_uint(out, va_arg(args, unsigned long long));
                else
                    out = write_uint(out, va_arg(args, unsigned int));
                break;
            }

            case 'x': {
                if (is_ll)
                    out = write_hex(out, va_arg(args, unsigned long long), width);
                else
                    out = write_hex(out, va_arg(args, uint32_t), width);
                break;
            }

            case 's': {
                out = write_str(out, va_arg(args, const char*));
                break;
            }

            case 'c': {
                *out++ = (char)va_arg(args, int);
                break;
            }

            case '%': {
                *out++ = '%';
                break;
            }

            default: {
                *out++ = '%';
                *out++ = fmt[i];
                break;
            }
        }
    }

    *out = '\0';
    va_end(args);
    return start;
}

char* strstr(const char* haystack, const char* needle) {
    if (!*needle) {
        return (char*)haystack;
    }

    for (; *haystack; haystack++) {
        const char* h = haystack;
        const char* n = needle;

        while (*h && *n && (*h == *n)) {
            h++;
            n++;
        }

        if (!*n) {
            return (char*)haystack; 
        }
    }
    
    return NULL; 
}