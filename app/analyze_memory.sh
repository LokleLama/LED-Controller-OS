#!/bin/bash
# Memory analysis script for RP2040

ELF_FILE="$1"
PROGRAM_FLASH_SIZE="$2"

# RP2040 has 264 KB of RAM
TOTAL_RAM=270336

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║              MEMORY LAYOUT ANALYSIS                        ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""

# ELF Section Sizes
echo "=== ELF Section Sizes ==="
arm-none-eabi-size -A -d "$ELF_FILE"
echo ""

# Flash Layout
echo "=== Flash Layout ==="
echo "Total Flash:    2097152 bytes (2048 KB)"
echo "Program Space:  ${PROGRAM_FLASH_SIZE} bytes (1792 KB)"
echo "Reserved SPFS:  262144 bytes (256 KB)"
echo ""

# RAM Memory Layout
echo "=== RAM Memory Layout (RP2040: 264 KB total) ==="
arm-none-eabi-nm --size-sort -S --radix=d "$ELF_FILE" | awk -v total_ram=$TOTAL_RAM '
BEGIN {
    bss = 0
    data = 0
}
/\.bss/ {
    bss += $2
}
/\.data/ {
    data += $2
}
END {
    heap_stack = total_ram - bss - data
    ram_used = bss + data
    ram_percent = (ram_used * 100) / total_ram
    
    printf "Static .data:   %8d bytes (%6.2f KB) - Initialized data\n", data, data/1024
    printf "Static .bss:    %8d bytes (%6.2f KB) - Uninitialized data\n", bss, bss/1024
    printf "Heap + Stack:   %8d bytes (%6.2f KB) - Dynamic allocation\n", heap_stack, heap_stack/1024
    printf "────────────────────────────────────────────────\n"
    printf "Total Static:   %8d bytes (%6.2f KB) [%.1f%%]\n", ram_used, ram_used/1024, ram_percent
    printf "RAM Available:  %8d bytes (%6.2f KB) [%.1f%%]\n", heap_stack, heap_stack/1024, 100-ram_percent
}'

echo ""
echo "=== Top 10 Largest Static Objects in RAM ==="
arm-none-eabi-nm --size-sort -S --radix=d "$ELF_FILE" | \
    grep -E ' [bBdD] ' | \
    sort -rn -k2 | \
    head -10 | \
    awk '{printf "  %8d bytes  %s\n", $2, $3}'

echo ""
echo "=== Stack Configuration ==="
STACK_SIZE=$(arm-none-eabi-objdump -h "$ELF_FILE" | grep -E '\.stack' | awk '{print strtonum("0x"$3)}')
if [ -z "$STACK_SIZE" ]; then
    STACK_SIZE=$(arm-none-eabi-readelf -S "$ELF_FILE" | grep -E '\.stack' | awk '{print strtonum("0x"$6)}')
fi
if [ -z "$STACK_SIZE" ]; then
    echo "Stack size: Check linker script (typically 2048 bytes)"
else
    printf "Stack size: %d bytes (%.2f KB)\n" $STACK_SIZE $(echo "$STACK_SIZE/1024" | bc -l)
fi

echo ""
echo "=== Flash Usage Summary ==="
arm-none-eabi-size -B -d "$ELF_FILE" | tail -n 1 | awk -v total_flash=$PROGRAM_FLASH_SIZE '{
    text = $1
    data = $2
    bss = $3
    used = text + data
    percent = (used * 100) / total_flash
    
    printf "Flash Used:     %8d bytes (%6.2f KB) [%.1f%%]\n", used, used/1024, percent
    printf "Flash Free:     %8d bytes (%6.2f KB) [%.1f%%]\n", total_flash-used, (total_flash-used)/1024, 100-percent
}'

echo ""
echo "╔════════════════════════════════════════════════════════════╗"
echo "║  Note: Heap size is dynamic and shares space with stack   ║"
echo "║  Actual heap/stack split depends on runtime usage         ║"
echo "╚════════════════════════════════════════════════════════════╝"
echo ""
