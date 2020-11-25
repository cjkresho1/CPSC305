#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <getopt.h>
#include "adts.h"

#define NDEBUG

/**
 * Define macros for assembler
 */
#define MAX_LINE_LENGTH 512
#define DELIMITERS " ,\t\n"
#define MEMORY_SIZE 4
#define PC_OFFSET 2
#define INSTRUCTION_SIZE 32
#define ALWAYS_CONDITION ""
#define BRANCH_OFFSET_SIZE  26
#define NUMBER_OF_LINES 1000

/**
 * Types of tokens
 */
typedef enum {INSTRUCTION, LABEL, EXPRESSION_TAG,
              EXPRESSION_EQUAL, REGISTER, UNDEFINED} typeEnum;

/**
 * Define map data structures and functions to full them.
 */
map DATA_OPCODE;
map ALL_INSTRUCTIONS;
map CONDITIONS;
map DATA_TYPE;
map SHIFTS;

/**
 * Returns a map with all the Data Processing instructions from the assembler
 * and maps them to their respective opcode (bits 24, 23, 22, 21)
 */
map fillDataToOpcode(void) {
  map m = constructMap();
  put(&m, "add", 4);
  put(&m, "sub", 2);
  put(&m, "rsb", 3);
  put(&m, "and", 0);
  put(&m, "eor", 1);
  put(&m, "orr", 12);
  put(&m, "mov", 13);
  put(&m, "tst", 8);
  put(&m, "teq", 9);
  put(&m, "cmp", 10);
  return m;
}

/**
 * Returns a map with all instructions mapped to their respective type:
 * 0 Data Processing
 * 1 Multiply
 * 2 Single Data Transfer
 * 3 Branch
 * 4 Shifts
 * 5 Special andeq r0, r0, r0
 */
map fillAllInstructions(void) {
  map m = constructMap();
  // 0 Data Processing
  put(&m, "add", 0);
  put(&m, "sub", 0);
  put(&m, "rsb", 0);
  put(&m, "and", 0);
  put(&m, "eor", 0);
  put(&m, "orr", 0);
  put(&m, "mov", 0);
  put(&m, "tst", 0);
  put(&m, "teq", 0);
  put(&m, "cmp", 0);
  // 1 Multiply
  put(&m, "mul", 1);
  put(&m, "mla", 1);
  // 2 Single Data Transfer
  put(&m, "ldr", 2);
  put(&m, "str", 2);
  // 3 Branch
  put(&m, "beq", 3);
  put(&m, "bne", 3);
  put(&m, "bge", 3);
  put(&m, "blt", 3);
  put(&m, "bgt", 3);
  put(&m, "ble", 3);
  put(&m, "b", 3);
  put(&m, "bl", 3); // gusty
  //put(&m, "bx", 3); // gusty
  // 4 Shifts
  put(&m, "lsl", 4);
  put(&m, "lsr", 4);
  put(&m, "asr", 4);
  put(&m, "ror", 4);
  // 5 Special
  put(&m, "andeq", 5);
  // 6 SVC
  put(&m, "svc", 6);
  return m;
}

/**
 * Returns a map containing all conditions from the cond field of the
 * instruction with their respective code
 */
map fillConditions(void) {
  map m = constructMap();
  put(&m, "eq", 0);
  put(&m, "ne", 1);
  put(&m, "ge", 10);
  put(&m, "lt", 11);
  put(&m, "gt", 12);
  put(&m, "le", 13);
  put(&m, "", 14);
  return m;
}

/**
 * Returns a map that classifies all Data Processing instructions:
 * 0 Instructions that compute results
 * 1 Mov instruction
 * 2 Instructions that set flags
 */
map fillDataToType(void) {
  map m = constructMap();
  // instructions that compute results and, eor, sub, rsb, add, orr
  put(&m, "and", 0);
  put(&m, "eor", 0);
  put(&m, "sub", 0);
  put(&m, "rsb", 0);
  put(&m, "add", 0);
  put(&m, "orr", 0);
  // single operand instruction mov
  put(&m, "mov", 1);
  // Instructions that do not compute results,
  // but do set the CPSR flags: tst, teq, cmp
  put(&m, "tst", 2);
  put(&m, "teq", 2);
  put(&m, "cmp", 2);
  return m;
}

/**
 * Returns a map with all 4 shifts mapped to their respective code
 */
map fillShifts(void) {
  map m = constructMap();
  put(&m, "lsl", 0);
  put(&m, "lsr", 1);
  put(&m, "asr", 2);
  put(&m, "ror", 3);
  return m;
}

void fillAll(void) {
  DATA_OPCODE      = fillDataToOpcode();
  ALL_INSTRUCTIONS = fillAllInstructions();
  CONDITIONS       = fillConditions();
  DATA_TYPE        = fillDataToType();
  SHIFTS           = fillShifts();
}

void freeAll(void) {
  clearMap(&DATA_OPCODE);
  clearMap(&ALL_INSTRUCTIONS);
  clearMap(&CONDITIONS);
  clearMap(&DATA_TYPE);
  clearMap(&SHIFTS);
}

/**
 * Define function prototypes for forward references.
 * I will eventually reorder the code so these are not needed
 */
 /**
* Returns a char array that contains all the program lines
* Maps all labels with their respective memory location
* Finds the number of the instructions and returns it through instructionsNumber
* Finds the number of ldr instructions and returns it through ldrCount
* Finds the number of lines and returns it through lineNumber
* Throws any errors occour during the first pass such as multiple definitions
* of the same label
**/
char **firstPass(FILE *input, map *labelMapping, vector *errorVector,
        uint32_t *instructionsNumber, uint32_t *ldrCount, uint32_t *lineNumber);

