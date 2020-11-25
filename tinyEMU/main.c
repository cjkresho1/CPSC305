#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "memory.h"
#include "memory_system.h"
#include "bit_functions.h"
#include "cpu.h"
#include "isa.h"


int main(int argc, char **argv) {
    while (1) {
        char cmd[100], file_name[100];
        int num, num1;
        printf("Enter cmd: ");
        scanf("%s", cmd);
        if (strcmp(cmd, "set_reg") == 0) {
            printf("Enter reg: ");
            scanf("%d", &num);
            printf("Ender reg value: ");
            scanf("%d", &num1);
            set_reg(num, num1);
        }
        else if (strcmp(cmd, "dump") == 0) {
            printf("Enter address: ");
            scanf("%x", &num);
            printf("Enter num bytes: ");
            scanf("%d", &num1);
            dump_memory(num, num1); 
        }
        else if (strcmp(cmd, "regs") == 0) {
            show_regs();
        }
        else if (strcmp(cmd, "step") == 0) {
            step();
        }
        else if (strcmp(cmd, "step_s") == 0) {
            step_show_reg();
        }
        else if (strcmp(cmd, "step_m") == 0) {
            step_show_reg_mem();
        }
        else if (strcmp(cmd, "step_n") == 0) {
            printf("Enter num of steps: ");
            scanf("%d", &num);
            step_n(num);
        }
        else if (strcmp(cmd, "load") == 0) {
            printf("Enter file name: ");
            scanf("%s", file_name);
            load_memory(file_name);
        }
        else if (strcmp(cmd, "disassemble") == 0) {
            printf("Enter inst: ");
            scanf("%x", &num);
            printf("num: %x\n", num);
            decoded *d = decode(num);
            if (d != NULL)
                printf("%s\n", disassemble(d));
            else
                printf("Invalid Inst\n");
        }
        else if (strcmp(cmd, "q") == 0) {
            exit(0);
        }
        else {
            printf("invalid cmd\n");
        }        
    }

}
