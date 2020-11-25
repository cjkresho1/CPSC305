#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <getopt.h>
#include <time.h> 
#include <unistd.h>
#include <sys/wait.h>

#define NDEBUG

// Define register macros
#define NUMBER_REGS 17
#define INDEX_PC 15
#define INDEX_CPSR 16
#define INDEX_SP 13
#define INDEX_LR 14
#define KERNEL_MODE 1

// Define memory layout macros
#define MEM_SIZE_WORDS 16384 // 2 ^ 14 word addresses
#define USER_ADDR  0x0000
#define OS_ADDR    0x1000
#define HEAP_ADDR  0x2000
#define STACK_ADDR 0x4000
#define MEM_DUMP_LEN 8

// Define maximum arguments for an emu command
#define MAXARGS 10

// Define GPIO macros
#define GPIO20_29_ADDRESS 0x20200008
#define IO_STR_ADDRESS   0x20200004
#define IO_INT_ADDRESS   0x20200000
#define GPIO_OUTPUT_OFF   0x20200028
#define GPIO_OUTPUT_ON    0x2020001C

struct proc_state {
  int NEG; // N flag
  int ZER; // Z flag
  int CRY; // C flag
  int OVF; // V flag
  int SVC; // kernel mode when 1
  int PC;
  int prev_regs[NUMBER_REGS];
  int regs[NUMBER_REGS];
  int prev_memory[OS_ADDR];
  int memory[MEM_SIZE_WORDS];
};

typedef struct proc_state proc_state_t;

struct pipeline {
  int fetched;
  int decoded;
  int breakpoint;
};

typedef struct pipeline pipeline_t;

/*
 opcodes and condcodes are used by disassemble.
 */
static char *opcodes[] = {"and", "eor", "sub", "rsb", "add", "adc", "sbc", "rsc", 
                          "tst", "teq", "cmp", "cmn", "orr", "mov", "bic", "mvn" };
static char *condcodes[] = {"eq", "ne", "cs", "cc", "mi", "pl", "vs", "vc", "hi", "ls", "ge", "lt", "gt", "le"};

/*
 Print the disassembled intruction to stdout.
 TODO: Maybe change to return a string with the disassembled instruction.
 */
void disassemble(int inst) { // prints disassembled instruction to stdout
  int cond = inst >> 28 & 0xf;
  int zo = inst >> 26 & 3;
  int i = inst >> 25 & 1;
  int p = inst >> 24 & 1;
  int u = inst >> 23 & 1;
  //int b = inst >> 22 & 1;
  int w = inst >> 21 & 1;
  int l = inst >> 20 & 1;
  int rn = inst >> 16 & 0xf;
  int rd = inst >> 12 & 0xf;
  int rs = inst & 0xf;
  int sl = inst >> 7 & 0x1f;
  int st = inst >> 5 & 3;
  int imm = inst & 0xfff;
  int s = inst >> 20 & 1;
  int dpopcode = inst >> 21 & 0xf;
  int dpimm = inst & 0xff;
  int dpimmro = inst >> 8 & 0xf;
  int dprm = inst & 0xf;
  int dpregshift = inst >> 4 & 1;
  int dprs = inst >> 8 & 0xf; // reg in shift
  int dpst = inst >> 5 & 3; // shift type
  int dpimmsl = inst >> 7 & 0x1f; // imm shift len
  int bl = inst >> 24 & 1;
  int boff = inst & 0xffffff;
  int mulop1 = inst >> 22 & 0x3f;
  int mulop2 = inst >> 4 & 0xf;
  if (mulop1 == 0 && mulop2 == 9) { // mul
    int mrd = inst >> 16 & 0xf;
    //int mrn = inst >> 12 & 0xf;
    int mrs = inst >> 8 & 0xf;
    int mrm = inst & 0xf;
    printf("mul");
    if (cond <= 13)
      printf("%s", condcodes[cond]);
    if (s)
      printf("s");
    printf(" r%d, r%d, r%d", mrd, mrm, mrs);
  } else if (zo == 1) { // ldr/str
    if (l)
        printf("ldr");
    else
      printf("str");
    if (cond <= 13)
      printf("%s", condcodes[cond]);
    printf(" ");
    printf("r%d, ", rd);
    printf("[r%d", rn);
    if (i) { // reg with shifts
      if (!w || (w && p)) { // not update rn or pre update - regs go in []
        printf(", r%d", rs);
        if (sl != 0) {
          if (st == 0)
            printf(", lsl, #%d]", sl);
          else if (st == 1)
            printf(", lsr, #%d]", sl);
          else if (st == 2)
            printf(", asr, #%d]", sl);
          else if (st == 3)
            printf(", ror, #%d]", sl);
        } else
          printf("]");
        if (w && p && sl != 0)
          printf("!");
      } else { // post update rn - regs go at end of []
        printf("], r%d", rs);
        if (sl != 0) {
          if (st == 0)
            printf(", lsl #%d]", sl);
          else if (st == 1)
            printf(", lsr #%d]", sl);
          else if (st == 2)
            printf(", asr #%d]", sl);
          else if (st == 3)
            printf(", ror #%d]", sl);
        } else
          printf("]");
      }
    } else { // immediate
      if (!w || (w && p)) { // not update rn or pre update - imm go in []
        if (imm != 0) {
          if (!u)
            printf(", #-%d", imm);
          else
            printf(", #%d", imm);
        }
        printf("]");

        if (w && p && imm != 0)
          printf("!");
      } else { // imm go at end of []
        if (!u)
          printf("], #-%d", imm);
        else
          printf("], #%d", imm);
      }
    }
    /*
    printf("cond: %01x, ", cond); 
    printf("zo: %01x, ", zo); 
    printf("i: %01x, ", i); 
    printf("p: %01x, ", p); 
    printf("u: %01x, ", u); 
    printf("b: %01x, ", b); 
    printf("w: %01x, ", w); 
    printf("l: %01x, ", l); 
    printf("rn: %02x, ", rn); 
    printf("rd: %02x, ", rd); 
    printf("rs: %02x, ", rs); 
    printf("sl: %02x, ", sl); 
    printf("st: %02x, ", st); 
    printf("imm: %03x\n", imm); 
    */
  } else if (zo == 0) { // data processing instruction
    printf("%s", opcodes[dpopcode]);
    if (cond <= 13)
      printf("%s", condcodes[cond]);
    if (s)
      printf("s");
    printf(" ");
    if (dpopcode <= 7 || dpopcode == 12 || dpopcode == 14) {
      printf("r%d, ", rd);
      printf("r%d, ", rn);
    } else {
      if (dpopcode == 13 || dpopcode == 15) // mov or mvn
        printf("r%d, ", rd);
      else // compares use rn
        printf("r%d, ", rn);
    }
    if (i) { // imm op2 
      printf("#%02x", dpimm);
      if (dpimmro != 0)
        printf(" // Rotate: %02x", dpimmro);
    } else { // reg op2
      printf("r%d", dprm);
      if (dpregshift) { // shift count int register
        printf(", r%1x", dprs);
        if (dpst == 0)
          printf(", lsl");
        else if (dpst == 1)
          printf(", lsr");
        else if (dpst == 2)
          printf(", asr");
        else if (dpst == 3)
          printf(", ror");
      } else { // shift count in literal
        if (dpimmsl != 0) {
          if (dpst == 0)
            printf(", lsl #%d]", dpimmsl);
          else if (dpst == 1)
            printf(", lsr #%d]", dpimmsl);
          else if (dpst == 2)
            printf(", asr #%d]", dpimmsl);
          else if (dpst == 3)
            printf(", ror #%d]", dpimmsl);
        }
      }
    }
    //printf("\n");
  } else if (zo == 2) { // branch instruction
    printf("b");
    if (bl)
      printf("l");
    if (cond <= 13)
      printf("%s", condcodes[cond]);
    printf(" %06x", boff);
    //printf("\n");
  } else if (zo == 3) { // svc instruction
    printf("svc #%d", boff);
  }
}

