// kernel/kernel.c
//libs
#include "main.h"
#include "types.h"
#include "console.h"
#include "kerror.h"
#include "GDT.h"
#include "IDT.h"
#include "io.h"
#include "memory.h"
#include "string.h"
#include "ps2_key.h"
#include "sleep.h"

#include "task.h"
#include "WM.h"

#define KERNEL_VER_REL 0
#define KERNEL_VER_MAJ 1
#define KERNEL_VER_MIN 0
// drivers
#include "vga-textmode.h"
#include "pit.h"

// temp
void no_sse(){
    vga_refresh();
    printf("FOUT: SSE is niet beschikbaar..\n");
    printf("zonder SSE kan deze versie van de KRS-kernel niet opstarten.");
    vga_refresh();
    return;
}

void kernel_main(uint32_t magic, struct multiboot_info* mbinfo) {
    if (magic != 0x1BADB002){
        //halt();
    }

    const char* cmdline = (const char*) mbinfo->cmdline;

    if (cmdline) {
    // OS modus voorbeeld
    if (strstr(cmdline, "mode=safe")) {
       SAFE_MODE = true;
    }
    if (strstr(cmdline, "mode=debug")) {
        DEBUG_MODE = true;
    }
    if (strstr(cmdline, "mode=vm")) {
        VM_MODE = true;
    }
    }

    // TODO: clean vga driver
    setsource_console_putchar(vga_putc);
    setsource_console_clear(vga_clear);
    setsource_console_color(vga_set_color);
    setsource_console_cursor(vga_set_cursor);

    if (VM_MODE){
        setsource_console_attr(vga_make_attr);
    }else{setsource_console_attr(vga_make_attr_blink);}
    
    vga_init();
    

    console_color(VGA_WHITE, VGA_BLACK);
    clear();

    // TODO: multiboot checksum controleren in parse_memory_map()

    GDT_install();
    IDT_install();

    parse_memory_map(mbinfo);

    if(SAFE_MODE){
        pit_init((uint32_t)150);
    }else{ pit_init((uint32_t)1000);}
    
    

    // applications


    void task_mng(){
        
        keyboard_subscribe(current_task);

        extern uint8_t __kernel_start[];
        extern uint8_t __kernel_end[];

        uint32_t taskmngID = wm_api_make_window(VGA_BLACK, VGA_YELLOW, 25, 15, 1, 2, true, true, "Task Manager");

        uint32_t used_percentage(uint32_t total_bytes, uint32_t free_bytes) {
            if (total_bytes == 0) return 0;
            uint32_t used = total_bytes - free_bytes;
            return (used * 100U + total_bytes / 2) / total_bytes;
        }

        void refresh(){
            
            
            uint8_t cursor_x = 2;
            uint8_t cursor_y = 2;

            mem_inf_t* mem_inf = get_mem_inf();
            uint32_t total_usable_memory = mem_inf->total_usable_memory;
            memory_block_t* memory_block_list = mem_inf->memory_block_list;

                  
            uint32_t total_free_memory = 0;
            memory_block_t* cur = memory_block_list;

            while (cur) {
                uint32_t raw = cur->len & ~1U;
                total_free_memory += raw;
                cur = cur->next;
            }

            uint32_t mem_free_mb = total_free_memory / 1024 /1024;
            uint32_t mem_total_mb = total_usable_memory / 1024 / 1024;
            uint32_t percentage_used = used_percentage(total_usable_memory, total_free_memory);
            char msg[16];
            strconcat(msg, "MEM FREE: %d-MB",mem_free_mb);
            wm_api_place_text(VGA_BLACK, VGA_YELLOW, cursor_x, cursor_y, msg, taskmngID);
            cursor_y+=1;
            strconcat(msg, "MEM TOT: %d-MB",mem_total_mb);
            wm_api_place_text(VGA_BLACK, VGA_YELLOW, cursor_x, cursor_y, msg, taskmngID);
            cursor_y+=2;
            strconcat(msg, "USED: %d%%",percentage_used);
            wm_api_place_text(VGA_BLACK, VGA_YELLOW, cursor_x, cursor_y, msg, taskmngID);
            cursor_y+=2;
       
            wm_api_place_text(VGA_BLACK, VGA_YELLOW, cursor_x, cursor_y, "PID    TASK_NAME", taskmngID);
            
            wm_api_place_text(VGA_BLACK, VGA_YELLOW, cursor_x, cursor_y, "", taskmngID);
           
                task_t* task_table = get_task_table();
            
                for (uint8_t id = 0; id < 16; id++)
                {   
                    if(task_table[id].pid == 0){continue;}
                    char str[256];
                    strconcat(str, "%d  -  %s",task_table[id].pid, task_table[id].taskname);
                    wm_api_place_text(VGA_BLACK, VGA_YELLOW, cursor_x, cursor_y+id, str, taskmngID);                
                }  

            
        }
        refresh();

        bool isthere = true;
        void inc_msg(message_t* msg){
            if (msg->type == PS2_NEW_INPUT){
                uint8_t scancode = (msg->arg0 >> 24) & 0xFF;
                char key = translate_scancode(scancode);

                if(key == 'r'){
                   refresh();
                }
            }
        }    
        
        message_t msg;
        while(true){
           if(receive_process_message(&msg)){
                inc_msg(&msg);
           }
        }
    }

    void AWM(){
        init_wm();
        wm_loop();
    }

    void desktop(){


        
        uint32_t init_ID = wm_api_make_window(VGA_BLACK, VGA_LIGHT_GRAY, GRID_WITDH, GRID_HEIGHT, 0, 0, false, false, "INIT");
        
        char *str = "                                                                                ";
        wm_api_place_text(VGA_LIGHT_GRAY, VGA_WHITE, 0, 0, str, init_ID);
        wm_api_place_text(VGA_DARK_GRAY, VGA_WHITE, 0, 0, "KRS-kernel versie 0.1.1", init_ID);
        
        while (true)
        {
            asm ("nop");
        }
        
    }

    void term32(){
        uint32_t newwin = wm_api_make_window(VGA_GREEN, VGA_BLACK, 35, 17, 30, 2, true, true,"term");
        uint32_t cursorx = 1;
        uint32_t cursory = 1;

        size_t maxbuffersize = 512;
        char *keyboardbuffer = malloc(maxbuffersize);

        void handle_return(){

            if(keyboardbuffer){
                free(keyboardbuffer);
            }
            keyboardbuffer = malloc(maxbuffersize);
        }

        void handle_process_message(message_t* msg){
            if (msg->type == PS2_NEW_INPUT ){
                uint8_t scanline = (msg->arg0 >> 24) & 0xFF;
                char key = translate_scancode(scanline);

                if (scanline == 0x1c){
                    handle_return();
                    cursorx = 1;
                    cursory += 1;
                }else if(cursorx >= 34){
                    cursorx = 1;
                    cursory += 1; 
                }

                if(key!=0){
                    strconcat(keyboardbuffer, "%c", key);
                    char str[2];
                    strconcat(str, "%c",key);
                    wm_api_place_text(VGA_GREEN, VGA_BLACK, cursorx, cursory, str, newwin);
                    cursorx++;
                }

                
            }
        }

        while(true){
            message_t msg;
            if(receive_process_message(&msg)){
                handle_process_message(&msg);
            }
        }
    }

    int WM_PID = create_task((void*)AWM, (char*)"AWM",0);
    int keyboard_worker_PID = create_task((void*)keyboard_worker, "kb_thread", 0);

    if(!SAFE_MODE){
        int desktop_PID = create_task((void*)desktop, (char*)"desktop", 0);
        int taskmng_PID = create_task((void*)task_mng, (char*)"task-mng", 0);
        int term = create_task((void*)term32, (char*)"term",0);
        task_t* task = PID_to_task(term);
        keyboard_subscribe(task);
        
    }else{
        // safe mode applications
        void info_task(){
            int info_window = wm_api_make_window(VGA_WHITE, VGA_BLUE, GRID_WITDH, GRID_HEIGHT, 0, 0, false, false, "NOTICE");
            wm_api_place_text(VGA_WHITE, VGA_BLUE, 2,1,"RUNNING IN SAFE MODE", info_window);
            wm_api_place_text(VGA_WHITE, VGA_BLUE, 2,3,"only REQUIRED drivers,", info_window);
            wm_api_place_text(VGA_WHITE, VGA_BLUE, 2,4,"and task are running", info_window);
            wm_api_place_text(VGA_WHITE, VGA_BLUE, 2,6,"press CTRL+T to open a terminal window", info_window);
            
            while(true){
                asm volatile("nop");
            }
        }
        int info_app = create_task((void*)info_task, "safemode info", 0);
        
    }
    start_multitasking();
    

    while (true)
    {
        halt();
    }
    
}

 
