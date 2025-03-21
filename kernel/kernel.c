/**
 * NKOF Kernel Main
 * 
 * This is the main file for the Neural Kernel Optimization Framework.
 * It contains the entry point (kernel_main) that's called from assembly.
 */

#include "include/types.h"
#include "include/console.h"
#include "include/pmm.h"
#include "include/paging.h"
#include "include/kheap.h"

// Memory map passed from bootloader
extern memory_map_entry_t* boot_memory_map;
extern uint32_t boot_memory_map_count;

/**
 * Initialize memory subsystems
 */
static void memory_init(void) {
    // Initialize physical memory manager with boot memory map
    pmm_init(boot_memory_map, boot_memory_map_count);
    
    // Initialize paging system
    paging_init();
    
    // Initialize kernel heap for dynamic memory allocation
    kheap_init();
}

/**
 * Main kernel function - entry point from assembly
 */
void kernel_main(void) {
    // Initialize the console for output
    console_init();
    
    // Display welcome message
    console_write_string("Neural Kernel Optimization Framework (NKOF)\n");
    console_write_string("---------------------------------------\n");
    console_write_string("Kernel initialized successfully!\n\n");
    
    // Initialize memory management subsystems
    memory_init();
    
    // Output system information
    console_write_string("\nSystem Information:\n");
    console_write_string("- 32-bit Protected Mode\n");
    console_write_string("- Paging enabled\n");
    console_write_string("- Neural resource optimization: Initializing\n");
    
    // Initialize kernel subsystems
    // These functions will be implemented as we develop the OS
    // interrupts_init();
    // neural_init();
    
    // Perform a test allocation to verify the heap
    console_write_string("\nPerforming test heap allocations:\n");
    void* test_ptr1 = kmalloc(1024);
    void* test_ptr2 = kmalloc(2048);
    
    console_write_string("Allocated 1024 bytes at: ");
    console_write_hex((uint32_t)test_ptr1);
    console_write_string("\n");
    
    console_write_string("Allocated 2048 bytes at: ");
    console_write_hex((uint32_t)test_ptr2);
    console_write_string("\n");
    
    kfree(test_ptr1);
    console_write_string("Freed first allocation\n");
    
    kheap_print_stats();
    
    // Main kernel loop - halt the CPU when idle
    console_write_string("\nKernel initialized and running.\n");
    while (1) {
        // This will be replaced with proper process scheduling later
        __asm__ volatile("hlt");
    }
}