/*
 Shifts with S bit on set the carry flag
 LSRS #3 // bit  2 is placed in the carry flag
 ASRS #3 // bit  2 is placed in the carry flag
 R0RS #3 // bit  2 is placed in the carry flag
 LSLS #3 // bit 29 is placed in the carry flag

        3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 
 C      1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0    C
+-+    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  +-+
| |    | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |  | |
+-+    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+  +-+
 ^          |                                                     |        ^
 +----------+                                                     +--------+
 */
int executeShift(int contentsRm, int shiftValue, proc_state_t *pState,
                 int shiftType, int highBitRm, int Rm, bool setFlags) {
// TODO simplify args - Rm not used
  uint32_t operand2ThroughShifter = -1;
  int signBit = 0;
  switch(shiftType) {
    case 0x0: // left
      operand2ThroughShifter = contentsRm << shiftValue;
      break;
    case 0x1: // logical right
      operand2ThroughShifter = ((uint32_t) contentsRm) >> shiftValue;
      if(setFlags) {
        pState->CRY = contentsRm >> (shiftValue - 1) & 1;
      }
      break;
    case 0x2: // arithmetic right
      signBit = contentsRm  & 0x80000000;
      for(int i = 0; i < shiftValue; i++) {
        contentsRm = contentsRm >> 1;
        contentsRm =  contentsRm | signBit;
      }
      if(setFlags) {
        // how should carry be set. The original bit or arithmetic shifted bit. I do original.
        pState->CRY = contentsRm >> (shiftValue - 1) & 1; // original bit
      //pState->CRY = operand2ThroughShifter >> (shiftValue - 1) & 1; // arithmetic shifted bit
      }
      break;
    case 0x3: // rotate right
      operand2ThroughShifter = (contentsRm >> shiftValue) | (contentsRm << (32-shiftValue));
      if(setFlags) {
        pState->CRY = contentsRm >> (shiftValue - 1) & 1;
      }
      break;
  }
  assert(operand2ThroughShifter != -1);
  return operand2ThroughShifter;
}

/*
  Compute carry bit for two operands
 */
int getAdditionCarry(int operand1, int operand2) {
  int msbOp1 = operand1 >> 31;
  int msbOp2 = operand2 >> 31;
  int result = operand1 + operand2;
  int msbResult = result >> 31;
  if((msbOp1 + msbOp2) != msbResult) {
    return 1;
  } else {
    return 0;
  }
}

/*
  opcode is a data processing instruction
 */
void executeOperation(proc_state_t *pState, int Rdest, int Rn, int operand2, int S, int opcode) {
  int opResult = -1;
  int carry = -1;
  bool zflag = false;
  switch(opcode) {
    case 0x0: /*AND*/
      pState->regs[Rdest] = pState->regs[Rn] & operand2;
      opResult = pState->regs[Rdest];
      zflag = opResult == 0;
      carry = 0;
      break;
    case 0x1: /*EOR*/
      pState->regs[Rdest] = pState->regs[Rn] ^ operand2;
      opResult = pState->regs[Rdest];
      zflag = opResult == 0;
      carry = 0;
      break;
    case 0x2: /*SUB*/
      pState->regs[Rdest] = pState->regs[Rn] - operand2;
      opResult = pState->regs[Rdest];
      zflag = opResult == 0;
      carry = opResult & 0x80000000 ? 0 : 1;
      break;
    case 0x3: /*RSB*/
      pState->regs[Rdest] = operand2 - pState->regs[Rn];
      opResult = pState->regs[Rdest];
      zflag = opResult == 0;
      carry = getAdditionCarry(operand2, !(pState->regs[Rn]) + 1);
      break;
    case 0x4: /*ADD*/
      pState->regs[Rdest] = pState->regs[Rn] + operand2;
      opResult = pState->regs[Rdest];
      zflag = opResult == 0;
      carry = getAdditionCarry(pState->regs[Rn], operand2);
      break;
    case 0x8: /*TST*/
      opResult = pState->regs[Rn] & operand2;
      carry = 0;
      zflag = opResult == 0;
      break;
    case 0x9: /*TEQ*/
      opResult = pState->regs[Rn] ^ operand2;
      carry = 0;
      zflag = opResult == 0;
      break;
    case 0xA: /*CMP*/
      opResult = pState->regs[Rn] - operand2;
      carry = opResult & 0x80000000 ? 0 : 1;
      zflag = opResult == 0;
      break;
    case 0xC: /*ORR*/
      pState->regs[Rdest] = pState->regs[Rn] | operand2;
      opResult = pState->regs[Rdest];
      zflag = opResult == 0;
      carry = 0;
      break;
    case 0xD: /*MOV*/
      pState->regs[Rdest] = operand2;
      opResult = operand2;
      zflag = opResult == 0;
      carry = 0;
      //zflag = false; // should MOV r0, #0 set Z bit?
      // return from kernel mode is ugly.
      // svc instruction sets kernel mode and does BL to the OS address
      // OS does a mov PC, LR to return to application
      // If cpsr state is supervisory and mov destination if PC, we return to user mode
      if (pState->SVC && Rdest == INDEX_PC) { // returned from kernel mode
        pState->SVC = 0;
        pState->regs[INDEX_CPSR] = pState->regs[INDEX_CPSR] & ~KERNEL_MODE;
      }
      break;
  }

  if(S) { // CMP has the S bit set
    //barrel shifter ~ set C to carry out from any shift operation;
    //ALU ~ C = Cout of bit 31;
    pState->CRY = carry; // C bit
    pState->ZER = zflag ? 1 : 0; // Z bit
    pState->NEG = opResult >> 31;
    int modebits = pState->regs[INDEX_CPSR] & 0x1f;
    pState->regs[INDEX_CPSR] = (pState->NEG << 31) | (pState->ZER << 30) |
                               (pState->CRY << 29) | (pState->OVF << 28) | modebits;
  }
}

/*
  I bit - 0=imm op2, 1=reg op2
  S bit - 0=do not update cpsr, 1=update cpsr
  Rn - add rd, rn, op2
  Rdest - add rd, rn, op2
 */
