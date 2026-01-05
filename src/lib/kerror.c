#include "console.h"
#include "IDT.h"
#include "kerror.h"
#include "symbols.h"
#include "task.h"

void halt(){
    while (true){
        asm volatile ("hlt");
    }
}

const char* lookup_symbol(uint32_t addr) {
    const char* result = "onbekend";
    uint32_t best = 0;

    for (int i = 0; i < kernel_symbol_count; i++) {
        uint32_t sym = kernel_symbols[i].addr;

        // Find the symbol with the highest address that is still <= addr
        if (sym <= addr && sym >= best) {
            best = sym;
            result = kernel_symbols[i].name;
        }
    }

    return result;
}



void print_stack_trace(panic_registers *regs) {
    uint32_t *ebp = (uint32_t*)regs->ebp;
    uint32_t eip = regs->eip;

    printf("Stack trace:\n");
    printf(" -> %08x : %s\n", eip, lookup_symbol(eip));

    int frame_count = 0;
    while (ebp && frame_count < 32) { // limit frames to avoid infinite loop
        uint32_t ret = ebp[0];

        // sanity check return address

        printf(" -> %08x : %s\n", ret, lookup_symbol(ret));

        uint32_t *next_ebp = (uint32_t*)ebp[0];

        // sanity check next EBP
        if (next_ebp <= ebp) 
            break;  // prevent loops or stack underflow

        ebp = next_ebp;
        frame_count++;
    }
}

void print_eflags(uint32_t eflags)
{
    printf("EFLAGS = 0x%08x\n", eflags);

    printf("CF = %d | ",  (eflags >> 0) & 1);
    printf("PF = %d | ",  (eflags >> 2) & 1);
    printf("AF = %d |",  (eflags >> 4) & 1);
    printf("ZF = %d | ",  (eflags >> 6) & 1);
    printf("SF = %d | ",  (eflags >> 7) & 1);
    printf("TF = %d\n",  (eflags >> 8) & 1);
    printf("IF = %d | ",  (eflags >> 9) & 1);
    printf("DF = %d | ",  (eflags >> 10) & 1);
    printf("OF = %d |",  (eflags >> 11) & 1);

    printf("IOPL = %d | ",  (eflags >> 12) & 3);

    printf("NT = %d | ",  (eflags >> 14) & 1);
    printf("RF = %d\n",  (eflags >> 16) & 1);
    printf("VM = %d | ",  (eflags >> 17) & 1);
    printf("AC = %d | ",  (eflags >> 18) & 1);
    printf("VIF = %d \n",  (eflags >> 19) & 1);
    printf("VIP = %d | ",  (eflags >> 20) & 1);
    printf("ID = %d\n\n",  (eflags >> 21) & 1);
}

void panic(panic_registers *regs){

    console_color(VGA_WHITE, VGA_BLUE);
    vga_clear();

    vga_refresh();
    set_console_cursor(0,0);

    char* msg = interruptMessages[regs->intNum];

    printf("Onherstelbare systeem fout. Uitvoering uitgesteld.\n");
   
    printf("\n[!!!] ");
    printf("KERNEL PANIC! FAULT_ID >> %s\n",msg);
   

    if(in_interrupt>0){
        printf("_WHILE_IRQ_%s\n",current_irq_name);
        printf("IRQ_NEST: %d",in_interrupt);
    }

    printf("CS: 0x%08x | ",regs->cs);
    printf("DS: 0x%08x\n",regs->ds);
    printf("ES: 0x%08x | ",regs->es);
    printf("GS: 0x%08x\n",regs->gs);

    printf("EAX: 0x%08x | ",regs->eax);
    printf("EBX: 0x%08x\n",regs->ebx);
    printf("ECX: 0x%08x | ",regs->ecx);
    printf("EDX: 0x%08x\n",regs->edx);
    printf("ESI: 0x%08x | ",regs->esi);
    printf("EDI: 0x%08x\n",regs->edi);
    printf("ESP: 0x%08x | ",regs->esp);
    printf("EBP: 0x%08x\n",regs->ebp);

   
   // print_eflags(regs->eflags);

    printf("FAULT AT EIP: %08x > %s\n",regs->eip, lookup_symbol(regs->eip));
    printf("PID: %d\n\n",current_task->pid);

    print_stack_trace(regs);
    
    vga_refresh();
    halt();
}


Char *interruptMessages[] = {

    "DIVISION_FAULT",       //0
    "DEBUG",                  //1
    "NON_MASKABLE_INT", //2
    "BREAKPOINT",             //3
    "INTO_DETECTED_OVERFLOW", //4
    "OUT_OF_BOUND",          //5
    "INVALID_OPTCODE",         //6
    "NO_COPROCESSOR",         //7

    "DOUBLE_FAULT",                //8
    "COPROCESSOR_SEGMENT_OVERRUN", //9
    "BAD_TSS",                     //10
    "SEGMENT_NOT_PRESENT",         //11
    "STACK_FAULT",                 //12
    "GENERAL_PROTECTION",    //13
    "PAGE_FAULT", //14
    "UNKNOWN_INTERRUPT", //15

    "COPROCESSOR_FAULT", //16
    "ALIGNMENT_CHECK", //17
    "MACHINE_CHECK", //18
    "Reserved", //19
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",

    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
};




Char *irqMessages[] = {
    "SYSTEM_TIMER",       //0
    "KEYBOARD_PS/2",                  //1
    "INT_CONTROLLER", //2
    "SERIAL_CONTROLLER_2",             //3
    "SERIAL_CONTROLLER_1", //4
    "PARRALLEL_PORT_3_OR_ISA_SOUND",          //5
    "FLOPPY_CONTROLLER",         //6
    "PARRALLEL_PORT_1",         //7

    "RTC",                //8
    "ACPI", //9
    "PERIPHERAL_1",                     //10
    "PERIPHERAL_2",         //11
    "MOUSE_PS/2",                 //12
    "CO_PROCESSOR_OR_FLOATING_POINT_UNIT",    //13
    "ATA_PRIMARY",
    "ATA_SECONDARY",
};
