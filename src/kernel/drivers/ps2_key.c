#include "types.h"
#include "task.h"
#include "memory.h"
#include "ps2_key.h"
#include "IDT.h"
#include "io.h"
#include "console.h"
#include "kerror.h"

static bool shift = false;

static const char scancode_to_ascii[128] = {
    0,   27, '1','2','3','4','5','6','7','8','9','0','-','=', '\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n', 0,
    'a','s','d','f','g','h','j','k','l',';','\'','`', 0,'\\','z','x',
    'c','v','b','n','m',',','.','/', 0,'*', 0,' ', 0,
};

static const char scancode_to_ascii_shift[128] = {
    0,   27, '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n', 0,
    'A','S','D','F','G','H','J','K','L',':','"','~', 0,'|','Z','X',
    'C','V','B','N','M','<','>','?', 0,'*', 0,' ', 0,
};

// thread context
// ring buffer helpers
static inline bool kb_ring_empty(void) {
    return kb_ring.head == kb_ring.tail;
}


static inline bool kb_ring_full(void) {
    return ((kb_ring.head + 1) & (KB_RING_SIZE - 1)) == kb_ring.tail;
}



static inline bool kb_ring_get(uint8_t *sc) {
    if (kb_ring_empty()) return false;
    *sc = kb_ring.buf[kb_ring.tail];
    kb_ring.tail = (kb_ring.tail + 1) & (KB_RING_SIZE - 1);
    return true;
}


void keyboard_subscribe(task_t* task){
    kb_subscriber_t *sub = malloc(sizeof(kb_subscriber_t));
    if(!sub) return;
    sub->task = task;
    sub->next = kb_subscribers;
    kb_subscribers = sub;
}

char translate_scancode(uint8_t sc)
{
    /* Handle key releases */
    if (sc & 0x80) {
        uint8_t code = sc & 0x7F;

        if (code == SC_LSHIFT_PRESS || code == SC_RSHIFT_PRESS)
            shift = false;

        return 0;   // no ASCII
    }

    /* Handle shift presses */
    if (sc == SC_LSHIFT_PRESS || sc == SC_RSHIFT_PRESS) {
        shift = true;
        return 0;
    }

    /* Bounds check */
    if (sc > 127)
        return 0;

    /* Lookup tables */
    if (shift)
        return scancode_to_ascii_shift[sc];
    else
        return scancode_to_ascii[sc];

    
}



void keyboard_worker(uint32_t pid_sender) {
    while (true){
        uint8_t sc;
        while (kb_ring_get(&sc)) {
            // Build message
            // Loop through subscribers
            kb_subscriber_t *sub = kb_subscribers;
            while (sub) {
                if (sub->task) {
                
                    message_t *msg = malloc(sizeof(message_t));;
                    msg->type       = PS2_NEW_INPUT;
                    msg->arg0       = ((uint32_t)sc) << 24;  // scancode in high byte
                    msg->PID_SENDER = pid_sender; // e.g. keyboard driver thread PID
                    send_process_message(sub->task->pid, msg);
                    free(msg);
                
                }
                sub = sub->next;
            }
        }
    }
}



// irq context 
static inline void kb_ring_put(uint8_t sc) {
    if (!kb_ring_full()) {
        kb_ring.buf[kb_ring.head] = sc;
        kb_ring.head = (kb_ring.head + 1) & (KB_RING_SIZE - 1);
    } else {
        kb_ring.tail = (kb_ring.tail + 1) & (KB_RING_SIZE - 1);
        kb_ring.buf[kb_ring.head] = sc;
        kb_ring.head = (kb_ring.head + 1) & (KB_RING_SIZE - 1);
    }
}

void keyboard_irq(){

    uint8_t scancode = inb(0x60);
    if((scancode & 0x80) != 0){return;}
   
    kb_ring_put(scancode);
    return;
}

