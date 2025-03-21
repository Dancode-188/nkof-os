/**
 * NKOF Kernel Main
 * 
 * This is the main file for the Neural Kernel Optimization Framework.
 * It contains the entry point (kernel_main) that's called from assembly.
 */

#include "include/types.h"
#include "include/console.h"

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
    
    // Output system information
    console_write_string("System Information:\n");
    console_write_string("- 32-bit Protected Mode\n");
    console_write_string("- Neural resource optimization: Initializing\n");
    console_write_string("- Memory management: Basic\n");
    
    // Initialize kernel subsystems
    // These functions will be implemented as we develop the OS
    // memory_init();
    // interrupts_init();
    // neural_init();
    
    // Main kernel loop - halt the CPU when idle
    while (1) {
        // This will be replaced with proper process scheduling later
        __asm__ volatile("hlt");
    }
}