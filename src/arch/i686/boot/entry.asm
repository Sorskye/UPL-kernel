section .multiboot
align 4
    MULTIBOOT_MAGIC      equ 0x1BADB002
    MULTIBOOT_FLAGS      equ (1 << 0) | (1 << 1)    ; no memory map, no video mode, simple setup
    MULTIBOOT_CHECKSUM   equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM

section .text
align 4
bits 32
extern kernel_main
extern no_sse

global gdt_flush
gdt_flush:
    
    mov eax, [esp + 4]
    lgdt [eax]

    jmp 0x08:flush_cs_reload

flush_cs_reload:
    mov ax, 0x10 ; 0x10 = gdt index 2 (kernel data)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret

global _start
_start:
    mov esp, stack_top
    push ebx        ; multibooot memory map pointer
    push eax        ; magic number

    mov eax, 1
    cpuid
    test edx, (1<<25)
    jz no_sse
    ;SSE is available
   
    call kernel_main

.hang:
    hlt
    jmp .hang

section .bss
stack_bottom: resb 4096
stack_top: