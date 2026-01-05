
#pragma once
#ifndef PS2_KEY
#define PS2_KEY

#include "task.h"
#define PS2_NEW_INPUT 60

#define SC_LSHIFT_PRESS   0x2A
#define SC_RSHIFT_PRESS   0x36
#define SC_LSHIFT_RELEASE 0xAA
#define SC_RSHIFT_RELEASE 0xB6

#define KB_RING_SIZE 512

typedef struct kb_subscriber {
    task_t *task;
    struct kb_subscriber *next;
} kb_subscriber_t;

static kb_subscriber_t *kb_subscribers = NULL;

typedef struct kb_ring{
    uint8_t buf[KB_RING_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
} kb_ring_t;
static kb_ring_t kb_ring = { .head = 0, .tail= 0 };

void keyboard_subscribe(task_t* task);
void keyboard_irq();
char translate_scancode(uint8_t sc);
void keyboard_worker();


#endif