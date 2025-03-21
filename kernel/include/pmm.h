/**
 * NKOF Physical Memory Manager
 * 
 * This file contains declarations for the physical memory manager,
 * which keeps track of available physical memory pages.
 */

#ifndef NKOF_PMM_H
#define NKOF_PMM_H

#include "types.h"

// 4KB pages are standard in x86
#define PAGE_SIZE 4096

// Memory region types (compatible with BIOS E820 map)
#define MEMORY_REGION_AVAILABLE      1
#define MEMORY_REGION_RESERVED       2
#define MEMORY_REGION_ACPI_RECLAIMABLE 3
#define MEMORY_REGION_ACPI_NVS       4
#define MEMORY_REGION_BAD            5

// Memory map entry structure (from BIOS)
typedef struct memory_map_entry {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_attributes;
} memory_map_entry_t;

// Initialize the physical memory manager
void pmm_init(memory_map_entry_t* memory_map, uint32_t entry_count);

// Allocate a physical page, returns the physical address
uint32_t pmm_alloc_page(void);

// Free a previously allocated page
void pmm_free_page(uint32_t page_addr);

// Get the total amount of physical memory in bytes
uint64_t pmm_get_total_memory(void);

// Get the amount of free physical memory in bytes
uint64_t pmm_get_free_memory(void);

// Get the amount of used physical memory in bytes
uint64_t pmm_get_used_memory(void);

// Debug function to print memory stats
void pmm_print_stats(void);

// Mark a specific page as used
void pmm_mark_page_used(uint32_t page_addr);

// Check if a specific page is free
bool pmm_is_page_free(uint32_t page_addr);

#endif /* NKOF_PMM_H */