/**
* Fills the instrcutions array with all the decode instrcutions
* Throws any errors encountered during the pass
**/
void secondPass(uint32_t *instructionsNumber, uint32_t instructions[],
              vector *errorVector, map labelMapping,
              char** linesFromFile, uint32_t lineNumber);
// --------------------DECODING FUNCTIONS-------------------------
/* Main decode function which returns the decoded instruction */
uint32_t decode(vector *tokens, vector *addresses, uint32_t instructionNumber,
  uint32_t instructionsNumber, map labelMapping, vector *errorVector, char *ln);

/* Decodes any Data Processing Instruction */
uint32_t decodeDataProcessing(vector *tokens, vector *errorVector, char *ln);

/* Decodes any Multiply Instruction */
uint32_t decodeMultiply(vector *tokens, vector *errorVector, char *ln);

/* Decodes any Single Data Transfer Instruction */
uint32_t decodeSingleDataTransfer(vector *tokens, vector *addresses,
                uint32_t instructionNumber, uint32_t instructionsNumber,
                vector *errorVector, char *ln);

/* Decodes any Branch Instruction */
uint32_t decodeBranch(vector *tokens, uint32_t instructionNumber,
                map labelMapping, vector *errorVector, char *ln);

/* Decodes any Shift Instruction */
uint32_t decodeShift(vector *tokens, vector *errorVector, char *ln);

/**
* Creates a vector with all of the tokens of the original string
* without modifing it with respect to the delimiters (eg. " ,.")
**/
vector tokenise(char *start, char *delimiters);

// ---------------------TYPE FUNCTIONS-------------------------
/* Returns the type of the passed token */
typeEnum getType(char *token);

/* Helpers for type */
bool isInstruction(char *token);
bool isLabel(char *token);
bool isRegister(char *token);
bool isShift(char *token);
typeEnum isExpression(char *token);

/* Gets the shift type and applays the shift rules to the operand parameter */
void getShift(vector *tokens, uint32_t *operand, vector *errorVector, char *ln);

/**
 * Gets expressions of the type [register], [register, register],
 * [register, register, shift] (everything that is related to memory access)
 */
void getBracketExpr(vector *tokens, int *rn, int32_t *offset, int *i, int *u, int *p, int *w,
                    vector *errorVector, char *ln);
/**
 * Gets expression of type <#expression> or <=expression> and throws errors
 * if the value in the exp can't be represented
 */
int32_t getExpression(char *exp, vector *errorVector, char *ln);

/* Gets hexadecimal value from string */
int32_t getHex(char *exp);

/* Gets decimal value from string */
int32_t getDec(char *exp);

/* Clears the vector tokens if the token '@' is found (i.e. a comment) */
void getComment(vector *tokens);

// ------------------------HELPERS-------------------------------
/* Converts unsigned int to string */
char *uintToString(uint32_t num);

/* Checks format of registers and throws error if the format is invalid */
bool checkReg(vector *tokens, char *instr,
                vector *errorVector, char *ln);

/* Sets the cond field of the instruction */
void setCond(uint32_t *x, char *cond);

/* Frees the matrix of lines */
void clearLinesFromFile(char **linesFromFile);

// ----------------------ERRORS--------------------------------
/**
* All of the error functions append an error message to the errorVector
* which will be printed at the end of the program if there are any errors
**/
void throwUndefinedError(char *name, vector *errorVector, char *ln);
void throwLabelError(char *name, vector *errorVector, char *ln);
void throwExpressionError(char *expression, vector *errorVector, char *ln);
void throwRegisterError(char *name, vector *errorVector, char *ln);
void throwExpressionMissingError(char *ins, vector *errorVector, char *ln);

// -----------------------DEBUGGING---------------------------
void printStringArray(int n, char arr[][MAX_LINE_LENGTH]);
void printBinary(uint32_t nr);

/* Global variable */
int countDynamicExpansions = 1;

/*
  $ asm code.s                 // creates code.o and code.txt
  $ asm -o a.o code.s          // creates a.o and code.txt
  $ asm -t b.txt code.s        // creates code.o and b.txt
  $ asm -o a.o -t b.txt code.s // creates a.o and b.txt
  NOTE: The following creates a obj file named -t
  $ asm -o -t code.s // creates -t and code.txt
 */
static struct option long_options[] = {
  {"obj",    no_argument,       0,  'o' },
  {"txt",    required_argument, 0,  't' },
  {0,        0,                 0,   0  }
};

static int DEBUG = 0;

