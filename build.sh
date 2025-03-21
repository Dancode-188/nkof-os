#!/bin/bash
# NKOF Build Script

# Directory setup
mkdir -p build

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building NKOF Operating System...${NC}"

# Assemble stage 1 bootloader
echo "Assembling bootloader..."
nasm -f bin boot/stage1/bootloader.asm -o build/bootloader.bin

# Check if assembly was successful
if [ $? -ne 0 ]; then
    echo -e "${RED}Error: Bootloader assembly failed${NC}"
    exit 1
fi

# Check if boot.img exists, create it if it doesn't
if [ ! -f build/boot.img ]; then
    echo "Creating new disk image..."
    # Create a 10MB disk image (20160 sectors of 512 bytes)
    dd if=/dev/zero of=build/boot.img bs=512 count=20160
fi

# Write bootloader to the disk image
echo "Writing bootloader to disk image..."
dd if=build/bootloader.bin of=build/boot.img bs=512 count=1 conv=notrunc

# Check if write was successful
if [ $? -ne 0 ]; then
    echo -e "${RED}Error: Failed to write bootloader to disk image${NC}"
    exit 1
fi

echo -e "${GREEN}Build completed successfully!${NC}"
echo "To run in Bochs, use: bochs -f bochsrc.txt"