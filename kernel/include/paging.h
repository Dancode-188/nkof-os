/**
 * NKOF Paging System
 * 
 * This file contains declarations for the paging system which 
 * manages virtual memory through page tables.
 */

#ifndef NKOF_PAGING_H
#define NKOF_PAGING_H

#include "types.h"
#include "pmm.h"

// Page directory and table entry flags
#define PAGE_PRESENT     0x01
#define PAGE_WRITABLE    0x02
#define PAGE_USER        0x04
#define PAGE_WRITETHROUGH 0x08
#define PAGE_CACHE_DISABLE 0x10
#define PAGE_ACCESSED    0x20
#define PAGE_DIRTY       0x40
#define PAGE_SIZE_BIT    0x80    // 4MB page (PDE only)
#define PAGE_GLOBAL      0x100   // Global page (PDE only)

// Page directory entry structure
typedef struct {
    uint32_t entries[1024];
} page_directory_t;

// Page table entry structure
typedef struct {
    uint32_t entries[1024];
} page_table_t;

// Initialize the paging system
void paging_init(void);

// Map a virtual page to a physical page
void paging_map_page(uint32_t virtual_addr, uint32_t physical_addr, uint32_t flags);

// Unmap a virtual page
void paging_unmap_page(uint32_t virtual_addr);

// Get the physical address mapped to a virtual address
uint32_t paging_get_physical_address(uint32_t virtual_addr);

// Check if a virtual page is present
bool paging_is_page_present(uint32_t virtual_addr);

// Allocate a page and map it
uint32_t paging_alloc_and_map(uint32_t virtual_addr, uint32_t flags);

// Handle a page fault
void paging_handle_fault(uint32_t fault_addr, uint32_t error_code);

// Get the current page directory
page_directory_t* paging_get_directory(void);

// Load a new page directory
void paging_load_directory(page_directory_t* directory);

// Flush a specific page from the TLB
void paging_flush_tlb_page(uint32_t virtual_addr);

// Flush the entire TLB
void paging_flush_tlb(void);

#endif /* NKOF_PAGING_H */