#include "types.h"
#include "console.h"
#include "string.h"
#include "vga-textmode.h"

#include "kerror.h"
#include "memory.h"

#define ALIGN4(x)   (((x) + 3) & ~3U)
#define USED_FLAG   1U
#define MAX_MEM_ENTRIES 128

extern uint8_t __kernel_start[];
extern uint8_t __kernel_end[];

uint32_t total_usable_memory = 0;
struct memory_entry memory_entries[MAX_MEM_ENTRIES];
static memory_block_t* free_block_list = NULL;
uint32_t entries = 0;

// helpers
static inline uint32_t block_size(memory_block_t* b) {
    return b->len & ~USED_FLAG;
}
static inline bool block_used(memory_block_t* b) {
    return (b->len & USED_FLAG) != 0;
}
static inline void mark_used(memory_block_t* b) {
    b->len |= USED_FLAG;
}
static inline void mark_free(memory_block_t* b) {
    b->len &= ~USED_FLAG;
}


static void add_block(uint32_t addr, uint32_t len, int idx) {
    if (len < sizeof(memory_block_t) + 4) {
        return;
    }

    memory_block_t* block = (memory_block_t*)(uintptr_t)addr;
    block->len = ALIGN4(len) & ~USED_FLAG;
    block->next = free_block_list;
    free_block_list = block;
    total_usable_memory += block->len;
}


mem_inf_t* get_mem_inf(){
    mem_inf_t* info_struct;
    info_struct->memory_block_list = free_block_list;
    info_struct->total_usable_memory = total_usable_memory;
    return info_struct;
}

void init_allocator(struct memory_entry* entries, int entry_count) {
    free_block_list = NULL;

    uint32_t kstart = (uint32_t)(uintptr_t)__kernel_start;
    uint32_t kend   = (uint32_t)(uintptr_t)__kernel_end;

    for (int i = 0; i < entry_count; ++i) {
        uint32_t start = entries[i].addr;
        uint32_t end   = entries[i].addr + entries[i].len;

        if (end <= 0x00100000U) {
            // onder 1mb
            continue;
        }
        if (start < 0x00100000U) start = 0x00100000U;

        // buiten kernel
        if (end <= kstart || start >= kend) {
            // veilig
            add_block(start, end - start, i);
        } else {
            // overlap links
            if (start < kstart) {
                add_block(start, kstart - start, i);
            }
            // rechts
            if (end > kend) {
                add_block(kend, end - kend, i);
            }
        }
    }
}




void parse_memory_map(struct multiboot_info* mbinfo){
    if (!(mbinfo->flags & (1 << 6 ))) {
        printf("NO MEMORY MAP\n");
        return;
    }

    uint32_t mmap_end = mbinfo->mmap_addr + mbinfo->mmap_length;
    int av_entry_count = 0;

    for(uint32_t ptr = mbinfo->mmap_addr; ptr < mmap_end; ){
        struct multiboot_mmap_entry* entry = (struct multiboot_mmap_entry*)ptr;

        if(entry->type == MULTIBOOT_MEMORY_AVAILABLE){

            uint64_t addr64 = entry->addr;
            uint64_t len64  = entry->len;

            if (addr64 > 0xFFFFFFFFULL) {
                // meer dan 4GB ram
                ptr += entry->size + sizeof(entry->size);
                continue;
            }

            uint64_t end64 = addr64 + len64;
            if (end64 > 0xFFFFFFFFULL) {
                len64 = 0x100000000ULL - addr64;
            }
            struct memory_entry new_entry;
            new_entry.addr = (uint32_t)addr64;
            new_entry.len  = (uint32_t)len64;

            if (new_entry.addr >= 0x100000 && new_entry.len >= 16) {
                if(av_entry_count < MAX_MEM_ENTRIES){
                    memory_entries[av_entry_count++] = new_entry;
                }
            }
        }

        ptr += entry->size + sizeof(entry->size);
    }

    init_allocator(memory_entries, av_entry_count);
}



static memory_block_t* find_prev_for_size(uint32_t needed, memory_block_t** prev_out) {
    memory_block_t* prev = NULL;
    memory_block_t* cur = free_block_list;
    while (cur) {
        if (!block_used(cur) && block_size(cur) >= needed) {
            if (prev_out) *prev_out = prev;
            return cur;
        }
        prev = cur;
        cur = cur->next;
    }
    if (prev_out) *prev_out = prev;
    return NULL;
}

void debug_block_info(void *user_ptr) {
    if (!user_ptr) {
        printf("debug_block_info: user_ptr == NULL\n");
        return;
    }
    memory_block_t *blk = (memory_block_t*)((uint8_t*)user_ptr - sizeof(memory_block_t));
    uint32_t len_field = blk->len;
    uint32_t blocklen = block_size(blk); 
    int used = block_used(blk); 
    vga_refresh();
    halt();
}

// TODO: malloc/block list bugs fixen
void* malloc(uint32_t size) {
    if (size == 0) return NULL;

    uint32_t user_size = ALIGN4(size);
    uint32_t needed = sizeof(memory_block_t) + user_size;

    memory_block_t* prev = NULL;
    memory_block_t* block = find_prev_for_size(needed, &prev);
    if (!block) return NULL;

    uint32_t bsize = block_size(block);
    const uint32_t min_split = sizeof(memory_block_t) + 4; // header + tiny payload

    if (bsize >= needed + min_split) {
        uint8_t* base = (uint8_t*)block;
        memory_block_t* new_block = (memory_block_t*)(base + needed);
        uint32_t new_size = bsize - needed;

        new_block->len = (new_size & ~USED_FLAG);
        new_block->next = block->next;

        if (prev) prev->next = new_block;
        else free_block_list = new_block;

        block->len = (needed | USED_FLAG);
        
    } else {
        if (prev) prev->next = block->next;
        else free_block_list = block->next;
        mark_used(block);
    }

    return (void*)((uint8_t*)block + sizeof(memory_block_t));
}

// TODO: block list bugs zoeken
void free(void* ptr) {
    if (!ptr) return;
    memory_block_t* block = (memory_block_t*)((uint8_t*)ptr - sizeof(memory_block_t));

    if (!block_used(block)) {
       
        printf("WARNING: double free or invalid pointer at %p\n", ptr);
        asm("cli");
        vga_refresh();
        halt();
        return;
    }

    mark_free(block);

    memory_block_t* prev = NULL;
    memory_block_t* cur = free_block_list;
    while (cur && cur < block) {
        prev = cur;
        cur = cur->next;
    }

    block->next = cur;
    if (prev) prev->next = block;
    else free_block_list = block;

    if (block->next && !block_used(block->next)) {
    uint8_t* block_end = (uint8_t*)block + block_size(block);
    if (block_end == (uint8_t*)block->next) {
        uint32_t merged = block_size(block) + block_size(block->next);
        block->len = (merged & ~USED_FLAG);
        block->next = block->next->next;
    }
}
    if (prev && !block_used(prev)) {
    uint8_t* prev_end = (uint8_t*)prev + block_size(prev);
    if (prev_end == (uint8_t*)block) {
        uint32_t merged = block_size(prev) + block_size(block);
        prev->len = (merged & ~USED_FLAG);
        prev->next = block->next;
    }
}
}


void* memcpy(void* dest, const void* src, size_t n)
{
    unsigned char* d = dest;
    const unsigned char* s = src;

    while (n--) {
        *d++ = *s++;
    }

    return dest;
}