void executeDataProcessing(int instruction, proc_state_t *pState, pipeline_t *pipeline) {
   int S = instruction >> 20 & 1;
   int I = instruction >> 25 & 1;
   int Rn = instruction >> 16 & 0xf;
   int Rdest = instruction >> 12 & 0xf;
   int opcode = instruction >> 21 & 0xf;
   if(I) { // immed op2
     int rotate = instruction >> 8 & 0xf;
     int immediate = instruction & 0xff;
     rotate =  2 * rotate;
     int rotatedImmediate = (immediate >> rotate) | (immediate << (32-rotate));
     executeOperation(pState, Rdest, Rn, rotatedImmediate, S, opcode);
   } else { // reg op2
     int shift = instruction >> 4 & 0xff;
     int Rm = instruction & 0xf;
     int shiftType = shift >> 1 & 3; //2 bits (UX to 32)
     int highBitRm = pState->regs[Rm] >> 31;
     int contentsRm = pState->regs[Rm];
     int operand2ThroughShifter = -1;
     if(shift & 1) { // if bit0 is set, reg shift
       int bit7 = (instruction & 0x80) >> 7;
       if(bit7) { // bit7 must be 0 for reg shift operand
         fprintf(stderr, "%s\n", "Operand2 is invalid");
       }
       int Rs = instruction >> 8 & 0xf;
     //int shiftValue = getByteBigEndian(pState->regs[Rs], 3);
       int shiftValue = pState->regs[Rs] & 0xff;
       operand2ThroughShifter = executeShift(contentsRm, shiftValue, pState,
                                             shiftType, highBitRm, Rm, true);
     } else { // integer shift
       int shiftValueInteger = (shift & 0xF8 ) >> 3; // shift val in upper 5 bits
       operand2ThroughShifter = executeShift(contentsRm, shiftValueInteger,
                                             pState, shiftType, highBitRm,
                                             Rm, true);
     }
     executeOperation(pState, Rdest, Rn, operand2ThroughShifter, S, opcode);
   }
   if (Rdest == INDEX_PC) { // do a branch
     pState->PC = pState->regs[Rdest] - 4;
     pState->regs[INDEX_PC] = pState->PC;
     pipeline->decoded = -1;
     pipeline->fetched = -1;
   }
}

int setByte(int word, int index, int newByte) {
  // word indexed as in big endian {0, 1, 2, 3}
  switch (index) {
    case 0: return (0x00FFFFFF & word) | (newByte << 24);
    case 1: return (0xFF00FFFF & word) | (newByte << 16);
    case 2: return (0xFFFF00FF & word) | (newByte << 8);
    case 3: return (0XFFFFFF00 & word) |  newByte;
    default: return -1;
  }
}

/*
  This clever function (along with setByte) allows the following.
   mov r6, #0x100
   orr r6, r6, #5        // r6 has addres 0x105, not multiple of 4
   mov r3, #0xaa000000
   orr r3, r3, #0xbb0000
   orr r3, r3, #0xcc00
   orr r3, r3, #0xdd     // r3 has 0xaabbccdd
   str r3, [r6]          // updates bytes starting at 0x105
 */
void fillByteAddress(int startByteAddress, proc_state_t *pState, int *byteArr) {
  //Starting from mem[startByteAddress], function replaces existing bytes
  //with bytes from the array referenced by byteArr
  for(int i = 0; i < 4; i++) {
    pState->memory[startByteAddress / 4] =
    setByte(pState->memory[startByteAddress / 4], 3-startByteAddress % 4, byteArr[i]); 
    startByteAddress++;
  }
}

void writeToMemory(int word, int startByteAddress, proc_state_t *pState) {
  // Declaring heap allocated arrays that hold the state of the gpio pins
  /*gpioPhysical[0...2] is the representation of mem[0x2020 0000...0x2020 0008]
   *and holds control bits for each pin in the intervals 0-9, 10-19, 20-29resp*/

  /* gpioOutputOnOff holds two addresses:
   * 0 |-> 0x2020 0028 OFF
     1 |-> 0x2020 001C ON
   */

  int *gpioPhysical = (int *) malloc(3 * sizeof(int));
  int *gpioOutputOnOff = (int *) malloc(2 * sizeof(int));
  if(gpioPhysical == NULL || gpioOutputOnOff == NULL) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  //word written to memory starting from startByteAddress
//int byte[4] = {getByteBigEndian(word, 3),
//               getByteBigEndian(word, 2),
//               getByteBigEndian(word, 1),
//               getByteBigEndian(word, 0)};
  int byte[4];
  byte[0] = word       & 0xff;
  byte[1] = word >>  8 & 0xff;
  byte[2] = word >> 16 & 0xff;
  byte[3] = word >> 24 & 0xff;
  int *byteArray = byte;
  //Got every byte of the number to be written to memory
  //  int changedPin = -1;
  //changedPin is the index of the pin that has been set as output
  switch(startByteAddress) {
    case IO_INT_ADDRESS:
      if (pState->SVC) {
        printf("emu int printf: %d, 0x%08x\n", pState->regs[1], pState->regs[1]);
      } else
        printf("printf int from user mode not allowed.\n");
      break;
    case IO_STR_ADDRESS:
      if (pState->SVC) {
        //char str[100];
        char *s = (char *)&pState->memory[pState->regs[1] / 4];
        printf("emu str printf: %s\n", s);
      } else
        printf("printf int from user mode not allowed.\n");
      break;
    case GPIO20_29_ADDRESS:
      printf("%s\n", "One GPIO pin from 20 to 29 has been accessed");
      gpioPhysical[2] = word;
      // changedPin = getChangedOutputPin(word, 2);
      break;
    case GPIO_OUTPUT_ON: 
      printf("%s\n", "PIN ON");
      gpioOutputOnOff[0] = word;
      break;
    case GPIO_OUTPUT_OFF: 
      printf("%s\n", "PIN OFF");
      gpioOutputOnOff[1] = word;
      break;
    default:
      fillByteAddress(startByteAddress, pState, byteArray);
      //pState->memory[startByteAddres / 4] = word;
  }
  free(gpioPhysical);
  free(gpioOutputOnOff);
}

/*
  These two functions allows ldr to fetch 4 bytes from an address that is not muliple of 4
  See equivalent function fillByteAddress and setByte for str
  For now, I have commented out the ldr functions, but I retained the str functions
  ldr from an address that is not a multiple of four will get a wrong number
  str from an address that is not a multiple of four will work
*/

/*  
int getByteBigEndian(int content, int index) {
  switch (index) {
    case 0: return (content & 0xff000000) >> 24;
    case 1: return (content & 0xff0000) >> 16;
    case 2: return (content & 0xff00) >> 8;
    case 3: return (content & 0xff);
    default: return -1;
  }
}

int getMemoryContentsAtAddress(proc_state_t *pState, int address) {
  int byteAddress = address;
  int byte0 = getByteBigEndian(pState->memory[byteAddress / 4], 3 - (byteAddress % 4));
  byteAddress++;
  int byte1 = getByteBigEndian(pState->memory[byteAddress / 4], 3 - (byteAddress % 4));
  byteAddress++;
  int byte2 = getByteBigEndian(pState->memory[byteAddress / 4], 3 - (byteAddress % 4));
  byteAddress++;
  int byte3 = getByteBigEndian(pState->memory[byteAddress / 4], 3 - (byteAddress % 4));
  byteAddress++;
  return byte0 | (byte1 << 8) | (byte2 << 16) | (byte3 << 24);
}
*/

