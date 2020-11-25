#include <stdio.h>
#include "memory_system.h"
#include "memory.h"

#define DECODER_BITMASK 0x1f
#define DECODER_SHIFTER 5

/**
 * Converts a number into its bit position value
 * Examples:
 *  2 is converted to 0b00000000000000000000000000000010
 *  7 is converted to 0b00000000000000000000000001000000
 * 31 is converted to 0b10000000000000000000000000000000
 * Since it returns an int, this function works with 5 bits numbers
 */ 

unsigned int decoder(unsigned char b) {
    
    return 1 << (b & DECODER_BITMASK);

}

void split_address(int address, unsigned char *lower, unsigned char *upper) {
    *lower = address & DECODER_BITMASK;
    *upper = (address & (DECODER_BITMASK << DECODER_SHIFTER)) >> DECODER_SHIFTER;
}

void memory_store(int address, unsigned char value) {
    
    unsigned char lower, upper;        
    split_address(address, &lower, &upper);
    unsigned int lower_decoded = decoder(lower);
    unsigned int upper_decoded = decoder(upper);
    //printf("HERE1 %d %d %d %d\n", upper, upper_decoded, lower, lower_decoded);
    mem_put(upper_decoded, lower_decoded, value);
    //printf("HERE2\n");

}

void memory_store_word(int address, unsigned int value) {
    int byte_pos = 24;
    for (int i = address; i < address+4; i++) {
        unsigned char val = (value >> byte_pos) & 0xff;
        memory_store(i, val);
        byte_pos -= 8;
    }
}

unsigned char memory_fetch(int address) {
    unsigned char lower, upper;
    split_address(address, &lower, &upper);
    unsigned int lower_decoded = decoder(lower);
    unsigned int upper_decoded = decoder(upper);
    return mem_get(upper_decoded, lower_decoded);
    
}

unsigned int memory_fetch_word(int address) {
    unsigned int value = 0;
    for (int i = address; i < address+4; i++) {
        unsigned char val = memory_fetch(i);
        value = (value << 8) | val;
    }
    return value;
}

void system_bus(int address, int *value, int control) {
    if (control == READ)
        *value = memory_fetch_word(address);
    else
        memory_store_word(address, *value);
}

void dump_memory(int start_address, int num_bytes) {
    int start_boundary = start_address - (start_address % 8);
    int boundary_bytes = num_bytes + (start_address - start_boundary);
    printf("start_boundary: %d 0x%04x\n", start_boundary, start_boundary);
    for (int i = start_boundary; i < start_boundary+boundary_bytes; i+=8) {
        printf("0x%04x (0d%04d) 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \n", 
               i, i,
               memory_fetch(i),   memory_fetch(i+1), memory_fetch(i+2), memory_fetch(i+3),
               memory_fetch(i+4), memory_fetch(i+5), memory_fetch(i+6), memory_fetch(i+7));
    }
}

/**
 * First line is memory address.
 * Subsequent lines are 32 bit quantities
 * All lines are hex numbers
 */
void load_memory(char *filename) {
    FILE *fp = fopen(filename, "r");
    int address;
    fscanf(fp, "%x", &address);
    int mem_value;
    while (fscanf(fp, "%x", &mem_value) != EOF) {
        memory_store_word(address, mem_value);
        address += 4;
    }
    fclose(fp);
}
