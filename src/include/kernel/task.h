#include <stdint.h>
#ifndef TASK_H
#define TASK_H

#define MAX_TASKS 512
#define KERNEL_STACK_SIZE 4096

typedef void (*task_fn)(void *arg);
extern volatile uint32_t jiffies;

#define TASK_BLOCKED 0
#define TASK_IDLE 1
#define TASK_SLEEPING 2
#define TASK_READY 3
#define TASK_RUNNING 4
#define TASK_EXITED 5

typedef struct message{
    uint32_t type;
    uint32_t arg0;
    uint32_t arg1;
    uint32_t arg2;
    uint32_t arg3;
    uint32_t arg4;
    uint32_t arg5;
    void*    data0;
    void*    data1;
    void*    data2;
    uint32_t PID_SENDER;
    
} message_t;
#define MSG_QUEUE_SIZE 256

typedef struct task {
    uint32_t *esp;       
    uint32_t pid;
    uint8_t stack[KERNEL_STACK_SIZE] __attribute__((aligned(16)));
    char taskname[24];
    struct task* next;

    // IPC
    message_t queue[MSG_QUEUE_SIZE];
    int queue_head;
    int queue_tail;
    bool queue_full;
    uint8_t state;
    uint32_t wake_tick;
} task_t;


task_t* scheduler_choose_next(void);
void scheduler_tick(void);


int create_task(task_fn fn, char* name, void *arg);
void start_multitasking(void);
task_t* get_task_table();
int send_process_message(uint32_t pid_to, message_t* msg);

int receive_process_message(message_t* out);

task_t* PID_to_task(uint32_t pid);

extern task_t *current_task;

#endif