void executeLoadFromMemoryGPIO(proc_state_t *pState, int Rd, int address) {
  switch (address) {
    case IO_INT_ADDRESS:
      if (pState->SVC) {
        printf("emu int scanf - enter a number: ");
        int value;
        scanf("%d", &value);
        pState->regs[Rd] = value;
        //pState->regs[Rd] = IO_INT_ADDRESS;
      } else
        printf("scanf str from user mode not allowed.\n");
      break;
    case IO_STR_ADDRESS:
      if (pState->SVC) {
        //char str[100];
        printf("emu str scanf - enter a string: ");
        char *s = (char *)&pState->memory[pState->regs[1] / 4];
        scanf("%s", s);
      } else
        printf("scanf str from user mode not allowed.\n");
      break;
    case GPIO20_29_ADDRESS:
      printf("%s\n", "One GPIO pin from 20 to 29 has been accessed");
      pState->regs[Rd] = GPIO20_29_ADDRESS;
      break;
    default:
      if(address > (MEM_SIZE_WORDS - 4)) {
        printf("Error: Out of bounds memory access at address 0x%.8x\n", address);
      } else {
        pState->regs[Rd] = pState->memory[address / 4];
        //original code, which I thought converted big Endian to little Endian
        //pState->regs[Rd] = getMemoryContentsAtAddress(pState, address);
        //Simplified big Endian to little Endian
        //int value = pState->memory[address / 4];
        //int byte0 = value >> 24 & 0xff;
        //int byte1 = value >> 16 & 0xff;
        //int byte2 = value >> 8 & 0xff;
        //int byte3 = value & 0xff;
        //pState->regs[Rd] = byte0 | byte1 << 8 | byte2 << 16 | byte3 << 24;
      }
  }
}

int getEffectiveAddress(int Rn, int offset, int U, proc_state_t *pState) {
  if (U) {
    return (pState->regs[Rn] + offset);
  } else {
    return (pState->regs[Rn] - offset);
  }
}

/*
  I Bit - 0 is imm, 1 is reg with shift
  L Bit - 0 is str, 1 is ldr
  P Bit - 0 is post inc/dec, 1 is pre inc/dec
  U Bit - 0 is sub from rn, 1 is add to rn
  W Bit - 0 dont write back to rn, 1 write back to rn
  Rn - base register
  Rd - source/destination register
  Rm - shift register
 */
void executeSDataTransfer(int instruction, proc_state_t *pState) {
  int I = instruction >> 25 & 1;
  int L = instruction >> 20 & 1;
  int P = instruction >> 24 & 1;
  int U = instruction >> 23 & 1;
  int W = instruction >> 21 & 1;   // 0 dont write back to rn, 1 write back to rn
  int Rn = instruction >> 16 & 0xf;
  int Rd = instruction >> 12 & 0xf;
  int offset = -1;
  if (I) { // Offset is a shifted register
    int shift = instruction >> 4 & 0xff;
    int Rm = instruction & 0xf;
    int shiftType = shift >> 1 & 3; //2 bits (UX to 32)
    int highBitRm = pState->regs[Rm] >> 31;
    int contentsRm = pState->regs[Rm];
    int operand2ThroughShifter = -1;
    // if(getLSbit(shift)) { // if bit0 is set, reg shift
    //   int bit7 = (instruction & 0x80) >> 7;
    //   if(bit7) { // bit7 must be 0 for reg shift operand
    //     fprintf(stderr, "%s\n", "Operand2 is invalid");
    //   }
    if(shift & 1) { // if bit0 must be 0 for reg shift on ldr/str
      fprintf(stderr, "%s\n", "Operand2 is invalid");
    } else {
      int shiftValueInteger = (shift & 0xF8) >> 3;
      operand2ThroughShifter = executeShift(contentsRm, shiftValueInteger,
                               pState, shiftType, highBitRm, Rm, false);
    }
    offset = operand2ThroughShifter; // value to be added/subtracted to/from Rn
  } else { //Offset is 12-bit immediate value
    offset = (uint32_t) (instruction & 0xfff);
  }
  int address = -1;
  if (L) { // ldr inst, regs[Rd] = Mem[address]
     if (P) { //Pre-indexing
       address = getEffectiveAddress(Rn, offset, U, pState);
       address = pState->regs[Rn] + (U ? offset : -offset);
       executeLoadFromMemoryGPIO(pState, Rd, address);
       if (W) { // write-back address to rn
         pState->regs[Rn] = address;
       }
     } else { //Post-indexing
       address = pState->regs[Rn];
       executeLoadFromMemoryGPIO(pState, Rd, address);
       if (W) { // write-back address to rn
         pState->regs[Rn] = getEffectiveAddress(Rn,offset, U, pState);
         address = pState->regs[Rn] + (U ? offset : -offset);
       }
     }
  } else { // str instr,  Mem[address] = regs[Rd]
     if (P) { //Pre-indexing
       address = getEffectiveAddress(Rn, offset, U, pState);
       address = pState->regs[Rn] + (U ? offset : -offset);
       writeToMemory(pState->regs[Rd], address, pState);
       if (W) { // write-back address to rn
         pState->regs[Rn] = address;
       }
     } else { //Post-indexing
       address = pState->regs[Rn];
       writeToMemory(pState->regs[Rd], address, pState);
       //Then set base register
       if (W) { // write-back address to rn
         pState->regs[Rn] = getEffectiveAddress(Rn,offset, U, pState);
         address = pState->regs[Rn] + (U ? offset : -offset);
       }
     }
  }
}


void executeMultiply(int instruction, proc_state_t *pState) {
  int A = instruction >> 21 & 1;
  int S = instruction >> 20 & 1;
  int Rd = instruction >> 16 & 0xf;
  int Rn = instruction >> 12 & 0xf;
  int Rs = instruction >> 8 & 0xf;
  int Rm = instruction & 0xf;
  int auxResultMult = -1;
  assert(Rd != Rm);
  if(A) {
    pState->regs[Rd] = (int) (pState->regs[Rm] * pState->regs[Rs]) + pState->regs[Rn];
  } else {
    pState->regs[Rd] = (int) pState->regs[Rm] * pState->regs[Rs];
  }
  auxResultMult = pState->regs[Rd];
  if(S) {
    //Set NEG and ZER flags
    pState->NEG = auxResultMult >> 31;
    pState->ZER = auxResultMult ? 0 : 1;
    pState->regs[INDEX_CPSR] = (pState->NEG << 31) | (pState->ZER << 30) |
                               (pState->CRY << 29) | (pState->OVF << 28);
  }
}

/**
  SVC saves return address in LR and branches to OS at address OS_ADDR
 */