int main(int argc, char **argv) {
  char *objfilenameptr = NULL, *txtfilenameptr = NULL;
  char objfilename[128], txtfilename[128];
  int opt, long_index;
  
  while ((opt = getopt_long(argc, argv, "o:t:", long_options, &long_index )) != -1) {
    switch (opt) {
      case 'o': 
        objfilenameptr = optarg;
        break;
      case 't': 
        txtfilenameptr = optarg; 
        break;
      default: 
        printf("Invalid invocation.\n");
        exit(EXIT_FAILURE);
    }
  }
  argc -= optind;
  argv += optind;
  if(!argv[0]) {
    fprintf(stderr, "%s optind: %d argc: %d\n", "Wrong number of arguments", optind, argc);
    return EXIT_FAILURE;
  }

  char *srcfilename = argv[0];
  char *ext = strrchr(srcfilename, '.');
  if (ext == NULL || strlen(ext) != 2 || (ext != NULL && (ext[0] != '.' || ext[1] != 's'))) {
    fprintf(stderr, "Assembly file must have .s extension!\n");
    exit(EXIT_FAILURE);
  }
  int dotpos = ext - srcfilename;
  for (int i = 0; i < dotpos; i++) {
    objfilename[i] = argv[0][i];
    txtfilename[i] = argv[0][i];
  }
  objfilename[dotpos] = '\0';
  txtfilename[dotpos] = '\0';
  strcat(objfilename, ".o");
  strcat(txtfilename, ".txt");
  if (objfilenameptr == NULL)
    objfilenameptr = objfilename;
  if (txtfilenameptr == NULL)
    txtfilenameptr = txtfilename;

  printf(".s: %s, .o: %s, .txt: %s\n", argv[0], objfilenameptr, txtfilenameptr);

  FILE *input = fopen(argv[0], "r");
  // check file existance throw error if not found
  if (!input) {
    fprintf(stderr, "The file %s was not found\n", argv[1]);
    exit(EXIT_FAILURE);
  }

  vector errorVector = constructVector();
  uint32_t instructionsNumber;
  map labelMapping = constructMap();
  /**
  * fill the mappings
  * And after:
  * make first pass thorugh code and map all labels with their corresponidng
  * memmory addresses (fills labelMapping)
  **/
  fillAll();
  uint32_t ldrCount = 0;
  uint32_t lineNumber = 1;
  char **linesFromFile;
  linesFromFile = firstPass(input, &labelMapping, &errorVector,
                  &instructionsNumber, &ldrCount, &lineNumber);
  // Have checked not NULL condition in firstPass function
  /**
  * Make second pass now and replace all labels with their mapping
  * also decode all instructions and throw errors if any
  **/
  uint32_t instructions[instructionsNumber + ldrCount];
  rewind(input); // reset file pointer to the beginning of the file
  secondPass(&instructionsNumber, instructions,
             &errorVector, labelMapping, linesFromFile, lineNumber);

  // clear
  freeAll();
  clearMap(&labelMapping);
  fclose(input);

  // if we have compile erros stop and print errors
  if (!isEmptyVector(errorVector)) {
    while (!isEmptyVector(errorVector)) {
      char *error = getFront(&errorVector);
      fprintf(stderr, "%s\n", error);
      free(error);
    }

    clearVector(&errorVector);
    exit(EXIT_FAILURE);
  }

  FILE *output = fopen(objfilenameptr, "wb");
  fwrite(instructions, sizeof(uint32_t), instructionsNumber, output);
  fclose(output);
  FILE *hex = fopen(txtfilenameptr, "w");
  for (int i = 0; i < instructionsNumber; i++)
    fprintf(hex, "0x%08x\n", instructions[i]);
  fclose(hex);
  clearVector(&errorVector);
  clearLinesFromFile(linesFromFile);
  exit(EXIT_SUCCESS);
}

char **firstPass(FILE *input, map *labelMapping, vector *errorVector,
      uint32_t *instructionsNumber, uint32_t *ldrCount, uint32_t *lineNumber) {
  uint32_t currentMemoryLocation = 0;
  *ldrCount = 0;
  vector currentLabels = constructVector();
  char buffer[MAX_LINE_LENGTH];
  /*Allocate memory for the array of lines*/
  char **linesFromFile = (char **) malloc(NUMBER_OF_LINES * sizeof(char *));
  for(int i = 0; i < NUMBER_OF_LINES; i++) {
    linesFromFile[i] = (char *) malloc((MAX_LINE_LENGTH + 1) * sizeof(char));
    if(!linesFromFile) {
      perror("malloc");
      exit(EXIT_FAILURE);
    }
  }
  if(!linesFromFile) {
    perror("malloc");
    exit(EXIT_FAILURE);
  }
  /*End memory allocation for array of lines*/
  *instructionsNumber = 0;

  while(fgets(buffer, MAX_LINE_LENGTH, input)) {
    /*Perform dynamic expansion of lines array*/
    if((*lineNumber - 1) >= NUMBER_OF_LINES) {
     countDynamicExpansions++;
     //increment global variable countDynamicExpansions
     char **reallocatedArray =
     realloc(linesFromFile,
             countDynamicExpansions * NUMBER_OF_LINES * sizeof(char *));
             printf("%s\n", "I reallocated");
     if(reallocatedArray) {
        linesFromFile = reallocatedArray;
     } else {
       perror("realloc");
       exit(EXIT_FAILURE);
     }
    }
    /*End dynamic expansion*/
    strcpy(linesFromFile[*lineNumber - 1], buffer);
    vector tokens = tokenise(buffer, DELIMITERS);
    char *lineNo = uintToString(*lineNumber);
    // check for all tokens see if there are labels
    // if there are labels add all of them to a vector list and
    // map all labels with the memorry address of the next instruction
    while (!isEmptyVector(tokens)) {
      char *token = peekFront(tokens);

      if (getType(token) == LABEL) {
        token[strlen(token) - 1] = '\0';
        if (get(*labelMapping, token) || contains(currentLabels, token)) {
          // if this label already exists in the mapping this means
          // that we have multiple definitions of the same label
          // therefore throw an error message
          throwLabelError(token, errorVector, lineNo);
        } else {
          putBack(&currentLabels, token);
        }
      }

      if (getType(token) == INSTRUCTION) {
        // if the instruction is a ldr instruction and the <=expression>
        // is more than 0xFF we need to store the value at the bottom of the
        // binary file
        // we will assume that the argument is more than 0xFF and if it is
        // not then we will just not use the remaining space
        if (!strcmp(token, "ldr")) {
          (*ldrCount)++;
        }

        // if we found a valid instruction
        // map all current unmapped labels
        // to this current memmory location
        // and advance memory
        while (!isEmptyVector(currentLabels)) {
          // map all labels to current memorry location
          char *label = getFront(&currentLabels);
          put(labelMapping, label, currentMemoryLocation);
          free(label);
        }
        currentMemoryLocation++;
        (*instructionsNumber)++;
      }

      getComment(&tokens);

      free(getFront(&tokens));
    }
    free(lineNo);
    (*lineNumber)++;
  }

  // map all remaining unmached labels to current memory location
  while (!isEmptyVector(currentLabels)) {
    // map all labels to current memorry location
    put(labelMapping, getFront(&currentLabels), currentMemoryLocation);
  }
  return linesFromFile;
}

