/**
 * NKOF Kernel Heap Implementation
 *
 * This file implements a simple heap for dynamic memory allocation in the kernel.
 * It uses a linked list of free blocks with first-fit allocation strategy.
 */

#include "../include/kheap.h"
#include "../include/pmm.h"
#include "../include/paging.h"
#include "../include/console.h"

// Memory block header
typedef struct block_header {
    size_t size;                   // Size of the block (including header)
    uint32_t magic;                // Magic number for integrity checking
    uint8_t is_free;               // 1 if the block is free, 0 if allocated
    struct block_header* next;     // Pointer to the next block
} block_header_t;

// Magic number for block headers
#define HEAP_MAGIC 0x1BADB002

// Minimum block size (including header)
#define MIN_BLOCK_SIZE (sizeof(block_header_t) + 4)

// Heap statistics
static size_t heap_total = 0;
static size_t heap_used = 0;
static size_t heap_free = 0;

// Start and end of the heap
static uint32_t heap_start = 0;
static uint32_t heap_end = 0;
static uint32_t heap_max = 0;

// First block in the heap
static block_header_t* first_block = NULL;

/**
 * Expand the heap by a given number of pages
 */
static void expand_heap(size_t pages) {
    if (heap_end + pages * PAGE_SIZE > heap_max) {
        console_write_string("ERROR: Cannot expand heap beyond maximum limit\n");
        return;
    }
    
    // Map new pages
    for (uint32_t addr = heap_end; addr < heap_end + pages * PAGE_SIZE; addr += PAGE_SIZE) {
        paging_alloc_and_map(addr, PAGE_PRESENT | PAGE_WRITABLE);
    }
    
    // Update heap_end
    uint32_t old_end = heap_end;
    heap_end += pages * PAGE_SIZE;
    
    // Create a new free block for the expanded region
    block_header_t* new_block = (block_header_t*)old_end;
    new_block->size = pages * PAGE_SIZE;
    new_block->magic = HEAP_MAGIC;
    new_block->is_free = 1;
    
    // Update heap statistics
    heap_total += pages * PAGE_SIZE;
    heap_free += pages * PAGE_SIZE;
    
    // Insert the new block into the linked list
    block_header_t* current = first_block;
    if (!current) {
        // If this is the first block
        first_block = new_block;
        new_block->next = NULL;
        return;
    }
    
    // Find the last block
    while (current->next) {
        current = current->next;
    }
    
    current->next = new_block;
    new_block->next = NULL;
    
    // Try to merge with the previous block if it's free
    if (current->is_free) {
        current->size += new_block->size;
        current->next = NULL;
    }
}

/**
 * Split a block into two parts
 */
static void split_block(block_header_t* block, size_t size) {
    // Check if the block is large enough to split
    if (block->size < size + MIN_BLOCK_SIZE) {
        return;
    }
    
    // Create a new block after the current one
    block_header_t* new_block = (block_header_t*)((uint32_t)block + size);
    new_block->size = block->size - size;
    new_block->magic = HEAP_MAGIC;
    new_block->is_free = 1;
    new_block->next = block->next;
    
    // Update the current block
    block->size = size;
    block->next = new_block;
    
    // Update heap statistics
    heap_free += sizeof(block_header_t);
}

/**
 * Initialize the kernel heap
 */
void kheap_init(void) {
    console_write_string("Initializing kernel heap...\n");
    
    // Set heap boundaries (4MB initial, 16MB maximum)
    heap_start = 0x400000;  // 4MB (above identity-mapped kernel area)
    heap_end = heap_start;
    heap_max = 0x1000000;   // 16MB
    
    // Start with no blocks
    first_block = NULL;
    
    // Expand the initial heap
    expand_heap(16);  // 16 pages = 64KB initial heap
    
    console_write_string("Kernel heap initialized.\n");
    kheap_print_stats();
}

/**
 * Allocate memory of a specified size
 */
void* kmalloc(size_t size) {
    // Adjust size to include header and ensure minimum size
    size_t total_size = size + sizeof(block_header_t);
    if (total_size < MIN_BLOCK_SIZE) {
        total_size = MIN_BLOCK_SIZE;
    }
    
    // Align to 4 bytes
    total_size = (total_size + 3) & ~3;
    
    // Find a suitable free block
    block_header_t* current = first_block;
    block_header_t* best_fit = NULL;
    
    while (current) {
        // Check magic number
        if (current->magic != HEAP_MAGIC) {
            console_write_string("ERROR: Heap corruption detected\n");
            return NULL;
        }
        
        // Check if the block is free and large enough
        if (current->is_free && current->size >= total_size) {
            best_fit = current;
            break;
        }
        
        current = current->next;
    }
    
    // If no suitable block was found, expand the heap
    if (!best_fit) {
        // Calculate how many pages we need
        size_t pages = (total_size + PAGE_SIZE - 1) / PAGE_SIZE;
        
        // Expand the heap
        expand_heap(pages);
        
        // Try allocation again
        return kmalloc(size);
    }
    
    // Split the block if it's too large
    split_block(best_fit, total_size);
    
    // Mark the block as allocated
    best_fit->is_free = 0;
    
    // Update heap statistics
    heap_used += best_fit->size;
    heap_free -= best_fit->size;
    
    // Return a pointer to the data section
    return (void*)((uint32_t)best_fit + sizeof(block_header_t));
}

