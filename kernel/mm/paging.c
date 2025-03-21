/**
 * NKOF Paging Implementation
 *
 * This file implements the paging system for virtual memory management.
 */

#include "../include/paging.h"
#include "../include/pmm.h"
#include "../include/console.h"

// Current page directory
static page_directory_t* current_directory = NULL;

// Recursive mapping index (maps the page directory to itself)
#define RECURSIVE_INDEX 1023

/**
 * Enable paging on the CPU
 */
static void enable_paging(page_directory_t* directory) {
    // Load page directory
    asm volatile (
        "mov %0, %%cr3" : : "r" (directory)
    );
    
    // Enable paging by setting the paging bit in CR0
    uint32_t cr0;
    asm volatile (
        "mov %%cr0, %0" : "=r" (cr0)
    );
    cr0 |= 0x80000000; // Set PG bit
    asm volatile (
        "mov %0, %%cr0" : : "r" (cr0)
    );
}

/**
 * Create a page directory
 */
static page_directory_t* create_page_directory(void) {
    // Allocate a page for the directory
    uint32_t phys_addr = pmm_alloc_page();
    page_directory_t* dir = (page_directory_t*)phys_addr;
    
    // Clear the directory
    for (int i = 0; i < 1024; i++) {
        dir->entries[i] = 0;
    }
    
    return dir;
}

/**
 * Get a page table, creating it if it doesn't exist
 */
static page_table_t* get_or_create_page_table(page_directory_t* dir, uint32_t virtual_addr, bool create) {
    uint32_t pd_index = (virtual_addr >> 22) & 0x3FF;
    
    // Check if the page table exists
    if (!(dir->entries[pd_index] & PAGE_PRESENT)) {
        if (!create) {
            return NULL;
        }
        
        // Create a new page table
        uint32_t pt_phys = pmm_alloc_page();
        dir->entries[pd_index] = pt_phys | PAGE_PRESENT | PAGE_WRITABLE;
        
        // Clear the new page table
        page_table_t* pt = (page_table_t*)pt_phys;
        for (int i = 0; i < 1024; i++) {
            pt->entries[i] = 0;
        }
    }
    
    // Return the page table
    return (page_table_t*)(dir->entries[pd_index] & 0xFFFFF000);
}

/**
 * Initialize the paging system
 */
void paging_init(void) {
    console_write_string("Initializing paging...\n");
    
    // Create kernel page directory
    page_directory_t* kernel_dir = create_page_directory();
    
    // Identity map the first 4MB (kernel space)
    for (uint32_t addr = 0; addr < 0x400000; addr += PAGE_SIZE) {
        paging_map_page(addr, addr, PAGE_PRESENT | PAGE_WRITABLE);
    }
    
    // Set up recursive mapping for easier page table manipulation
    // This maps the page directory to itself at RECURSIVE_INDEX
    kernel_dir->entries[RECURSIVE_INDEX] = (uint32_t)kernel_dir | PAGE_PRESENT | PAGE_WRITABLE;
    
    // Save current directory
    current_directory = kernel_dir;
    
    // Enable paging
    enable_paging(kernel_dir);
    
    console_write_string("Paging initialized.\n");
}

/**
 * Map a virtual page to a physical page
 */
void paging_map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags) {
    // Align addresses to page boundaries
    virtual_addr &= 0xFFFFF000;
    physical_addr &= 0xFFFFF000;
    
    // Get directory indices
    uint32_t pd_index = (virtual_addr >> 22) & 0x3FF;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;
    
    // Get page table (create if necessary)
    page_table_t* pt = get_or_create_page_table(current_directory, virtual_addr, true);
    
    // Map page
    pt->entries[pt_index] = physical_addr | flags;
    
    // Flush TLB for this page
    paging_flush_tlb_page(virtual_addr);
}

/**
 * Unmap a virtual page
 */