void secondPass(uint32_t *instructionsNumber, uint32_t instructions[],
              vector *errorVector, map labelMapping,
              char **linesFromFile, uint32_t lineNumber) {
  //char buffer[MAX_LINE_LENGTH];
  uint32_t PC = 0;
  uint32_t ln = 1;
  vector addresses = constructVector();
  while(ln != lineNumber) {
    vector tokens = tokenise(linesFromFile[ln - 1], DELIMITERS);
    char *lineNo = uintToString(ln);
    while (!isEmptyVector(tokens)) {
      char *token = peekFront(tokens);
      if (DEBUG) {
      printf("In secondpass\n"); // gusty
      printVector(tokens); // gusty
      }
      if (getType(token) == INSTRUCTION) {
        // if there is a valid isntruction decode it and increase
        // instruction counter
        instructions[PC] = decode(&tokens, &addresses, PC, *instructionsNumber,
                      labelMapping, errorVector, lineNo);
        PC++;
      } else if (getType(token) == LABEL) {
        // we have a label so we just remove it
        free(getFront(&tokens));
      } else {
	getComment(&tokens);
	if (!isEmptyVector(tokens)) {
          throwUndefinedError(token, errorVector, lineNo);
          // throw error because instruction is undefined
          free(getFront(&tokens));
        }
      }
    }
    ln++;
    free(lineNo);
  }

  // put all ldr addresses > 0xFF at the end of the file
  while (!isEmptyVector(addresses)) {
    char *address = getFront(&addresses);
    instructions[PC] = getExpression(address, NULL, 0);
    free(address);
    PC++;
  }

  *instructionsNumber = PC;
}

void getComment(vector *tokens) {
  char *token = peekFront(*tokens);
  if (token[0] == '@' || (token[0] == '/' && token[1] == '/')) {
    clearVector(tokens);
  }
}

uint32_t decode(vector *tokens, vector *addresses, uint32_t instructionNumber,
                uint32_t instructionsNumber, map labelMapping,
                vector *errorVector, char *ln) {
  uint32_t type = *get(ALL_INSTRUCTIONS, peekFront(*tokens));
  char *token = NULL;
  switch (type) {
    case 0: return decodeDataProcessing(tokens, errorVector, ln);
    case 1: return decodeMultiply(tokens, errorVector, ln);
    case 2: return decodeSingleDataTransfer(tokens, addresses,
                        instructionNumber, instructionsNumber, errorVector, ln);
    case 3: return decodeBranch(tokens, instructionNumber,
                                      labelMapping, errorVector, ln);
    case 4: return decodeShift(tokens, errorVector, ln);
    case 5: // andeq r0,r0,r0 we just free 4 times
            free(getFront(tokens));
            free(getFront(tokens));
            free(getFront(tokens));
            free(getFront(tokens));
            return 0;
    case 6: token = getFront(tokens); // SVC instruction
            free(token);
            token = getFront(tokens);
            int res = 0;
            if (token[0] != '#') {
              printf("invalid SVC operand\n");
              throwExpressionError(token, errorVector, ln);
            } else if (token[1] == '0' && token[2] == 'x') {
              res = getHex(token + 1);
            } else {
              res = getDec(token + 1);
            }
            free(token);
            return 0xef000000 | res;
    default: //assert(false);
             free(getFront(tokens));
             return -1;
  }
}

uint32_t decodeDataProcessing(vector *tokens, vector *errorVector, char *ln) {
  char *instruction = getFront(tokens);
  uint32_t ins = 0;
  uint32_t opcode = *get(DATA_OPCODE, instruction) << 0x15;
  setCond(&ins, ALWAYS_CONDITION);
  // set opcode
  ins |= opcode;
  uint32_t dataType = *get(DATA_TYPE, instruction);
  /*
    dataType
    0 instructions that compute results and, eor, sub, rsb, add, orr
    1 single operand mov instruction
    2 Instructions that do not compute results, but set the CPSR flags: tst, teq, cmp
   */
  uint32_t rd = 0;
  uint32_t rn = 0;
  uint32_t operand2 = 0;
  uint32_t i = 0;
  char *token;
  if (dataType != 2) {
    // we have either 0, 1 type instruction
    if (checkReg(tokens, instruction, errorVector, ln)) {
      token = getFront(tokens);
      rd = getDec(token + 1) << 0xC;
      free(token);
    }
  }

  if (dataType != 1) {
    // we have either 0, 2 type instruction
    if (checkReg(tokens, instruction, errorVector, ln)) {
      token = getFront(tokens);
      rn = getDec(token + 1) << 0x10;
      free(token);
    }
  }

  token = peekFront(*tokens);
  if (getType(token) != EXPRESSION_TAG &&
      getType(token) != EXPRESSION_EQUAL &&
            getType(token) != REGISTER) {
    // throw expression error
    throwExpressionError(token, errorVector, ln);
    free(instruction);
    return -1;
  }

  if (getType(token) == EXPRESSION_TAG || getType(token) == EXPRESSION_EQUAL) {
    // decode expression and set bit i to 1
    operand2 = getExpression(token, errorVector, ln);
    i = 1;
  } else {
    // we have a register
    operand2 = getDec(token + 1);
  }
  free(getFront(tokens));

  if (dataType == 2) {
    // we have third type of instruction
    // with syntax <opcode> Rn, <Operand2>
    // set S bit to 1
    ins |= 0x1 << 0x14;
  }

  token = peekFront(*tokens);
  if (isShift(token)) {
    getShift(tokens, &operand2, errorVector, ln);
  }

  // set bit I
  ins |= i << 0x19;
  // set rn
  ins |= rn;
  // set rd register
  ins |= rd;
  // set operand2
  ins |= operand2;

  free(instruction);

  return ins;
}

