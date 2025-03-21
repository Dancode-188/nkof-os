/**
 * NKOF Basic Type Definitions
 * 
 * This file contains type definitions for the kernel to ensure
 * consistent data types across different platforms.
 */

#ifndef NKOF_TYPES_H
#define NKOF_TYPES_H

// Standard type definitions to ensure consistent sizes
typedef unsigned char      uint8_t;    // 8-bit unsigned
typedef unsigned short     uint16_t;   // 16-bit unsigned
typedef unsigned int       uint32_t;   // 32-bit unsigned
typedef unsigned long long uint64_t;   // 64-bit unsigned

typedef signed char        int8_t;     // 8-bit signed
typedef signed short       int16_t;    // 16-bit signed
typedef signed int         int32_t;    // 32-bit signed
typedef signed long long   int64_t;    // 64-bit signed

// Size type typically used for memory sizes and offsets
typedef uint32_t           size_t;

// Boolean type
typedef enum {
    false = 0,
    true = 1
} bool;

// NULL pointer definition
#define NULL ((void*)0)

#endif /* NKOF_TYPES_H */