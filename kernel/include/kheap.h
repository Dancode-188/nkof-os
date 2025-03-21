/**
 * NKOF Kernel Heap
 * 
 * This file declares the kernel heap interface for dynamic memory allocation.
 */

#ifndef NKOF_KHEAP_H
#define NKOF_KHEAP_H

#include "types.h"

// Initialize the kernel heap
void kheap_init(void);

// Allocate memory of a specified size
void* kmalloc(size_t size);

// Allocate aligned memory
void* kmalloc_aligned(size_t size, uint32_t alignment);

// Allocate zeroed memory
void* kzalloc(size_t size);

// Free allocated memory
void kfree(void* ptr);

// Reallocate memory to a new size
void* krealloc(void* ptr, size_t size);

// Get heap statistics
void kheap_get_stats(size_t* total, size_t* used, size_t* free);

// Print heap statistics
void kheap_print_stats(void);

#endif /* NKOF_KHEAP_H */