uint32_t decodeMultiply(vector *tokens, vector *errorVector, char *ln) {
  char *multType = (char *) getFront(tokens);
  // set bits 4-7, same for mul and mla
  uint32_t instr = 0x9 << 0x4;
  // set cond
  setCond(&instr, ALWAYS_CONDITION);

  uint32_t acc = 0;
  uint32_t rd = 0;
  uint32_t rm = 0;
  uint32_t rs = 0;
  uint32_t rn = 0;

  // "mul"/"mla" set common registers (rd, rm, rs)
  if(checkReg(tokens, multType, errorVector, ln)) {
    char *token = (char *) getFront(tokens);
    rd = getDec(token + 1) << 0x10;
    free(token);
  }

  if(checkReg(tokens, multType, errorVector, ln)) {
    char *token = (char *) getFront(tokens);
    rm = getDec(token + 1);
    free(token);
  }

  if(checkReg(tokens, multType, errorVector, ln)) {
    char *token = (char *) getFront(tokens);
    rs = getDec(token + 1) << 0x8;
    free(token);
  }

 // "mla" instr case
  if(!strcmp(multType, "mla")) {
    //set bit 21 (Accumulator)
    acc = 0x1 << 0x15;

    if(checkReg(tokens, multType, errorVector, ln)) {
      char *token = (char *) getFront(tokens);
      rn = getDec(token + 1) << 0xC;
      free(token);
    }
  }

  // set acc
  instr |= acc;
  // set rd
  instr |= rd;
  // set rm
  instr |= rm;
  // set rs
  instr |= rs;
  // set rn
  instr |= rn;

  free(multType);

  return instr;
}

// helper function to check if incoming token is a valid register
bool checkReg(vector *tokens, char *instr,
                vector *errorVector, char *ln) {
  char *reg = (char *) peekFront(*tokens);
  if(!reg || getType(reg) == INSTRUCTION) {
    throwExpressionMissingError(instr, errorVector, ln);
    return false;
  }

  if (getType(reg) != REGISTER) {
    throwRegisterError(reg, errorVector, ln);
    free(getFront(tokens));
    return false;
  }

  return true;
}

/* Process an ldr/str r0,[] expression - the bracket part 
   *tokens - examples
     token[0]: "[r0", token[1]: "#4]
     token[0]: "[r0", token[1]: "#-4]
     token[0]: "[r0", token[1]: "#4]!"
     token[0]: "[r0", token[1]: "#-4]!"
     token[0]: "[r0", token[1]: "r0]"
     token[0]: "[r0", toekn[1]: "r0", token[2]: "lsl #2]"
    *r: the register in token[0], "[r0" returns 0
    *offset: the offset in [] "#4]", returns 4
    *offset: also the reg in [], "r1]" returns 1
    *i: 0 is 12 bit imm, 1 is reg with shifts
    *u: 0 sub op2 from base, 1 add op2 to base
    *p: 0 is subtract from rn, 1 is add to rn
    *w: 0 is dont write back to rn, 1 is write back to rn

 */
void getBracketExpr(vector *tokens, int *rn, int32_t *offset, int *i, int *u, int *p, int *w,
                    vector *errorVector, char *ln) {
  char *token;
  int tokenSize = 0;
  *w = 0; // default is dont write back to rn
  *p = 1; // default is pre inc/decr - decodeSingleDataTransfer sets to 0 if needed 
  vector bracketExpr = constructVector();

  if (DEBUG) {
  printf("tokens at start of getBracketExpr\n"); // gusty
  printVector(*tokens); // gusty
  }
  // remove the square brackets from the existing tokens
  // tokens example "[r1", "#2]" -> "#2" without changing *w
  // tokens example "[r1", "#2]!" -> "#2" with *w = 1
  // tokens example "[r1", "r2", "LSL", "#2]!" -> "LSL", "#2" with *w = 1
  // Notice first token has [ and last token has ] or ]!
  // build bracketExpr from tokens
  // consume tokens in tokens as the are added to bracketExpr
  do {
    token = getFront(tokens); // consumes front token in tokens
    tokenSize = strlen(token);
    char *newToken = token;

    if (token[0] == '[') {
      newToken++;
    }

    if (token[tokenSize - 1] == '!' && token[tokenSize - 2] == ']') {
      *w = 1; // write back to rn
      *p = 1; // pre increment
      tokenSize--;
    }
    if (token[tokenSize - 1] == ']') {
      token[tokenSize - 1] = '\0';
      if (strlen(token)) {
        // token not empty
        putBack(&bracketExpr, newToken); // puts newToken on bracketExpr
      }
      free(token);
      break;
    }

    putBack(&bracketExpr, newToken);

    free(token);
  } while (!isEmptyVector(*tokens));

  if (DEBUG) {
  printf("tokens after bracketExp do loop\n"); // gusty
  printVector(bracketExpr); // gusty
  }
  token = getFront(&bracketExpr); // 1st token in bracketExpr is "r5"
  *rn = getDec(token + 1);
  free(token);

  token = getFront(&bracketExpr);

  *u = 1; // assume add to base, gusty
  if (DEBUG) {
  printf("token on front of bracketExp: %s\n", token); // gusty
  printVector(bracketExpr); // gusty
  }

  int fixtoken = 0;
  if (token) {
    /*
    if (!(strcmp(token, "-") == 0)) {
      u = 0;
      free(token);
      token = getFront(&bracketExpr);
      printf("token: %s\n", token); // gusty
    } else if (token[0] == '-') {
    if (token[0] == '#') {
      token++;
      u = 0;
      neg = 1; // gusty
    }*/
    if (token[0] == '-' && token[1] == 'r') { // ldr r0, [r1, -r2]
      token++;
      *u = 0;
      fixtoken = 1;
    }
    if (token[0] == '+' && token[1] == 'r') { // ldr r0, [r1, +r2]
      token++;
      fixtoken = 1;
    }
    if (token[0] == '#' && token[1] == '-') { // ldr r0, [r1, #-4]
      token[1] = '#';
      token++;
      *u = 0; // subtract from base, gusty
      fixtoken = 1;
    }

    if (DEBUG) {
    printf("GUSTY HERE: %s\n", token);
    }
    if (getType(token) == EXPRESSION_TAG) {
      *offset = getExpression(token, NULL, 0);
    } else if (getType(token) == REGISTER) {
      if (DEBUG) {
      printf("GUSTY HERE\n");
      }
      *offset = getDec(token + 1);
      *i = 1;
    }
  }
  if (fixtoken) {
    token--;
  }
  free(token);

  token = peekFront(bracketExpr);
  if (DEBUG) {
  printf("token on front of bracketExp after if (token): %s\n", token); // gusty
  }
  if (isShift(token)) {
    uint32_t aux = *offset;
    getShift(&bracketExpr, &aux, errorVector, ln);
    if (DEBUG) {
    printf("shift: %d\n", aux); // gusty
    }
    *offset = aux;
  }
}

