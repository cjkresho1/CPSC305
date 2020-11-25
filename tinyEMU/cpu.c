#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include "memory_system.h"
#include "bit_functions.h"
#include "cpu.h"
#include "isa.h"

static int registers[16];

static int cpsr; // status register

/* For future implementation of breakpoints
 *
#define NUM_BREAKPOINTS 2
static int breakpoints[NUM_BREAKPOINTS]; // allow 2 break points
 */

void set_reg(int reg, int value) {
    registers[reg] =  value;
}

int get_reg(int reg) {
    return registers[reg];
}

int get_cpsr() {
    return cpsr;
}

void show_regs() {
    printf("You need to implement show_regs().\n");
}

void step() {
    int pc = registers[PC];
    int inst; 
    system_bus(pc, &inst, READ);
    printf("You need to implement step().\n");
}

void step_n(int n) {
    printf("You need to implement step_n().\n");
}

void step_show_reg() {
    printf("You need to implement step_show_reg().\n");
}

void step_show_reg_mem() {
    printf("You need to implement step_show_reg_mem().\n");
}