void executeSvc(int instruction, proc_state_t *pState, pipeline_t *pipeline){
    if (pState->SVC)
      printf("Already in kernel mode.\n");
    int service = instruction & 0xffffff;
    pState->regs[INDEX_LR] = pState->PC;
    pState->PC = OS_ADDR;
    pState->regs[INDEX_PC] = pState->PC;
    pState->regs[INDEX_CPSR] = pState->regs[INDEX_CPSR] | KERNEL_MODE;
    pState->SVC = 1;
    pState->regs[0] = service;
    pipeline->decoded = -1;
    pipeline->fetched = -1;

}

void executeBranch(int instruction, proc_state_t *pState, pipeline_t *pipeline){
    int offset = instruction & 0xffffff;
    offset = offset << 2;
    //Offset is a 32-bit value having bits[25...31] equal 0
    int maskBit25 = 0x2000000;
    int signBitPosition = 25;
    int signBit = (offset & maskBit25) >> signBitPosition;
    int mask26To31 = 0xFC000000;
    if(signBit) {
      offset = offset | mask26To31;
    }
    // otherwise the offset is unchanged
    int PCvalue = pState->PC;
    if (instruction & 1<<24) // bl instructin
      pState->regs[INDEX_LR] = PCvalue;
    pState->PC = PCvalue + offset;
    pState->regs[INDEX_PC] = pState->PC;
    pipeline->decoded = -1;
    pipeline->fetched = -1;
}

bool shouldExecute(int instruction, proc_state_t *pState) {
   uint8_t cond = (uint8_t)(instruction >> 28 & 0xf);
   int N = pState->NEG;
   int Z = pState->ZER;
   int V = pState->OVF;
   switch(cond) {
     case 0:  return Z == 1;
     case 1:  return Z == 0;
     case 10: return N == V;
     case 11: return N != V;
     case 12: return ((Z == 0) && (N == V));
     case 13: return ((Z == 1) || (N != V));
     case 14: return true;
     default: return false;
   }
}

/**
 Bits 26 and 27 distinguish three instructions
 00 - Data Processing Instructions
 01 - LDR/STR instructions - Single Data Transfer
 10 - Branch instructions
 Multiply requires bits 22-27 to be 0 and bits 4-7 to be 9
 */
void executeInst(int instruction, proc_state_t *pState, pipeline_t *pipeline){
   int idBits = instruction >> 26 & 3;
   if(idBits == 1) {
       if(shouldExecute(instruction, pState)) {
        executeSDataTransfer(instruction, pState);
       }
   } else if(idBits == 2) {
      if(shouldExecute(instruction, pState)) {
        executeBranch(instruction, pState, pipeline);
      }
   } else if (idBits == 3) {
      //int svc = instruction >> 24 & 0xf;
      executeSvc(instruction, pState, pipeline);
   } else if(!idBits) {
      int mulop1 = instruction >> 22 & 0x3f;
      int mulop2 = instruction >> 4 & 0xf;
      if (mulop1 == 0 && mulop2 == 9) { // mul
        if(shouldExecute(instruction, pState)) {
          executeMultiply(instruction, pState);
        }
      } else {
        if(shouldExecute(instruction, pState)) {
         executeDataProcessing(instruction, pState, pipeline);
        }
      }
   } else {
      printf("idBits: %x\n", idBits);
      printf("%s\n", "Should not get here");
      fprintf(stderr, "%s\n", "Invalid instruction executing.");
      exit(EXIT_FAILURE);
   }
}

void memoryLoader(FILE *file, proc_state_t *pState, int addr) {
  if(!file) {
    fprintf(stderr, "%s\n", "File not found");
  }
  fread(pState->memory+addr, sizeof(uint32_t), MEM_SIZE_WORDS, file);
  fclose(file);
}

/*
 pState->memory[] - array of int stored in big Endian
 */
void memory_dump(proc_state_t *pState, int start_address, int num_words) {
    for (int i = start_address; i < start_address+num_words; i++) {
        int b = pState->memory[i];
        int l = (b >> 24 & 0xff) | (b >> 8 & 0xff00) | (b << 8 & 0xff0000) | (b << 24 & 0xff000000);
        printf("0x%08x:  0x%08x 0x%08x  ", i<<2, b, l);
        char *bp = (char *)(&b);
        for (int j = 0; j < 4; j++)
          if (isprint(bp[j]))
            printf("%c", bp[j]);
          else
            printf(" ");
        printf("\n");
    }
}

/*
 Print disassembled memory to stdout from start_addres to start_addres+num_words-1
 */
void memory_list(proc_state_t *pState, int start_address, int num_words) {
    for (int i = start_address; i < start_address+num_words; i++) {
        printf("0x%08x:  ", i<<2);
        disassemble(pState->memory[i]);
        printf("\n");
    }
}

void printPipeline(pipeline_t *pl, proc_state_t *pState) {
  printf("executed: ");
  printf("0x%08x: ", pState->PC - 8);
  if (pl->decoded != 0 && pl->decoded != -1)
    disassemble(pl->decoded);
  else
    printf("pl flushed");
  printf("   fetched: ");
  printf("0x%08x: ", pState->PC - 4);
  if (pl->fetched != 0 && pl->fetched != -1)
    disassemble(pl->fetched);
  else
    printf("not ins");
  printf("   nextfetch: ");
  printf("0x%08x: ", pState->PC);
  int pcmem = pState->memory[pState->PC / 4];
  if (pcmem != 0 && pcmem != -1)
    disassemble(pcmem);
  else
    printf("not ins");
  printf("\n");
  printf("executed: 0x%08x: 0x%08x  ", pState->PC - 8, pl->decoded);
  printf("fetched: 0x%08x: 0x%08x  ", pState->PC - 4, pl->fetched);
  printf("nextfetch: 0x%08x: 0x%08x\n", pState->PC, pcmem);
    
}

/*
 Execute the pl->fetched instruction
 Updates pipeline
 Optionally prints pipeline and emu state
 Return value:
  0 - regular step
  1 - breakpoint hit
  2 - finished
 */
int step(proc_state_t *pState, pipeline_t *pl, int ss, int printpl) {
  int status = 0;
  for (int reg = 0; reg < NUMBER_REGS; reg++)
    pState->prev_regs[reg] = pState->regs[reg];
  for (int mem = 0; mem < OS_ADDR; mem++)
    pState->prev_memory[mem] = pState->memory[mem];
  pState->PC += 4;
  pState->regs[INDEX_PC] = pState->PC;
  pl->decoded = pl->fetched;
  pl->fetched = pState->memory[pState->PC / 4 - 1];
  if (printpl)
    printPipeline(pl, pState);
  if (pl->decoded != -1) {
    executeInst(pl->decoded, pState, pl);
    if (ss) {
      for (int reg = 0; reg < NUMBER_REGS; reg++)
        if (pState->prev_regs[reg] != pState->regs[reg]) {
          if (reg == 15)
            printf("PC  :%11d (0x%.8x)\n", pState->regs[reg], pState->regs[reg]);
          else if (reg == 16) {
            int content = pState->regs[reg];
            printf("CPSR:%11d (0x%.8x)", content, content);
            printf("  N: %d, Z: %d, C: %d, V: %d K: %d\n", content >> 31 & 1, content >> 30 & 1, content >> 29 & 1, content >> 28 & 1, pState->SVC);
          }
          else
            printf("R%-3d:%11d (0x%.8x)\n", reg, pState->regs[reg], pState->regs[reg]);
        }
      if (ss == 2) { 
        for (int mem = 0; mem < OS_ADDR; mem+=4)
          if (pState->prev_memory[mem / 4] != pState->memory[mem / 4]) {
            int b = pState->memory[mem / 4];
            int l = (b >> 24 & 0xff) | (b >> 8 & 0xff00) | (b << 8 & 0xff0000) | (b << 24 & 0xff000000);
            printf("0x%.8x: 0x%.8x 0x%.8x\n", mem, b, l);
          }
      }
    }
  }
  // PC-4 is addr of fetched. BP hit when pl->breakpoiont == PC-4
  if (pl->breakpoint != -1 && (pState->PC - 4) == pl->breakpoint) {
    printf("Breakpoint hit: %08x: ", pState->PC - 4);
    disassemble(pl->fetched);
    printf("\n");
    status = 1;
  }
  // !pl->decoded indicated finished in the original code, i.e., pl->decode == 0
  return status;
}