/* Process ldr/str instructions - two formats
   1. ldr r0,[] - has a bracket expression
   2. ldr r0,=label - has an =label

 */
uint32_t decodeSingleDataTransfer(vector *tokens, vector *addresses,
                uint32_t instructionNumber, uint32_t instructionsNumber,
                vector *errorVector, char *ln) {
  char *instruction = getFront(tokens);
  char *rdName;
  uint32_t ins = 1 << 0x1A;
  int i = 0; // 0 is 12-bit immediate, 1 is reg with shift
  int p = 1; // 0 is post (x++ or x--), 1 is pre (++x or --x)
  int u = 1; // 0 is down (sub), 1 is up (add)
  int w = 0; // 0 is not write back to rn, 1 is write back to rn
  int l = !strcmp(instruction, "ldr") ? 1 : 0; // 0 is str, 1 is ldr
  int32_t offset = 0;
  int rn = 0xf; // default rn - base register
  int rd = 0;   // destination register LDR rd, [rb]
  char *token;

  setCond(&ins, ALWAYS_CONDITION);

  if (checkReg(tokens, instruction, errorVector, ln)) { // get rd of LDR rd, [rb]
    token = getFront(tokens);
    rd = getDec(token + 1);
    rdName = token;
  } else {
    free(instruction);
    return -1;
  }

  // check for register or expression
  token = peekFront(*tokens);
  if (token && token[0] == '[') {
    // we have indexed address
    getBracketExpr(tokens, &rn, &offset, &i, &u, &p, &w, errorVector, ln);

    if (DEBUG) {
    printf("decodeSingle after getBracketExp\n");
    printVector(*tokens); // gusty
    printf("offset: %d\n", offset); // gusty
    }

    token = peekFront(*tokens);

    if (DEBUG) {
    printf("Tokens after ], token: %s\n", token);
    printVector(*tokens); // gusty
    }
    // Next if handles post increment ldr/str instructions
    // ldr r3, [r1, #12], #4
    // ldr r3, [r1], r4
    // ldr r3, [r1], r4, LSL 2
    // ldr r3, [r1], -r4
    if (token) { // ldr r3, [r1, #12], #4
      if (!strcmp(token, "-")) {
        u = 0;
        free(token);
        token = getFront(tokens);
      } else if (token[0] == '#' && token[1] == '-') { // change #-8 to #8
        token++;
        token[0] = '#';
        u = 0;
      }

      if (DEBUG) {
      printf("Processing tokens after ], token: %s\n", token);
      printVector(*tokens); // gusty
      }

      if (getType(token) == EXPRESSION_TAG) { // we have post indexed expression
        token = getFront(tokens);
        if (u == 0) { // we modified peeked token - see previous if statement
          token++;
          token[0] = '#';
        }
        offset = getExpression(token, NULL, 0);
        if (DEBUG) {
        printf("post indexed expression: token: %s, offset: %d\n", token, offset);
        }
        if (u == 0) // we modified token, restore it before free
          token--;
        free(token);
        p = 0;
        w = 1;
      } else if (getType(token) == REGISTER) { // we have post indexed register
        token = getFront(tokens);
        offset = getDec(token + 1);
        free(token);
        p = 0;
        i = 1;
        w = 1;

        token = peekFront(*tokens);
        if (isShift(token)) {
          uint32_t aux = offset;
          getShift(tokens, &aux, errorVector, ln);
          offset = aux;
        }
      }
    }

    if (offset < 0) {
      offset = abs(offset);
      u = 0;
    }
  } else { // ldr r5, =0x20200000
    p = 1; // pre inc/dec, add offset to the PC ldr r0, [ps, offset]
    if (getType(token) != EXPRESSION_EQUAL) {
      throwExpressionError(instruction, errorVector, ln);
      free(rdName);
      free(instruction);
      return -1;
    }

    if (!strcmp(instruction, "ldr")) {
      // we have a load instruction
      if (getType(token) == EXPRESSION_EQUAL) {
        uint32_t address = getExpression(token, errorVector, ln);

        if (address <= 0xFF) {
          // interpret as move instruction
          putFront(tokens, rdName);
          putFront(tokens, "mov");
          free(rdName);
          free(instruction);
          return decodeDataProcessing(tokens, errorVector, ln);
        } else {
          // interpret as normal
          putBack(addresses, token);
          int addressLocation = instructionsNumber + addresses->size - 1;
          offset = (addressLocation - instructionNumber - 2) * MEMORY_SIZE;
        }
      }
      free(getFront(tokens));
    }
  }

  ins |= i << 0x19; // set bit i 25, reg=1, imm=0
  ins |= p << 0x18; // set bit p 24, post=1, pre=0
  ins |= u << 0x17; // set bit u 23, sub=0, add=1
  ins |= w << 0x15; // set bit w 21, dont write back=0, write back=1
  ins |= l << 0x14; // set bit l 20, str=0, ldr=1
  ins |= rn << 0x10; // set rn
  ins |= rd << 0xC; // set rd
  ins |= offset; // set offset

  free(rdName);
  free(instruction);
  return ins;
}

