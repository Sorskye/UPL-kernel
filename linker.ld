ENTRY(_start)

SECTIONS
{
  . = 1M; /* Load the kernel at 1 MB */

  __kernel_start = .;

  .multiboot ALIGN(4K) :
  {
    KEEP(*(.multiboot))
  }

  .text ALIGN(4K) :
  {
    *(.text*)
  }

  .rodata ALIGN(4K) :
  {
    *(.rodata*)
  }

  .data ALIGN(4K) :
  {
    *(.data*)
  }

  .bss ALIGN(4K) :
  {
    *(COMMON)
    *(.bss*)
  }

  __kernel_end = .;
}