/*
  Prints non-zero memory in Big and Little Endian
 */
void printMemory(int memory[]) {
   printf("%s", "Non-zero memory:\n");
   int prev_addr = -4; // last address printed
   for(int addr = 0; addr < MEM_SIZE_WORDS; addr += 4) {
     if(memory[addr / 4]) {
       if (addr - prev_addr != 4)
         printf("...\n");
       int b = memory[addr / 4];
       int l = (b >> 24 & 0xff) | (b >> 8 & 0xff00) | (b << 8 & 0xff0000) | (b << 24 & 0xff000000);
       printf("0x%.8x: 0x%.8x 0x%.8x  ", addr, b, l);
       char *bp = (char *)(&b);
       for (int j = 0; j < 4; j++)
         if (isprint(bp[j]))
           printf("%c", bp[j]);
         else
           printf(" ");
       printf("\n");
       prev_addr = addr;
     }
   }
}

int getNumberOfDecimalDigits(int number) {
  int counter = 0;
  while (number) {
    number /= 10;
    counter++;
  }
  return counter;
}

void printRegs(proc_state_t *pState) {
  printf("%s\n", "Registers:");
  int content, regNegative;
  for(int i = 0; i < NUMBER_REGS; i++) {
    content = (pState->regs)[i];
    regNegative = pState->regs[i] & 0x80000000;
   // if(i != INDEX_LR && i != INDEX_SP) {
     if(i == INDEX_PC) {
       printf("PC  :%11d (0x%.8x)\n", content, content);
     } else if(i == INDEX_CPSR) {
       if(regNegative) {
          printf("CPSR: %11d (0x%.8x)", content, content);
       } else {
          printf("CPSR:%11d (0x%.8x)", content, content);
       }
       printf("  N: %d, Z: %d, C: %d, V: %d\n", content >> 31 & 1, content >> 30 & 1, content >> 29 & 1, content >> 28 & 1);
     } else {
       if(getNumberOfDecimalDigits(pState->regs[i]) >= 10 && regNegative) {
          printf("R%-3d: %11d (0x%.8x)\n", i, content, content);
       } else {
          printf("R%-3d:%11d (0x%.8x)\n", i, content, content);
       }
     }
   // }
  }
}

void printProcessorState(proc_state_t *pState) {
  printRegs(pState);
  printMemory(pState->memory);
}

int ishexdigit(char c) {
   return isdigit(c) || c == 'a' || c == 'b' || c == 'c' || c == 'd' || c == 'e' || c == 'f';
}

/*
  Converts a hex s to a number. s can optionally have 0x as a prefix
 */
int number(char *s) {
  int positive = 1;
  if (s[0] == '-') {
    positive = 0;
    s++;
  }
  if (s[0] == '0' && s[1] == 'x')
    s = s + 2;
  for (int i = 0; s[i] != '\0'; i++)
    if (!ishexdigit(s[i]))
      return -1;
  int ans = (int)strtol(s, NULL, 16);
  return positive ? ans : -ans;
}

static char whitespace[] = " \t\r\n\v";

int getstr(char **ps, char *es, char **str) {
  char *s = *ps;
  while (s < es && strchr(whitespace, *s))
    s++;
  if (*s == 0)
    return 0;
  *str = s;
  while (s < es && !strchr(whitespace, *s))
    s++;
  *s = 0;
  *ps = s;
  //printf("getstr: %s\n", *str);
  return 1;
}

int getcmd(char *buf, int nbuf) {
  char *s = 0;
  fprintf(stderr, "emu $ ");
  fflush(stdout);  
  memset(buf, 0, nbuf);
  s = fgets(buf, nbuf, stdin);
  if(s == 0) // EOF - enter on blank lines terminates shell
    return -1;
  buf[strcspn(buf, "\n")] = '\0';
  return 0;
}

int getscriptcmd(char *buf, int nbuf, FILE *fp) {
  char *s = 0;
  memset(buf, 0, nbuf);
  s = fgets(buf, nbuf, fp);
  if(s == 0) // EOF - enter on blank lines terminates shell
    return -1;
  buf[strcspn(buf, "\n")] = '\0';
  return 0;
}

int my_random(int start, int end) { // assumes srand has been called
    return rand() % (end-start+1) + start;
}

static char *cmdargv[MAXARGS];
static int mem_dump_addr = 0;
static int mem_dump_len = MEM_DUMP_LEN;
static int mem_list_addr = 0;
static int mem_list_len = MEM_DUMP_LEN;