uint32_t decodeBranch(vector *tokens, uint32_t instructionNumber,
                        map labelMapping, vector *errorVector, char *ln) {
  char *branch = getFront(tokens);
  if (DEBUG) {
  printf("branch 0: branch: %s, %lu, %c, %c\n", branch, strlen(branch), branch[0], branch[1]);
  }
  uint32_t ins = 0xA << 0x18;
  if (branch[1] == 'l' && strlen(branch) == 2) { // bl instruction
    ins = ins | 1<<24;
    if (DEBUG) {
    printf("branch 1\n");
    }
    setCond(&ins, (branch + 2));
  } else {
    setCond(&ins, (branch + 1));
  }
  if (DEBUG) {
  printf("branch 2\n");
  }
  uint32_t *mem;
  uint32_t target;

  char *expression = (char *) peekFront(*tokens);

  if (!expression || getType(expression) == INSTRUCTION) {
    throwExpressionMissingError(branch, errorVector, ln);
    free(branch);
    return -1;
  }

  if ((mem = get(labelMapping, expression))) {
    // we have a mapping
    target = *mem - instructionNumber - PC_OFFSET;
    target <<= INSTRUCTION_SIZE - (BRANCH_OFFSET_SIZE - 2);
    target >>= INSTRUCTION_SIZE - (BRANCH_OFFSET_SIZE - 2);
  } 
  else {
    printf("no mapping for target of Branch.");
    target = 0;
  }

  ins |= target;
  getFront(tokens);
  free(branch);
  free(expression);
  return ins;
}

uint32_t decodeShift(vector *tokens, vector *errorVector, char *ln) {
  char *shift = getFront(tokens);
  char *rn = NULL;

  if (checkReg(tokens, shift, errorVector, ln)) {
    rn = getFront(tokens);
  }

  if (!rn) {
    free(shift);
    return -1;
  }

  // fill front of tokens with mov Rn, Rn, lsl <#expression>
  putFront(tokens, shift);
  putFront(tokens, rn);
  putFront(tokens, rn);
  putFront(tokens, "mov");

  free(shift);
  free(rn);

  return decodeDataProcessing(tokens, errorVector, ln);
}

void getShift(vector *tokens, uint32_t *operand,
                vector *errorVector, char *ln) {
  char *shift = getFront(tokens);
  char *token = peekFront(*tokens);

  if (getType(token) != EXPRESSION_TAG && getType(token) != REGISTER) {
    // throw error
  }

  if (getType(token) == EXPRESSION_TAG) {
    // we have expression
    *operand |= getExpression(token, errorVector, ln) << 0x7;
  } else {
    // we have register
    *operand |= getDec(token + 1) << 0x8;
    // set bit 4
    *operand |= 0x1 << 0x4;
  }

  *operand |= *get(SHIFTS, shift) << 0x5;

  free(getFront(tokens));
  free(shift);
}

void setCond(uint32_t *x, char *cond) {
  uint32_t condition = *get(CONDITIONS, cond) << 0x1C;
  // make space
  *x <<= 4;
  *x >>= 4;
  *x |= condition;
}

vector tokenise(char *start, char *delimiters) {
  int tokenSize = 0;
  int i;
  vector tokens = constructVector();

  for (i = 0; start[i] != '\0'; i++) {
    tokenSize++;

    if (strchr(delimiters, start[i])) {
      // found delimiter add token to vector
      tokenSize--;
      if (tokenSize) {
        // make sapce for token string
        char *token = malloc((tokenSize + 1) * sizeof(char));
        strncpy(token, start + i - tokenSize, tokenSize);
        token[tokenSize] = '\0';
        putBack(&tokens, token);
        tokenSize = 0;
        free(token);
      }
    }
  }

  if (tokenSize) {
    // we still have one token left
    char *token = malloc((tokenSize + 1) * sizeof(char));
    strncpy(token, start + i - tokenSize, tokenSize);
    token[tokenSize] = '\0';
    putBack(&tokens, token);
    free(token);
  }

  return tokens;
}

bool isRegister(char *token) {
  int length  = strlen(token);

  if (length < 2 || length > 3 || token[0] != 'r') {
    return false;
  }

  for (int i = 1; i < length; i++) {
    if (token[i] < '0' || token[i] > '9') {
      return false;
    }
  }

  int number  = atoi(token + 1); // token points to "R12", token+1 points to "12"
  return number >= 0 && number <= 16; // BUG: number <= 15
}

bool isLabel(char *token) {
  return token[strlen(token) - 1] == ':';
}

typeEnum isExpression(char *token) {
  if (!strlen(token)) {
    return UNDEFINED;
  }

  if (token[0] != '#' && token[0] != '=') {
    return UNDEFINED;
  }

  int i = 1;
  if (token[i] == '-') {
    i++;
  }

  if (strlen(token) >= i + 3 && token[i] == '0' &&token[i + 1] == 'x') {
    // we might have a hex value
    i += 2;

    for (; token[i] != '\0'; i++) {
      if ((token[i] < '0' || token[i] > '9') &&
          (token[i] < 'a' || token[i] > 'f') &&
          (token[i] < 'A' || token[i] > 'F')) {
        return UNDEFINED;
      }
    }
  } else {
    // we might have a decimal value

    if (token[i] == '\0') {
      return UNDEFINED;
    }

    for (int i = 1; token[i] != '\0'; i++) {
      if (token[i] < '0' || token[i] > '9') {
        return UNDEFINED;
      }
    }
  }

  return token[0] == '#' ? EXPRESSION_TAG : EXPRESSION_EQUAL;
}

