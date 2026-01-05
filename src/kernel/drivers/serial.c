#include "serial.h"
#include "io.h"

static int serial_ready() {
    return inb(0x3F8 + 5) & 0x20;
}

void serial_write(char c){
    while (!serial_ready());
    outb(0x3F8,c);
}

void serial_print(const char* s){
    while (*s) serial_write(*s++);
}