/**
 * Allocate aligned memory
 */
void* kmalloc_aligned(size_t size, uint32_t alignment) {
    // Ensure alignment is a power of 2
    if (alignment & (alignment - 1)) {
        return NULL;
    }
    
    // Allocate more memory to ensure alignment can be satisfied
    size_t padding = alignment + sizeof(void*);
    uint8_t* raw_mem = (uint8_t*)kmalloc(size + padding);
    
    if (!raw_mem) {
        return NULL;
    }
    
    // Calculate aligned address
    uint8_t* aligned_mem = (uint8_t*)(((uint32_t)raw_mem + sizeof(void*) + alignment - 1) & ~(alignment - 1));
    
    // Store the original pointer just before the aligned memory
    *((uint8_t**)aligned_mem - 1) = raw_mem;
    
    return aligned_mem;
}

/**
 * Allocate zeroed memory
 */
void* kzalloc(size_t size) {
    void* ptr = kmalloc(size);
    
    if (ptr) {
        // Zero out the allocated memory
        uint8_t* byte_ptr = (uint8_t*)ptr;
        for (size_t i = 0; i < size; i++) {
            byte_ptr[i] = 0;
        }
    }
    
    return ptr;
}

/**
 * Merge adjacent free blocks
 */
static void merge_free_blocks(void) {
    block_header_t* current = first_block;
    
    while (current && current->next) {
        // Check magic numbers
        if (current->magic != HEAP_MAGIC || current->next->magic != HEAP_MAGIC) {
            console_write_string("ERROR: Heap corruption detected during merge\n");
            return;
        }
        
        // If both blocks are free, merge them
        if (current->is_free && current->next->is_free) {
            current->size += current->next->size;
            current->next = current->next->next;
            continue;  // Check if we can merge again
        }
        
        current = current->next;
    }
}

/**
 * Free allocated memory
 */
void kfree(void* ptr) {
    if (!ptr) {
        return;
    }
    
    // Get the block header
    block_header_t* block = (block_header_t*)((uint32_t)ptr - sizeof(block_header_t));
    
    // Check magic number
    if (block->magic != HEAP_MAGIC) {
        console_write_string("ERROR: Attempt to free invalid memory block\n");
        return;
    }
    
    // Check if the block is already free
    if (block->is_free) {
        console_write_string("WARNING: Attempt to free already freed memory\n");
        return;
    }
    
    // Mark the block as free
    block->is_free = 1;
    
    // Update heap statistics
    heap_used -= block->size;
    heap_free += block->size;
    
    // Merge adjacent free blocks
    merge_free_blocks();
}

/**
 * Free aligned memory
 */
static void kfree_aligned(void* ptr) {
    if (!ptr) {
        return;
    }
    
    // Get the original pointer
    void* original = *((void**)ptr - 1);
    
    // Free the original allocation
    kfree(original);
}

/**
 * Reallocate memory to a new size
 */
void* krealloc(void* ptr, size_t size) {
    if (!ptr) {
        return kmalloc(size);
    }
    
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    // Get the block header
    block_header_t* block = (block_header_t*)((uint32_t)ptr - sizeof(block_header_t));
    
    // Check magic number
    if (block->magic != HEAP_MAGIC) {
        console_write_string("ERROR: Attempt to reallocate invalid memory block\n");
        return NULL;
    }
    
    // Calculate usable size in the current block
    size_t current_size = block->size - sizeof(block_header_t);
    
    // If the new size is smaller, we can just return the same block
    if (size <= current_size) {
        // We could split the block here, but for simplicity we'll skip that
        return ptr;
    }
    
    // Need to allocate a larger block
    void* new_ptr = kmalloc(size);
    if (!new_ptr) {
        return NULL;
    }
    
    // Copy data from the old block to the new one
    uint8_t* src = (uint8_t*)ptr;
    uint8_t* dst = (uint8_t*)new_ptr;
    for (size_t i = 0; i < current_size; i++) {
        dst[i] = src[i];
    }
    
    // Free the old block
    kfree(ptr);
    
    return new_ptr;
}

/**
 * Get heap statistics
 */
void kheap_get_stats(size_t* total, size_t* used, size_t* free) {
    if (total) *total = heap_total;
    if (used) *used = heap_used;
    if (free) *free = heap_free;
}

/**
 * Print heap statistics
 */
void kheap_print_stats(void) {
    console_write_string("Kernel Heap Statistics:\n");
    
    console_write_string("  Total heap size: ");
    console_write_int((int)(heap_total / 1024));
    console_write_string(" KB\n");
    
    console_write_string("  Used heap size:  ");
    console_write_int((int)(heap_used / 1024));
    console_write_string(" KB\n");
    
    console_write_string("  Free heap size:  ");
    console_write_int((int)(heap_free / 1024));
    console_write_string(" KB\n");
}