bool isInstruction(char *token) {
  return get(ALL_INSTRUCTIONS, token);
}

bool isShift(char *token) {
  if (!token) {
    return false;
  }

  return get(SHIFTS, token);
}

typeEnum getType(char *token) {
  if (!token) {
    return UNDEFINED;
  }

  if (isLabel(token)) {
    return LABEL;
  }

  if (isExpression(token) != UNDEFINED) {
    return isExpression(token);
  }

  if (isInstruction(token)) {
    return INSTRUCTION;
  }

  if (isRegister(token)) {
    return REGISTER;
  }

  return UNDEFINED;
}

// BUG: returns positivee in *exp for #-4
int32_t getExpression(char *exp, vector *errorVector, char *ln) {
  uint32_t res = 0;
  uint32_t rotations = 0;

  int i = 1, neg = 0;

  if (exp[i] == '-') {
    i++;
    neg = 1;
  }

  if (exp[i] == '0' && exp[i + 1] == 'x') {
    res = getHex(exp + 1);
  } else {
    res = getDec(exp + 1);
  }

  if (exp[0] == '#') {
    // check if exp can be roatated to a 8 bit imediate value
    //while (abs(res) >= 0x100 && rotations <= 30) { - original line had abs(res) - res is uint
    while (res >= 0x100 && rotations <= 30) {
      char bits31_30 = (res & 0xC0000000) >> (INSTRUCTION_SIZE - 2);
      res <<= 2;
      res |= bits31_30;
      rotations += 2;
    }

    if (rotations > 30) {
      // throw an error
      // number can't be represented
      printf("HERE\n");
      throwExpressionError(exp, errorVector, ln);
      return -1;
    }

    res |= ((rotations / 2) << 0x8);
  }

  if (neg)
    res = -res;

  return res;
}

int32_t getHex(char *exp) {
  int32_t res = 0;
  bool negative = false;
  if (exp[0] == '-') {
    negative = true;
    exp++;
  }
  exp += 2;
  for (int i = 0; exp[i] != '\0'; i++) {
    int x;
    if (exp[i] >= 'A' && exp[i] <= 'F') {
      x = exp[i] - 'A' + 10;
    } else if (exp[i] >= 'a' && exp[i] <= 'f') {
      x = exp[i] - 'a' + 10;
    } else {
      x = exp[i] - '0';
    }
    res <<= 4;
    res |= x;
  }

  return negative ? -res : res;
}

int32_t getDec(char *exp) {
  return atoi(exp);
}

char *uintToString(uint32_t num) {
  int n = num;
  int length = 0;

  do {
    length++;
    n /= 10;
  } while (n != 0);

  char *ret = malloc((length + 1) * sizeof(char));

  for (int i = 0; i < length; i++) {
    ret[length - i - 1] = num % 10 + '0';
    num /= 10;
  }

  ret[length] = '\0';

  return ret;
}

void clearLinesFromFile(char **linesFromFile) {
  for(int i = 0; i < (countDynamicExpansions * NUMBER_OF_LINES); i++) {
    free(linesFromFile[i]);
  }
  free(linesFromFile);
}

// ----------------------ERRORS--------------------------------
void throwUndefinedError(char *name, vector *errorVector, char *ln) {
  char *error = malloc(200);
  strcpy(error, "[");
  strcat(error, ln);
  strcat(error, "] Undefined instruction ");
  strcat(error, name);
  strcat(error, ".");
  putBack(errorVector, error);
  free(error);
}

void throwLabelError(char *name, vector *errorVector, char *ln) {
  char *error = malloc(200);
  strcpy(error, "[");
  strcat(error, ln);
  strcat(error, "] Multiple definitions of the same label: ");
  strcat(error, name);
  strcat(error, ".");
  putBack(errorVector, error);
  free(error);
}

void throwExpressionError(char *name, vector *errorVector, char *ln) {
  char *error = malloc(200);
  strcpy(error, "[");
  strcat(error, ln);
  strcat(error, "] The expression ");
  strcat(error, name);
  strcat(error, " is invalid.");
  putBack(errorVector, error);
  free(error);
}

void throwRegisterError(char *name, vector *errorVector, char *ln) {
  char *error = malloc(200);
  strcpy(error, "[");
  strcat(error, ln);
  strcat(error, "] The register ");
  strcat(error, name);
  strcat(error, " is invalid.");
  putBack(errorVector, error);
  free(error);
}

void throwExpressionMissingError(char *ins, vector *errorVector, char *ln) {
  char *error = malloc(200);
  strcpy(error, "[");
  strcat(error, ln);
  strcat(error, "] The expression is missing from the ");
  strcat(error, ins);
  strcat(error, " instruction.");
  putBack(errorVector, error);
  free(error);
}

// -----------------------DEBUGGING---------------------------
void printBinary(uint32_t nr) {
  uint32_t mask = 1 << (INSTRUCTION_SIZE - 1);

  printf("%d: ", nr);

  for (int i = 0; i < INSTRUCTION_SIZE; i++) {
    if (!(i % 8) && i) {
      putchar(' ');
    }
    putchar((nr & mask) ? '1' : '0');
    mask >>= 1;
  }

  putchar('\n');
}

void printStringArray(int n, char arr[][MAX_LINE_LENGTH]) {
  for (int i = 0; i < n; i++) {
    printf("%s", arr[i]);
  }
}
