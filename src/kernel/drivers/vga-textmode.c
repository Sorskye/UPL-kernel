
#include "types.h"
#include "vga-textmode.h"
#include "io.h"
#include "memory.h"
#include "kerror.h"
#include "console.h"

static volatile uint16_t* vga_buff_addr = (uint16_t*)0xB8000;
volatile uint8_t cursor_x =0, cursor_y = 0;
const uint8_t GRID_WITDH=80, GRID_HEIGHT=25;
static volatile uint8_t VGA_ENTRY_ATTR = VGA_WHITE | VGA_BLACK << 4;

uint16_t* vga_framebuffer = NULL;
uint16_t* alloc_framebuffer(){
    return (uint16_t*)malloc(4000);
}

void vga_push_framebuffer(uint16_t* framebuffer){
    memcpy(vga_buff_addr, framebuffer, 4000);

}

void vga_refresh(){
    vga_push_framebuffer(vga_framebuffer);
}


void vga_enable_cursor(uint8_t cursor_start, uint8_t cursor_end)
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, (inb(0x3D5) & 0xC0) | cursor_start);

	outb(0x3D4, 0x0B);
	outb(0x3D5, (inb(0x3D5) & 0xE0) | cursor_end);
}

void vga_disable_cursor()
{
	outb(0x3D4, 0x0A);
	outb(0x3D5, 0x20);
}

void vga_disable_blink(){
    outb(0x3C0, 0x10);
    outb(0x3C1, inb(0x3C1) & ~0x08 | 0x20);
}

void vga_enable_blink() {
    // Reset attribute controller flip-flop
    (void)inb(0x3DA);

    // Selecteer register 0x10
    outb(0x3C0, 0x10);

    // lees huidige waarde
    uint8_t val = inb(0x3C1);

    // zet knipper
    val |= 0x08; 
    val &= ~0x20;

    // Reset flip-flop
    (void)inb(0x3DA);

    // schrijf waarde
    outb(0x3C0, val);
}

static inline void vga_load_default_palette() {
    outb(0x00, 0x3C8);
    for (int i = 0; i < 16; i++) {
        outb(vga_default_palette[i][0] >> 2, 0x3C9);
        outb(vga_default_palette[i][1] >> 2, 0x3C9);
        outb(vga_default_palette[i][2] >> 2, 0x3C9);
    }
}

void vga_init(){
    vga_framebuffer = alloc_framebuffer();
    vga_load_default_palette();
}

// framebuffer ops
uint8_t vga_make_attr_blink(enum VGA_COLOR FG, enum VGA_COLOR BG, bool blink){
    uint8_t attr = (uint8_t)(FG | (BG & 0x7) << 4);
    if(blink){
        attr |= 0x80;
    }
    return attr;
}

uint8_t vga_make_attr(enum VGA_COLOR FG, enum VGA_COLOR BG, bool blink){
    return (uint8_t)(FG | BG << 4);
}


void framebuff_putc_at(char c, uint16_t x, uint16_t y, uint16_t* framebuffer, uint8_t ATTRIBUTE){
    const size_t index = y * GRID_WITDH + x;
    framebuffer[index] = ((uint8_t)ATTRIBUTE << 8) | (uint8_t)c;
}

void framebuffer_putc(char c, uint16_t x, uint16_t y, uint16_t* framebuffer, uint8_t ATTRIBUTE) {  
    if (c == '\n') { x = 0; y++; }  
    else {
        const size_t index = y * 80 + x;
        framebuffer[index] = ((uint8_t)ATTRIBUTE << 8) | (uint8_t)c;
        x++;
    }
}

void framebuff_putstring(char *str, uint16_t x, uint16_t y, uint16_t* framebuffer, uint8_t ATTRIBUTE){
    while(*str){
        framebuffer_putc(*str++,  x,  y, framebuffer, ATTRIBUTE);
    }
}

void framebuff_clear(uint16_t* framebuffer, uint16_t cx, uint16_t cy){
    VGA_ENTRY_ATTR &= 0x7F;
    for(int x=0; x < GRID_WITDH; x++){
        for(int y=0; y < GRID_HEIGHT; y++){
            cx = x;
            cy = y;
            framebuff_putc_at(' ', cx, cy, framebuffer, VGA_ENTRY_ATTR);
        }
    }

    cx = 0;
    cy = 0;
}

// hardware ops

void vga_set_cursor(uint16_t x, uint16_t y)
{
	uint16_t pos = y * GRID_WITDH + x;

	outb(0x3D4, 0x0F);
	outb(0x3D5, (uint8_t) (pos & 0xFF));
	outb(0x3D4, 0x0E);
	outb(0x3D5, (uint8_t) ((pos >> 8) & 0xFF));

    cursor_x = x;
    cursor_y = y;
}

void vga_putc_at(char c, int x, int y){
    const size_t index = y * 80 + x;
    vga_framebuffer[index] = ((uint8_t)VGA_ENTRY_ATTR << 8) | (uint8_t)c;
}
void vga_putc(char c) {
    
    if (c == '\n') { cursor_x = 0; cursor_y++; }
    else {
        const size_t index = cursor_y * 80 + cursor_x;
        vga_framebuffer[index] = ((uint8_t)VGA_ENTRY_ATTR << 8) | (uint8_t)c;
        cursor_x++;
    }
}

void vga_putstr(char *str){
    while(*str){
        vga_putc(*str++);
    }
}

int vga_get_cursor(){
    return cursor_x, cursor_y;
}

void vga_set_color(enum VGA_COLOR FG, enum VGA_COLOR BG){
    VGA_ENTRY_ATTR = FG | BG << 4;
}

void vga_clear(){


    VGA_ENTRY_ATTR &= 0x7F;
    for(int x=0; x < GRID_WITDH; x++){
        for(int y=0; y < GRID_HEIGHT; y++){
            cursor_x = x;
            cursor_y = y;
            vga_putc(' ');
        }
    }

    cursor_x = 0;
    cursor_y = 0;
}    //vga_enable_blink();
  
   // vga_disable_blink();