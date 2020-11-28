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
    printf("PC: 0x%.8X, inst: 0x%.8X, %s\n", registers[PC], inst, disassemble(dInst));
    registers[PC] = registers[PC] + 4;
    int tempLdr, tempStr, tempLdx, tempStx, tempCmp, tempCpsr;

    switch(dInst->opcode) {
        case LDR:
            if (dInst->rd > 15 || dInst->rd < 0) {
                exit(1);
            }
            if (dInst->address > 1023 || dInst->address < 0) {
                exit(1);
            }
            system_bus(dInst->address, &tempLdr, READ);
            registers[dInst->rd] = tempLdr;
        break;
        case STR:
            if (dInst->rd > 15 || dInst->rd < 0) {
                exit(1);
            }
            if (dInst->address > 1023 || dInst->address < 0) {
                exit(1);
            }
            tempStr = registers[dInst->rd];
            system_bus(dInst->address, &tempStr, WRITE);
        break;
        case LDX:
            if (dInst->rd > 15 || dInst->rd < 0) {
                exit(1);
            }
            if (dInst->rn > 15 || dInst->rn < 0) {
                exit(1);
            }
            if ((registers[dInst->rn] + dInst->offset) > 1023 || (registers[dInst->rn] + dInst->offset) < 0) {
                exit(1);
            }
            system_bus(registers[dInst->rn] + dInst->offset, &tempLdx, READ);
            registers[dInst->rd] = tempLdx;
        break;
        case STX:
            if (dInst->rd > 15 || dInst->rd < 0) {
                exit(1);
            }
            if (dInst->rn > 15 || dInst->rn < 0) {
                exit(1);
            }
            if ((registers[dInst->rn] + dInst->offset) > 1023 || (registers[dInst->rn] + dInst->offset) < 0) {
                exit(1);
            }
            tempStx = registers[dInst->rd];
            system_bus(registers[dInst->rn] + dInst->offset, &tempStx, WRITE);
        break;
        case MOV:
            if (dInst->flag == 1) {
                if (dInst->rd > 15 || dInst->rd < 0) {
                    exit(1);
                }
                if (dInst->rn > 15 || dInst->rn < 0) {
                    exit(1);
                }
                registers[dInst->rd] = registers[dInst->rn];
            }
            else {
                if (dInst->rd > 15 || dInst->rd < 0) {
                    exit(1);
                }
                registers[dInst->rd] = dInst->immediate;
            }
        break;
        case ADD:
            if (dInst->rd > 15 || dInst->rd < 0) {
                exit(1);
            }
            if (dInst->rn > 15 || dInst->rn < 0) {
                exit(1);
            }
            if (dInst->rm > 15 || dInst->rm < 0) {
                exit(1);
            }
            registers[dInst->rd] = registers[dInst->rm] + registers[dInst->rn];
        break;
        case SUB:
            if (dInst->rd > 15 || dInst->rd < 0) {
                exit(1);
            }
            if (dInst->rn > 15 || dInst->rn < 0) {
                exit(1);
            }
            if (dInst->rm > 15 || dInst->rm < 0) {
                exit(1);
            }
            registers[dInst->rd] = registers[dInst->rm] - registers[dInst->rn];
        break;
        case MUL:
            if (dInst->rd > 15 || dInst->rd < 0) {
                exit(1);
            }
            if (dInst->rn > 15 || dInst->rn < 0) {
                exit(1);
            }
            if (dInst->rm > 15 || dInst->rm < 0) {
                exit(1);
            }
            registers[dInst->rd] = registers[dInst->rm] * registers[dInst->rn];
        break;
        case DIV:
            if (dInst->rd > 15 || dInst->rd < 0) {
                exit(1);
            }
            if (dInst->rn > 15 || dInst->rn < 0) {
                exit(1);
            }
            if (dInst->rm > 15 || dInst->rm < 0) {
                exit(1);
            }
            registers[dInst->rd] = registers[dInst->rm] / registers[dInst->rn];
        break;
        case AND:
            if (dInst->rd > 15 || dInst->rd < 0) {
                exit(1);
            }
            if (dInst->rn > 15 || dInst->rn < 0) {
                exit(1);
            }
            if (dInst->rm > 15 || dInst->rm < 0) {
                exit(1);
            }
            registers[dInst->rd] = registers[dInst->rm] & registers[dInst->rn];
        break;
        case ORR:
            if (dInst->rd > 15 || dInst->rd < 0) {
                exit(1);
            }
            if (dInst->rn > 15 || dInst->rn < 0) {
                exit(1);
            }
            if (dInst->rm > 15 || dInst->rm < 0) {
                exit(1);
            }
            registers[dInst->rd] = registers[dInst->rm] | registers[dInst->rn];
        break;
        case EOR:
            if (dInst->rd > 15 || dInst->rd < 0) {
                exit(1);
            }
            if (dInst->rn > 15 || dInst->rn < 0) {
                exit(1);
            }
            if (dInst->rm > 15 || dInst->rm < 0) {
                exit(1);
            }
            registers[dInst->rd] = registers[dInst->rm] ^ registers[dInst->rn];
        break;
        case CMP:
            tempCpsr = 0;
            if (dInst->rd > 15 || dInst->rd < 0) {
                exit(1);
            }
            if (dInst->flag == 1) {
                if (dInst->rn > 15 || dInst->rn < 0) {
                    exit(1);
                }
                tempCmp = registers[dInst->rd] - registers[dInst->rn];
            }
            else {
                tempCmp = registers[dInst->rd] - dInst->immediate;
            }
            if (tempCmp < 0) {
                tempCpsr += 8;
            }
            if (tempCmp == 0) {
                tempCpsr += 4;
            }
            cpsr = (tempCpsr << 28);
        break;
        case B:
            if (dInst->address > 1023 || dInst->address < 0) {
                exit(1);
            }
            if (dInst->condition == 0) {
                //b
                registers[PC] = dInst->address;
            }
            else if (dInst->condition == 1) {
                //beq
                if ((cpsr >> 28) & 0x4) {
                    registers[PC] = dInst->address;
                }
            }
            else if (dInst->condition == 2) {
                //bne
                if (((cpsr >> 28) & 0x4) == 0) {
                    registers[PC] = dInst->address;
                }
            }
            else if (dInst->condition == 3) {
                //ble
                if (((cpsr >> 28) & 0x4) || (((cpsr >> 28) & 0x1) != ((cpsr >> 28) & 0x2))) {
                    registers[PC] = dInst->address;
                }
            }
            else if (dInst->condition == 4) {
                //blt
                if (((cpsr >> 28) & 0x1) != ((cpsr >> 28) & 0x2)) {
                    registers[PC] = dInst->address;
                }
            }
            else if (dInst->condition == 5) {
                //bge
                if (((cpsr >> 28) & 0x1) == ((cpsr >> 28) & 0x2)) {
                    registers[PC] = dInst->address;
                }
            }
            else if (dInst->condition == 6) {
                //bgt
                if ((((cpsr >> 28) & 0x4) == 0) && (((cpsr >> 28) & 0x1) == ((cpsr >> 28) & 0x2))) {
                    registers[PC] = dInst->address;
                }
            }
            else {
                //bl
                registers[PC] = dInst->address;
            }
        break;
        default:
            exit(1);
        break;
    }
    printf("CPSR: 0x%.8X\n", cpsr);
    free(dInst);
}

