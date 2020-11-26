#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "cpu.h"
#include "isa.h"

static char *opcodes[] = {"nothing", "ldr", "str",  "ldx", "stx", "mov", "add", "sub", "mul", "div", "and", "orr", "eor", "cmp", "b"};
static int opcodes_val[] = {0, LDR, STR,  LDX, STX, MOV, ADD, SUB, MUL, DIV, AND, ORR, EOR, CMP, B};
static char *reg_strs[] = {"r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"};
static char *branches[] = {"b", "beq", "bne", "ble", "blt", "bge", "bgt"};

char *opcode_str(int opcode) {
    for (int i = 1; i < NUM_OPCODES+1; i++)
        if (opcode == opcodes_val[i])
            return opcodes[i];
    return NULL;
}

char disassembled[100]; // holds a disassembled instruction
char *disassemble(decoded *d) {
    char *p = disassembled;
    if (d->opcode != B) {
        p = strcpy(p, opcode_str(d->opcode));
        p = strcat(p, " ");
        p = strcat(p, reg_strs[d->rd]);
        p = strcat(p, ", ");
    }
    char buf[25];
    switch (d->opcode) {
      case LDR: case STR:
        sprintf(buf, "0x%x", d->address);
        p = strcat(p, buf);
        break;
      case LDX: case STX:
        p = strcat(p, "[");
        p = strcat(p, reg_strs[d->rn]);
        sprintf(buf, "#0x%x", d->offset);
        p = strcat(p, buf);
        break;
      case MOV: case CMP:
        if (d->flag)
            p = strcat(p, reg_strs[d->rn]);
        else {
            sprintf(buf, "#0x%x", d->immediate);
            p = strcat(p, buf);
        }
        break;
      case ADD: case SUB: case MUL: case DIV: case AND: case ORR: case EOR:
        p = strcat(p, reg_strs[d->rm]);
        p = strcat(p, ", ");
        p = strcat(p, reg_strs[d->rn]);
        break;
      case B:
        if (d->condition == 0x80)
            p = strcpy(p, "BL");
        else
            p = strcpy(p, branches[d->condition]);
        p = strcat(p, " ");
        sprintf(buf, "0x%x", d->address);
        p = strcat(p, buf);
        break;
      default:
        p = strcat(p, "BAD");
    }
    return disassembled;
}



decoded *decode(unsigned int inst) {
  decoded *val = malloc(sizeof(decoded));
  val->opcode = (inst >> 24) & 0xFF;
  val->rd = (inst >> 16) & 0xFF;
  val->rm = (inst >> 8) & 0xFF;
  val->rn = inst & 0xFF;
  val->flag = (inst >> 15) & 0x1 ;
  val->address = inst & 0xFFFF;
  val->immediate = inst & 0xFFFF;
  val->offset = (inst >> 8) & 0xFF;
  val->condition = (inst >> 16) & 0xFF;
  return val;
}
