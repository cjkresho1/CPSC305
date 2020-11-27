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
    printf("R0:  0x%.8X, R1:  0x%.8X, R2:  0x%.8X, R3:  0x%.8X\n", registers[0], registers[1], registers[2], registers[3]);
    printf("R4:  0x%.8X, R5:  0x%.8X, R6:  0x%.8X, R7:  0x%.8X\n", registers[4], registers[5], registers[6], registers[7]);
    printf("R8:  0x%.8X, R9:  0x%.8X, R10: 0x%.8X, R11: 0x%.8X\n", registers[8], registers[9], registers[10], registers[11]);
    printf("R12: 0x%.8X, R13: 0x%.8X, R14: 0x%.8X, R15: 0x%.8X\n", registers[12], registers[13], registers[14], registers[15]);
}

/**
 * Perform one fetch, decode, execute cycle at PC.
 * If branch, PC = branchAddress, else PC = PC + 4
 */
void step() {
    int pc = registers[PC];
    int inst; 
    system_bus(pc, &inst, READ);
    decoded *dInst = decode(inst);

    switch(dInst->opcode) {
        case LDR:
            //LDR case
        break;
        case STR:
            //STR case
        break;
        case LDX:
            //LDX case
        break;
        case STX:
            //STX case
        break;
        case MOV:
            //MOV
        break;
        case ADD:
            //ADD
        break;
        case SUB:
            //SUB
        break;
        case MUL:
            //MUL
        break;
        case DIV:
            //DIV
        break;
        case AND:
            //AND
        break;
        case ORR:
            //ORR
        break;
        case EOR:
            //EOR
        break;
        case CMP:
            //CMP
        break;
        case B:
            //B
        break;
        default:
            //Default, shouldn't happen
        break;
    }
}

//Preform n steps
void step_n(int n) {
    for (int i = 0; i < n; i++) {
        step();
    }
}

//Step, then show any registers changed
void step_show_reg() {
    printf("You need to implement step_show_reg().\n");
}

//Step, then show any registers or memory that have changed
void step_show_reg_mem() {
    printf("You need to implement step_show_reg_mem().\n");
}