//Preform n steps
void step_n(int n) {
    for (int i = 0; i < n; i++) {
        step();
    }
}

//Step, then show any registers changed
void step_show_reg() {
    int prev_regs[16];
    for (int i = 0; i < 16; i++) {
        prev_regs[i] = registers[i];
    }

    step();

    for (int i = 0; i < 16; i++) {
        if (prev_regs[i] != registers[i]) {
            printf("reg[%i]: before: 0x%.8X, after: 0x%.8X\n", i, prev_regs[i], registers[i]);
        }
    }
}

//Step, then show any registers or memory that have changed
void step_show_reg_mem() {
    int prev_regs[16];
    int prev_address_val = 0;
    int cur_address_val = 0;
    for (int i = 0; i < 16; i++) {
        prev_regs[i] = registers[i];
    }

    int pc = registers[PC];
    int inst; 
    system_bus(pc, &inst, READ);
    decoded *dInst = decode(inst);
    if (dInst->opcode == STR) {
        system_bus(dInst->address, &prev_address_val, READ);
    }
    else if (dInst->opcode == STX) {
        system_bus(dInst->rn + dInst->offset, &prev_address_val, READ);
    }

    step();

    for (int i = 0; i < 16; i++) {
        if (prev_regs[i] != registers[i]) {
            printf("reg[%i]: before: 0x%.8X, after: 0x%.8X\n", i, prev_regs[i], registers[i]);
        }
    }

    if (dInst->opcode == STR) {
        system_bus(dInst->address, &cur_address_val, READ);
        printf("addr: 0x%.4X, before: 0x%.8X, after: 0x%.8X\n", dInst->address, prev_address_val, cur_address_val);
    }
    else if (dInst->opcode == STX) {
        system_bus(dInst->rn + dInst->offset, &cur_address_val, READ);
        printf("addr: 0x%.4X, before: 0x%.8X, after: 0x%.8X\n", dInst->rn + dInst->offset, prev_address_val, cur_address_val);
    }
}

