typedef struct {
    int opcode;
    int rd;
    int rm;
    int rn;
    int flag;
    int address;
    int immediate;
    int offset;
    int condition;
} decoded;

decoded *decode(unsigned int inst);
char *disassemble(decoded *d);
