#!/bin/bash
# NKOF Build Script

# Directory setup
mkdir -p build
mkdir -p boot/stage1
mkdir -p boot/stage2

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building NKOF Operating System...${NC}"

# Ensure stage1 and stage2 bootloader files exist
if [ ! -f boot/stage1/bootloader.asm ]; then
    echo -e "${RED}Error: boot/stage1/bootloader.asm not found${NC}"
    exit 1
fi

if [ ! -f boot/stage2/stage2.asm ]; then
    echo -e "${YELLOW}Creating stage2.asm in the boot/stage2 directory...${NC}"
    # Check if stage2.asm exists in the current directory
    if [ -f stage2.asm ]; then
        cp stage2.asm boot/stage2/stage2.asm
    else
        echo -e "${RED}Error: stage2.asm not found${NC}"
        exit 1
    fi
fi

# Assemble stage 1 bootloader
echo -e "${BLUE}Assembling stage 1 bootloader...${NC}"
nasm -f bin boot/stage1/bootloader.asm -o build/bootloader.bin

# Check if assembly was successful
if [ $? -ne 0 ]; then
    echo -e "${RED}Error: Stage 1 bootloader assembly failed${NC}"
    exit 1
fi

# Assemble stage 2 bootloader
echo -e "${BLUE}Assembling stage 2 bootloader...${NC}"
nasm -f bin boot/stage2/stage2.asm -o build/stage2.bin

# Check if assembly was successful
if [ $? -ne 0 ]; then
    echo -e "${RED}Error: Stage 2 bootloader assembly failed${NC}"
    exit 1
fi

# Check stage2.bin file size
STAGE2_SIZE=$(stat -c%s "build/stage2.bin" 2>/dev/null || echo "0")
echo "Stage 2 bootloader size: $STAGE2_SIZE bytes"
if [ "$STAGE2_SIZE" -eq "0" ]; then
    echo -e "${RED}Error: Stage 2 bootloader is empty${NC}"
    exit 1
fi

# Compile the kernel
echo -e "${BLUE}Compiling kernel...${NC}"

# Assemble kernel entry
echo "Assembling kernel entry..."
nasm -f elf32 kernel/arch/x86_64/entry.asm -o build/kernel_entry.o

# Compile C files
echo "Compiling kernel C files..."
gcc -m32 -c kernel/kernel.c -o build/kernel.o -ffreestanding -O2 -Wall -Wextra
gcc -m32 -c kernel/console.c -o build/console.o -ffreestanding -O2 -Wall -Wextra

# Link the kernel
echo "Linking kernel..."
ld -m elf_i386 -T kernel/kernel.ld -o build/kernel.bin build/kernel_entry.o build/kernel.o build/console.o -nostdlib

# Check if kernel compilation was successful
if [ $? -ne 0 ]; then
    echo -e "${RED}Error: Kernel compilation failed${NC}"
    exit 1
fi

# Check if boot.img exists, create it if it doesn't
if [ ! -f build/boot.img ]; then
    echo "Creating new disk image..."
    # Create a 10MB disk image (20160 sectors of 512 bytes)
    dd if=/dev/zero of=build/boot.img bs=512 count=20160
fi

# Write bootloader to the disk image
echo -e "${BLUE}Writing stage 1 bootloader to disk image...${NC}"
dd if=build/bootloader.bin of=build/boot.img bs=512 count=1 conv=notrunc

# Check if write was successful
if [ $? -ne 0 ]; then
    echo -e "${RED}Error: Failed to write stage 1 bootloader to disk image${NC}"
    exit 1
fi

# Write stage 2 bootloader to the disk image (at sector 1, right after the boot sector)
echo -e "${BLUE}Writing stage 2 bootloader to disk image...${NC}"
dd if=build/stage2.bin of=build/boot.img bs=512 seek=1 conv=notrunc

# Check if write was successful
if [ $? -ne 0 ]; then
    echo -e "${RED}Error: Failed to write stage 2 bootloader to disk image${NC}"
    exit 1
fi

# Write kernel to the disk image (at sector 5)
echo -e "${BLUE}Writing kernel to disk image...${NC}"
dd if=build/kernel.bin of=build/boot.img bs=512 seek=5 conv=notrunc

# Check if write was successful
if [ $? -ne 0 ]; then
    echo -e "${RED}Error: Failed to write kernel to disk image${NC}"
    exit 1
fi

echo -e "${GREEN}Build completed successfully!${NC}"
echo "To run in Bochs, use: bochs -f bochsrc.txt"