int do_cmd(int argc, char **cmdargv, proc_state_t *pState, pipeline_t *pipeline) {
  bool finished = false;
  int retval = 1;
  if (cmdargv[0][0] == 'r') {
    if (argc == 3) { // cmd is r 0 55, modify r0 to be 55
      int reg = number(cmdargv[1]);
      int rval = 0, val = 0;
      if (cmdargv[2][0] == 'r') { // generate a random 32-bit value
        if (cmdargv[2][1] == '1') // random number between 0 and 100
          rval = my_random(0, 100);
        else if (cmdargv[2][1] == '2') // random number between 0 and 200
          rval = my_random(0, 200);
        else if (cmdargv[2][1] == '-' && cmdargv[2][2] == '1') // negative random number between 0 and 100
          rval = -my_random(0, 100);
        else if (cmdargv[2][1] == '-' && cmdargv[2][2] == '2') // negative random number between 0 and 200
          rval = -my_random(0, 200);
        else // 32-bit random number
          rval = rand(); //0xfeedabee;
      }
      else
        val  = number(cmdargv[2]);
      if (reg == -1 || reg > 15)
        fprintf(stderr, "%s\n", "invalid reg on r command.");
      else {
        pState->regs[reg] = rval ? rval : val;
        printf("R%-3d:%11d (0x%.8x)\n", reg, pState->regs[reg], pState->regs[reg]);
      }
    } else if (argc == 2) { // cmd is r 6, display r6
      int reg = number(cmdargv[1]);
      if (reg == -1 || reg > 15)
        fprintf(stderr, "%s\n", "invalid reg on r command.");
      else
        printf("R%-3d:%11d (0x%.8x)\n", reg, pState->regs[reg], pState->regs[reg]);
    } else // print all regs
      printRegs(pState);
  } else if (cmdargv[0][0] == 's' && cmdargv[0][1] == 't') {
    printProcessorState(pState);
  } else if (cmdargv[0][0] == 's') {
    int ss = 0;
    if (cmdargv[0][1] == 's')
      ss = 1;
    if (cmdargv[0][2] == 's')
      ss = 2;
    if (argc == 2) { // format is s num
      int steps = number(cmdargv[1]);
      if (steps == -1 || steps > 100)
        fprintf(stderr, "%s\n", "invalid number of steps on s command.");
      else
        for (int i = 0; i < steps; i++) {
          finished = step(pState, pipeline, ss, ss);
          if (finished == 1) // breakpoint hit
            break;
        }
    }
    else
      finished = step(pState, pipeline, ss, 1);
  } else if (cmdargv[0][0] == 'd') {
    int t = 0;
    if (argc > 1) { // at least d addr
      t = number(cmdargv[1]);
      mem_dump_addr = t >= 0 ? t/4 : mem_dump_addr;
    }
    if (argc > 2) { // we have d addr len
      t = number(cmdargv[2]);
      mem_dump_len = t >= 0 ? t : mem_dump_len;
    }
    memory_dump(pState, mem_dump_addr, mem_dump_len);
    mem_dump_addr += mem_dump_len;
  } else if (cmdargv[0][0] == 'l' && cmdargv[0][1] == 'd') {
    if (argc != 3) { // must issue ld filename addr
      fprintf(stderr, "%s\n", "format is ld filename addr");
    } else {
      int addr  = number(cmdargv[2]);
      if (addr < 0)
        fprintf(stderr, "%s\n", "invalid address on ld command.");
      else {
        FILE *file = fopen(cmdargv[1], "rb");
        memoryLoader(file, pState, addr/4);
      }
    }
  } else if (cmdargv[0][0] == 'l') {
    int t = 0;
    if (argc > 1) { // at least l addr
      t = number(cmdargv[1]);
      mem_list_addr = t >= 0 ? t/4 : mem_list_addr;
    }
    if (argc > 2) { // we have l addr len
      t = number(cmdargv[2]);
      mem_list_len = t >= 0 ? t : mem_list_len;
    }
    memory_list(pState, mem_list_addr, mem_list_len);
    mem_list_addr += mem_list_len;
  } else if (cmdargv[0][0] == 'm') {
    if (argc == 3) { // format is m addr value
      int addr = number(cmdargv[1]);
      int rval = 0, val = 0;
      if (cmdargv[2][0] == 'r') { // generate a random 32-bit value
        if (cmdargv[2][1] == '1') // random number between 0 and 100
          rval = my_random(0, 100);
        else if (cmdargv[2][1] == '2') // random number between 0 and 200
          rval = my_random(0, 200);
        else if (cmdargv[2][1] == '-' && cmdargv[2][2] == '1') // negative random number between 0 and 100
          rval = -my_random(0, 100);
        else if (cmdargv[2][1] == '-' && cmdargv[2][2] == '2') // negative random number between 0 and 200
          rval = -my_random(0, 200);
        else // 32-bit random number
          rval = rand(); //0xfeedabee;
      }
      else
        val  = number(cmdargv[2]);
      if (addr < 0)
        fprintf(stderr, "%s\n", "invalid address on m command.");
      else
        pState->memory[addr / 4] = rval ? rval : val;
    }
    else
      fprintf(stderr, "%s\n", "format is m addr value");
  } else if (cmdargv[0][0] == 'p' && cmdargv[0][1] == 'l') {
    if (argc > 1) { // pl 300 - set state to begin execution at addr 300
      int addr = number(cmdargv[1]);
      if (addr == -1 || addr > MEM_SIZE_WORDS-mem_list_len) {
        fprintf(stderr, "%s\n", "invalid addr on pl command.");
      }
      else {
        pState->PC = addr+4;
        pState->regs[INDEX_PC] = pState->PC;
        pipeline->fetched = pState->memory[addr/4];
        pipeline->decoded = -1;
      }
    } else
      printPipeline(pipeline, pState);
  } else if (cmdargv[0][0] == 'b') {
    if (argc > 1) { // b addr - set a break point
      int addr = number(cmdargv[1]);
      if (addr < 0)
        pipeline->breakpoint = -1;
      else
        pipeline->breakpoint = addr;
    } else
      printf("breakpoint: 0x%08x\n", pipeline->breakpoint);
  } else if (cmdargv[0][0] == 'c' && cmdargv[0][1] == 'p') {
    if (argc != 3) {
      fprintf(stderr, "%s\n", "format is cp addr string");
    }
    else {
      int addr = number(cmdargv[1]);
      if (addr == -1 || addr > MEM_SIZE_WORDS-mem_list_len) {
       fprintf(stderr, "%s\n", "invalid addr on cp command.");
      }
      else {
        int chars[16] = {0}; 
        strcpy((char *)chars, cmdargv[2]);
        int sl = strlen(cmdargv[2]);
        for (int i = 0; i < (sl + 3)/4; i++) {
          pState->memory[addr / 4] = chars[i];
          addr += 4;
        }
      }
    } 
  } else if (cmdargv[0][0] == '.') {
    for (int i = 0; i < strlen(cmdargv[0]); i++) // cp the null char also
      cmdargv[0][i] = cmdargv[0][i+1];
    if (fork() == 0) 
      execvp(cmdargv[0], cmdargv);
    wait(NULL);
  } else if (cmdargv[0][0] == 'q') {
    retval = 0;
  }
  else {
    fprintf(stderr, "%s\n", "Invalid command.");
  }
  return retval;
}

