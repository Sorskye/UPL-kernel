
#include "types.h"
#include "task.h"
#include "vga-textmode.h"
#include "console.h"
#include "kerror.h"
#include "string.h"
#include "GDT.h"

// extern assembly routine
extern void context_switch(uint32_t **old_sp_ptr, uint32_t *new_sp);

task_t task_table[MAX_TASKS];
task_t *current_task = NULL;
static task_t *task_head = NULL;
static int task_count = 1;
static uint32_t next_pid = 1;

#define INITIAL_EFLAGS 0x202  
volatile uint32_t jiffies = 0;

task_t* PID_to_task(uint32_t PID){
    task_t* task = &task_table[PID];
    return task;
}


int send_process_message(uint32_t pid_to, message_t* msg){
    asm volatile("cli");

    // get task to send to
    task_t* task_to = NULL;
    for (int i = 0; i < MAX_TASKS; i++) {
        if (task_table[i].pid == pid_to) {
            task_to = &task_table[i];
            break;
        }
    }

    if (!task_to) {
        // pid not found
        asm volatile("sti");
        return -1; 
    }

    if (task_to->queue_full==1) {
        // queue full
        asm volatile("sti");
        return -2; 
    }

    msg->PID_SENDER = current_task->pid;

    // append to queue tail
    task_to->queue[task_to->queue_tail] = *msg;
    task_to->queue_tail = (task_to->queue_tail + 1) % MSG_QUEUE_SIZE;

    if (task_to->queue_tail == task_to->queue_head){
        task_to->queue_full = 1;
    }
    
    if(task_to->state == TASK_BLOCKED){
        task_to->state = TASK_READY;
    }

    asm volatile("sti");
    return 1;
}

int receive_process_message(message_t* out){
    asm volatile("cli");
    if(!current_task->queue_full && (current_task->queue_head == current_task->queue_tail)){
       
        current_task->state = TASK_BLOCKED;
        asm volatile("sti");
        return 0;
    }

    *out = current_task->queue[current_task->queue_head];
    current_task->queue_head = (current_task->queue_head + 1) % MSG_QUEUE_SIZE;

    current_task->queue_full = 0;

    asm volatile("sti");
    return 1;
}

void scheduler_update_time(void) {
    jiffies++;

    size_t cnt = 0;
    for (task_t *t = task_head; t; t = t->next) {
        if (t->state == TASK_SLEEPING && (int32_t)(t->wake_tick - jiffies) <= 0) {
            t->state = TASK_READY;
        }
        if (cnt++ >= task_count) break;
    }
    return;
}

task_t* scheduler_choose_next(void) {
    if (!current_task || !task_head) return current_task;

    task_t *next = current_task;
    size_t max = task_count + 1;

    while (max--) {
        next = next->next ? next->next : task_head;
        if (next->state == TASK_READY)
            return next;
    }

    return current_task;
}

void scheduler_tick(void) {
    scheduler_update_time();

    task_t *old = current_task;
    task_t *next = scheduler_choose_next();

    if (next != old)
        current_task = next;

    return;
}

int create_task(task_fn fn,char* name, void *arg) {
   
    if (task_count >= MAX_TASKS) return -1;
    asm volatile("cli");

    task_t *t = &task_table[task_count];
    t->pid = next_pid++;
    t->next = NULL;

    strcpy(t->taskname, name, strlen(name));
    t->taskname[sizeof(t->taskname) - 1] = '\0';

    uint32_t *sp = (uint32_t*)((uintptr_t)t->stack + KERNEL_STACK_SIZE);

    // set interrupt stack frame for interrupt return, after timer interrupt
    *(--sp) = INITIAL_EFLAGS;   // eflags
    *(--sp) = KERNEL_CODE_SELECTOR;  // cs
    *(--sp) = (uint32_t)fn;     // eip (entry point of the task)

    *(--sp) = 0x0; // eax
    *(--sp) = 0x0; // ecx
    *(--sp) = 0x0; // edx
    *(--sp) = 0x0; // ebx
    *(--sp) = 0x0; // esp (placeholder)
    *(--sp) = 0x0; // ebp
    *(--sp) = (uint32_t) arg; // esi (arg)
    *(--sp) = 0x0; // edi

    t->esp = sp;

    if (!task_head) {
        task_head = t;
        t->next = t;
    } else {
        task_t *tail = task_head;
        while (tail->next != task_head) tail = tail->next;
        tail->next = t;
        t->next = task_head;
    }


    // clear message queue
    t->queue_head = 0;
    t->queue_tail = 0;
    t->queue_full = false;
    t->state = TASK_READY;

    task_count++;
    asm("sti");
    return t->pid;
}

void start_multitasking(void) {

    if (!task_head) return;
    current_task = task_head;
    
    uint32_t *kernel_esp;
    context_switch(&kernel_esp, current_task->esp);
}

task_t* get_task_table(){
    return task_table;
}