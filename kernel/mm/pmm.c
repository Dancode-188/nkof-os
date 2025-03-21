/**
 * NKOF Physical Memory Manager Implementation
 *
 * This file implements functions for managing physical memory pages.
 */

#include "../include/pmm.h"
#include "../include/console.h"

// Bitmap to track free/used pages
// Each bit represents one page (1 = used, 0 = free)
static uint32_t* bitmap = NULL;
static uint32_t bitmap_size = 0;

// Memory statistics
static uint64_t total_memory = 0;
static uint64_t used_memory = 0;
static uint64_t free_memory = 0;

// Number of pages
static uint32_t total_pages = 0;

// The physical address where our kernel ends
// This is defined by the linker
extern uint32_t end;

/**
 * Set a bit in the bitmap
 */
static void bitmap_set(uint32_t bit) {
    bitmap[bit / 32] |= (1 << (bit % 32));
}

/**
 * Clear a bit in the bitmap
 */
static void bitmap_clear(uint32_t bit) {
    bitmap[bit / 32] &= ~(1 << (bit % 32));
}

/**
 * Test if a bit is set in the bitmap
 */
static bool bitmap_test(uint32_t bit) {
    return (bitmap[bit / 32] & (1 << (bit % 32))) != 0;
}

/**
 * Initialize the physical memory manager using the memory map
 */
void pmm_init(memory_map_entry_t* memory_map, uint32_t entry_count) {
    console_write_string("Initializing Physical Memory Manager...\n");
    
    // If we don't have a valid memory map, use default conservative values
    if (memory_map == NULL || entry_count == 0) {
        console_write_string("Warning: No memory map provided. Using conservative defaults.\n");
        
        // Assume a conservative memory size (16MB)
        total_memory = 16 * 1024 * 1024;
        total_pages = total_memory / PAGE_SIZE;
        
        // Place bitmap at 1MB
        bitmap_size = (total_pages + 7) / 8;
        bitmap_size = (bitmap_size + 3) & ~3;
        bitmap = (uint32_t*)0x100000;
        
        // Initialize all as used initially
        for (uint32_t i = 0; i < bitmap_size / 4; i++) {
            bitmap[i] = 0xFFFFFFFF;
        }
        
        // Mark a safe region as free (4MB to 8MB)
        uint32_t safe_start = 4 * 1024 * 1024;
        uint32_t safe_end = 8 * 1024 * 1024;
        uint32_t start_page = safe_start / PAGE_SIZE;
        uint32_t end_page = safe_end / PAGE_SIZE;
        
        for (uint32_t page = start_page; page < end_page; page++) {
            bitmap_clear(page);
        }
        
        // Mark bitmap itself as used
        uint32_t bitmap_pages = (bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE;
        uint32_t bitmap_start_page = ((uint32_t)bitmap) / PAGE_SIZE;
        
        for (uint32_t page = bitmap_start_page; page < bitmap_start_page + bitmap_pages; page++) {
            bitmap_set(page);
        }
        
        // Calculate free and used memory
        free_memory = safe_end - safe_start;
        used_memory = total_memory - free_memory;
        
        console_write_string("Initialized with default memory configuration.\n");
        pmm_print_stats();
        return;
    }
    
    // First, calculate total memory and find largest free block for the bitmap
    uint64_t highest_addr = 0;
    uint64_t largest_free_block = 0;
    uint64_t largest_free_addr = 0;
    
    for (uint32_t i = 0; i < entry_count; i++) {
        memory_map_entry_t* entry = &memory_map[i];
        
        // Find the highest memory address
        uint64_t end_addr = entry->base_addr + entry->length;
        if (end_addr > highest_addr) {
            highest_addr = end_addr;
        }
        
        // Count available memory
        if (entry->type == MEMORY_REGION_AVAILABLE) {
            total_memory += entry->length;
            
            // Find largest free block for bitmap
            if (entry->length > largest_free_block) {
                largest_free_block = entry->length;
                largest_free_addr = entry->base_addr;
            }
        }
    }
    
    // Calculate number of pages
    total_pages = (uint32_t)(highest_addr / PAGE_SIZE);
    if (highest_addr % PAGE_SIZE != 0) {
        total_pages += 1;
    }
    
    // Calculate bitmap size in bytes (1 bit per page)
    bitmap_size = (total_pages + 7) / 8;
    
    // Round up to next 4 bytes for alignment
    bitmap_size = (bitmap_size + 3) & ~3;
    
    // Place bitmap at the start of the largest free memory block
    // Explicit cast to prevent warning about different size
    bitmap = (uint32_t*)((uint32_t)largest_free_addr);
    
    // Initialize bitmap: mark all pages as used initially
    for (uint32_t i = 0; i < bitmap_size / 4; i++) {
        bitmap[i] = 0xFFFFFFFF;
    }
    
    // Mark available regions as free in the bitmap
    for (uint32_t i = 0; i < entry_count; i++) {
        memory_map_entry_t* entry = &memory_map[i];
        
        if (entry->type == MEMORY_REGION_AVAILABLE) {
            uint32_t start_page = (uint32_t)(entry->base_addr / PAGE_SIZE);
            uint32_t end_page = (uint32_t)((entry->base_addr + entry->length) / PAGE_SIZE);
            
            for (uint32_t page = start_page; page < end_page; page++) {
                bitmap_clear(page);
            }
        }
    }
    
    // Mark bitmap itself as used
    uint32_t bitmap_pages = (bitmap_size + PAGE_SIZE - 1) / PAGE_SIZE;
    uint32_t bitmap_start_page = (uint32_t)((uint32_t)bitmap / PAGE_SIZE);
    
    for (uint32_t page = bitmap_start_page; page < bitmap_start_page + bitmap_pages; page++) {
        bitmap_set(page);
    }
    
    // Mark kernel and low memory (0-1MB) as used
    uint32_t kernel_end_page = ((uint32_t)&end + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint32_t page = 0; page < kernel_end_page; page++) {
        bitmap_set(page);
    }
    
    // Calculate free and used memory
    free_memory = 0;
    for (uint32_t i = 0; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            free_memory += PAGE_SIZE;
        }
    }
    used_memory = total_memory - free_memory;
    
    // Print memory stats
    console_write_string("Physical memory manager initialized.\n");
    pmm_print_stats();
}