void do_script(char *scriptfilename, proc_state_t *pState, pipeline_t *pipeline) {
  printf("< redirect from script. file: %s\n", scriptfilename);
  cmdargv[0] = ""; // prevent seg fault when first cmd is empty line
  FILE *fp = fopen(scriptfilename, "r");
  if (fp != NULL) {
    char command[100];
    while (getscriptcmd(command, sizeof(command), fp) >= 0) {
      char *str;
      char *s = command;
      char *es = s + strlen(s);
      int argc = 0;
      cmdargv[0] = "";
      while (getstr(&s, es, &str) != 0) {
        cmdargv[argc] = str;
        //printf("%s\n", cmdargv[argc]);
        argc++;
      }
      cmdargv[argc] = 0;
      if (argc > 0) {
        if (cmdargv[0][0] != '<') {
          if (do_cmd(argc, cmdargv, pState, pipeline) == 0)
            break;
        }
      }
    }
  }
  else {
    printf("File %s not found.\n", cmdargv[1]);
  }
}
/*
 Commands - all numbers are hex regardless of prefix. You can omit 0x or prepend with 0x, either way > hex 
 Addresses are truncated to previous addr that is a multiple of 4. E.g., 105 becomes 104
 r - show regs
 r 1 - show reg 1
 r 1 10 - modify reg 1, put 16 into r1
 r 1 r - modify reg 1 with a random 32-bit number
 r 1 rN - modify reg 1 with a random number
        - N=1, random number between 0 and 100
        - N=2, random number between 0 and 200
        - N=-1, negative random number between 0 and 100
        - N=-2, negative random number between 0 and 200
 s - step, does a fetch, decode, execute of one instruction
 s 3 - step 3 times using s, maximum of 100 steps
 ss - step and show regs that changed
 ss 3 - step 3 times using ss
 sss - step and show regs and user memory that changed
 st - show computer state
 d - dump 8 words of memory, starting at where last dump ended
 d 500 - dump 8 words of memory starting at 500
 d 500 20 - dump 20 words of memory starting at 500
 l - list 8 words of memory, starting at where last list ended, list disassembles 32-bits
 l 500 - list 8 words of memory starting at 500
 l 500 20 - list 20 words of memory starting at 500
 m 100 606 - modify memory address 100 with 606, addr is truncated, 105 becomes 104
 m 100 r - modify memory address 100 with a random 32-bit number
 m 100 rN - modify memory address 100 with a random number
          - N=1, random number between 0 and 100
          - N=2, random number between 0 and 200
          - N=-1, negative random number between 0 and 100
          - N=-2, negative random number between 0 and 200
 pl - show pipeline
 pl 400 - set pipeline to begin execution at address 400
 cp 500 abc - copy abc to address 500, null terminated
 ld file.o 100 - load file.o into memory beginning at addres 100
 .ls - run the ls command as a child process, . prefixes Linux commands
 < file.emu - run the script file.emu
 */
void gustyCycle(proc_state_t *pState, pipeline_t *pipeline) {
  //pipeline_t pipeline = {-1, -1, -1};
  //bool finished = false;
  // Initialisation
  //pState->PC = 4;
  //pState->regs[INDEX_PC] = pState->PC;
  //pipeline.fetched = pState->memory[0];
  // PC is stored twice in pState(regs array and separate field)
  char command[100];
  while (getcmd(command, sizeof(command)) >= 0) {
    char *s = command;
    char *es = s + strlen(s);
    char *str;
    int argc = 0;
    cmdargv[0] = ""; // prevent seg fault when first cmd is empty line
    while (getstr(&s, es, &str) != 0) {
      cmdargv[argc] = str;
      //printf("%s\n", cmdargv[argc]);
      argc++;
    }
    cmdargv[argc] = 0;
    if (argc > 0) {
      if (cmdargv[0][0] != '<') {
        if (do_cmd(argc, cmdargv, pState, pipeline) == 0)
          break;
      }
      else {
        do_script(cmdargv[1], pState, pipeline);
      }
    }
  }
}

/*
  $ emu code.o : run emu and load code.o at address 0
  $ emu -o code.o : run emu, load OS, load code.o at address 0, SP is set 
  $ emu -o code.o arg1 arg2 : run emu, load os, load code.o at address 0, arg1/2 passed on stack
  $ emu --OS newos.o code.o arg1 arg2 : like prev, but newos.o is loaded
  -o or --os : load OS as part of emu, os is in os.o
  -f file.o or --OS file.o : load OS as part of emu. OS in file.o
  The proc_state_t member memory[] is an array of 16384 words, 65K bytes of memory.
  65K is loads of memory. For now, we keep everything below 8192.
  Memory Layout
  0 - 4095 : user space
  0 - 0xFFF : user space
  4096 - 8192 : os
  0x1000 - 0x1FFF : os
  16384 : Start of Stack, grows down (toward 0)
  0x4000 : Start of Stack, grows down (toward 0)
  8192 : Start of Heap, grows up (toward 65K-1)
  0x2000 : Start of Heap, grows up (toward 65K-1)
 */
static struct option long_options[] = {
  {"os",     no_argument,       0,  'o' },
  {"OS",     required_argument, 0,  'f' },
  {"script", required_argument, 0,  's' },
  {0,        0,                 0,   0  }
};

static char *osfilename = "no os";
static char *scriptfilename = "no script";

int main(int argc, char **argv) {
  int load_os = 0, use_script = 0;
  int opt, long_index;
  while ((opt = getopt_long(argc, argv, "of:s:", long_options, &long_index )) != -1) {
    switch (opt) {
      case 'o': 
        load_os = 1;
        osfilename = "os.o";
        break;
      case 'f': 
        load_os = 1;
        osfilename = optarg; 
        break;
      case 's':
         use_script = 1;
         scriptfilename = optarg;
         break;
      default: 
        printf("Invalid invocation.\n");
        exit(EXIT_FAILURE);
    }
  }
  argc -= optind;
  argv += optind;
  printf("load os: %d, os filename: %s, .o filename: %s, scriptfilename: %s\n", load_os, osfilename, argv[0], scriptfilename);
  if(!argv[0]) {
    fprintf(stderr, "%s optind: %d argc: %d\n", "Wrong number of arguments", optind, argc);
    return EXIT_FAILURE;
  }
  //fprintf(stdout, "optind: %d argc: %d\n", optind, argc);
  proc_state_t *pStatePtr = (proc_state_t *) malloc(sizeof(proc_state_t));
  // Initialize state to 0
  pStatePtr->NEG = 0;
  pStatePtr->ZER = 0;
  pStatePtr->CRY = 0;
  pStatePtr->OVF = 0;
  pStatePtr->PC = 0;
  for(int i = 0; i < MEM_SIZE_WORDS; i++) {
    pStatePtr->memory[i] = 0;
  }
  for(int i = 0; i < NUMBER_REGS; i++) {
    pStatePtr->regs[i] = 0;
  }
  //End Initialisation
  if(!pStatePtr) {
    perror("calloc");
    exit(EXIT_FAILURE);
  }
  if (load_os) {
    FILE *osf = fopen(osfilename, "rb");
    if (osf == NULL) {
      fprintf(stderr, "File not found: %s\n", osfilename==NULL ? "os filename" : osfilename);
      return EXIT_FAILURE;
    }
    memoryLoader(osf, pStatePtr, OS_ADDR/4);
    pStatePtr->regs[13] = STACK_ADDR;
    pStatePtr->memory[0x2000/4] = 0x2004; // heap free memory pointer
  }
  FILE *file = fopen(argv[0], "rb");
  if (file == NULL) {
    fprintf(stderr, "File not found: %s\n", argv[0]);
    return EXIT_FAILURE;
  }
  memoryLoader(file, pStatePtr, 0);
  pipeline_t pipeline = {-1, -1, -1};
  // Initialize pipeline
  pStatePtr->PC = 4;
  pStatePtr->regs[INDEX_PC] = pStatePtr->PC;
  pipeline.fetched = pStatePtr->memory[0];
  if (use_script) {
    do_script(scriptfilename, pStatePtr, &pipeline);
  }
  srand(time(0)); // initialize for rand() to work
  gustyCycle(pStatePtr, &pipeline);
  free(pStatePtr);
  return EXIT_SUCCESS;
}
