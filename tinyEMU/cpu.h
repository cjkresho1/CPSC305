
// Define opcodes
#define LDR 0x11
#define STR 0x12
#define LDX 0x13
#define STX 0x14
#define MOV 0x21
#define ADD 0x31
#define SUB 0x32
#define MUL 0x33
#define DIV 0x34
#define AND 0x35
#define ORR 0x36
#define EOR 0x37
#define CMP 0x41
#define B   0x51
#define NUM_OPCODES 14
// Define branch cond field
#define BAL 0
#define BEQ 1
#define BNE 2
#define BLE 3
#define BLT 4
#define BGE 5
#define BGT 6
#define BL 0x80
// Define CPSR bit positions
#define N 31
#define Z 30
#define C 29
#define V 28
// Define regists
#define SR 13
#define LR 14
#define PC 15
#define R0 0
#define R1 1
#define R2 2
#define R3 3
#define R4 4
#define R5 5
#define R6 6
#define R7 7
#define R8 8
#define R9 9
#define R10 10
#define R11 11
#define R12 12
#define R13 13
#define R14 14
#define R15 15

void set_reg(int reg, int value);
int get_reg(int reg);
int get_cpsr();
void show_regs();
void step();
void step_n(int n);
void step_show_reg();
void step_show_reg_mem();