void paging_unmap_page(uint32_t virtual_addr) {
    // Align address to page boundary
    virtual_addr &= 0xFFFFF000;
    
    // Get directory indices
    uint32_t pd_index = (virtual_addr >> 22) & 0x3FF;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;
    
    // Get page table (if it exists)
    page_table_t* pt = get_or_create_page_table(current_directory, virtual_addr, false);
    if (pt) {
        // Unmap page
        pt->entries[pt_index] = 0;
        
        // Flush TLB for this page
        paging_flush_tlb_page(virtual_addr);
    }
}

/**
 * Get the physical address mapped to a virtual address
 */
uint32_t paging_get_physical_address(uint32_t virtual_addr) {
    // Get directory indices
    uint32_t pd_index = (virtual_addr >> 22) & 0x3FF;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;
    uint32_t offset = virtual_addr & 0xFFF;
    
    // Get page table (if it exists)
    page_table_t* pt = get_or_create_page_table(current_directory, virtual_addr, false);
    if (!pt || !(pt->entries[pt_index] & PAGE_PRESENT)) {
        return 0;
    }
    
    // Get physical address
    uint32_t physical_addr = pt->entries[pt_index] & 0xFFFFF000;
    return physical_addr + offset;
}

/**
 * Check if a virtual page is present
 */
bool paging_is_page_present(uint32_t virtual_addr) {
    // Get directory indices
    uint32_t pd_index = (virtual_addr >> 22) & 0x3FF;
    uint32_t pt_index = (virtual_addr >> 12) & 0x3FF;
    
    // Get page table (if it exists)
    page_table_t* pt = get_or_create_page_table(current_directory, virtual_addr, false);
    if (!pt) {
        return false;
    }
    
    // Check if page is present
    return (pt->entries[pt_index] & PAGE_PRESENT) != 0;
}

/**
 * Allocate a page and map it
 */
uint32_t paging_alloc_and_map(uint32_t virtual_addr, uint32_t flags) {
    // Align address to page boundary
    virtual_addr &= 0xFFFFF000;
    
    // Allocate a physical page
    uint32_t physical_addr = pmm_alloc_page();
    if (physical_addr == 0) {
        return 0;  // Out of memory
    }
    
    // Map the page
    paging_map_page(virtual_addr, physical_addr, flags);
    
    return virtual_addr;
}

/**
 * Handle a page fault
 */
void paging_handle_fault(uint32_t fault_addr, uint32_t error_code) {
    // Print fault information
    console_write_string("Page fault at address: ");
    console_write_hex(fault_addr);
    console_write_string("\nError code: ");
    console_write_hex(error_code);
    console_write_string("\n");
    
    // Decode error code
    console_write_string("Fault details: ");
    if (!(error_code & 0x1)) {
        console_write_string("Page not present, ");
    }
    if (error_code & 0x2) {
        console_write_string("Write operation, ");
    } else {
        console_write_string("Read operation, ");
    }
    if (error_code & 0x4) {
        console_write_string("User mode, ");
    } else {
        console_write_string("Kernel mode, ");
    }
    if (error_code & 0x8) {
        console_write_string("Reserved bits overwritten, ");
    }
    if (error_code & 0x10) {
        console_write_string("Instruction fetch");
    }
    console_write_string("\n");
    
    // Halt the system on unhandled page fault
    console_write_string("System halted due to unhandled page fault.\n");
    for (;;) {
        asm volatile ("hlt");
    }
}

/**
 * Get the current page directory
 */
page_directory_t* paging_get_directory(void) {
    return current_directory;
}

/**
 * Load a new page directory
 */
void paging_load_directory(page_directory_t* directory) {
    current_directory = directory;
    asm volatile (
        "mov %0, %%cr3" : : "r" (directory)
    );
}

/**
 * Flush a specific page from the TLB
 */
void paging_flush_tlb_page(uint32_t virtual_addr) {
    asm volatile (
        "invlpg (%0)" : : "r" (virtual_addr) : "memory"
    );
}

/**
 * Flush the entire TLB
 */
void paging_flush_tlb(void) {
    uint32_t cr3;
    asm volatile (
        "mov %%cr3, %0" : "=r" (cr3)
    );
    asm volatile (
        "mov %0, %%cr3" : : "r" (cr3)
    );
}