/**
 * Allocate a physical page
 */
uint32_t pmm_alloc_page(void) {
    // Find a free page
    for (uint32_t i = 0; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            // Mark the page as used
            bitmap_set(i);
            
            // Update stats
            free_memory -= PAGE_SIZE;
            used_memory += PAGE_SIZE;
            
            // Return the physical address
            return i * PAGE_SIZE;
        }
    }
    
    // No free pages available
    console_write_string("ERROR: Out of physical memory!\n");
    return 0;
}

/**
 * Free a physical page
 */
void pmm_free_page(uint32_t page_addr) {
    uint32_t page = page_addr / PAGE_SIZE;
    
    // Check if the page is valid
    if (page >= total_pages) {
        console_write_string("ERROR: Attempted to free invalid page!\n");
        return;
    }
    
    // Check if the page is already free
    if (!bitmap_test(page)) {
        console_write_string("WARNING: Attempted to free already free page!\n");
        return;
    }
    
    // Mark the page as free
    bitmap_clear(page);
    
    // Update stats
    free_memory += PAGE_SIZE;
    used_memory -= PAGE_SIZE;
}

/**
 * Get the total amount of physical memory
 */
uint64_t pmm_get_total_memory(void) {
    return total_memory;
}

/**
 * Get the amount of free physical memory
 */
uint64_t pmm_get_free_memory(void) {
    return free_memory;
}

/**
 * Get the amount of used physical memory
 */
uint64_t pmm_get_used_memory(void) {
    return used_memory;
}

/**
 * Print memory statistics
 */
void pmm_print_stats(void) {
    console_write_string("Memory Statistics:\n");
    
    console_write_string("  Total memory: ");
    console_write_int((int)(total_memory / 1024 / 1024));
    console_write_string(" MB\n");
    
    console_write_string("  Used memory:  ");
    console_write_int((int)(used_memory / 1024 / 1024));
    console_write_string(" MB\n");
    
    console_write_string("  Free memory:  ");
    console_write_int((int)(free_memory / 1024 / 1024));
    console_write_string(" MB\n");
    
    console_write_string("  Total pages:  ");
    console_write_int(total_pages);
    console_write_string("\n");
}

/**
 * Mark a specific page as used
 */
void pmm_mark_page_used(uint32_t page_addr) {
    uint32_t page = page_addr / PAGE_SIZE;
    
    // Check if the page is valid
    if (page >= total_pages) {
        return;
    }
    
    // Check if the page is already marked as used
    if (bitmap_test(page)) {
        return;
    }
    
    // Mark the page as used
    bitmap_set(page);
    
    // Update stats
    free_memory -= PAGE_SIZE;
    used_memory += PAGE_SIZE;
}

/**
 * Check if a specific page is free
 */
bool pmm_is_page_free(uint32_t page_addr) {
    uint32_t page = page_addr / PAGE_SIZE;
    
    // Check if the page is valid
    if (page >= total_pages) {
        return false;
    }
    
    return !bitmap_test(page);
}