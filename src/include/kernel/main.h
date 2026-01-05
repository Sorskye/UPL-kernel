// kernel/kernel.h
#ifndef KERNEL_H
#define KERNEL_H

#include "types.h"
#include "memory.h"

volatile bool SAFE_MODE = false;
volatile bool VM_MODE = false;
volatile bool DEBUG_MODE = false;

void kernel_main(uint32_t magic, struct multiboot_info* mbinfo);

#endif