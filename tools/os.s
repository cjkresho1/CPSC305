cmp r0, #1
beq scanfintfunc
cmp r0, #2
beq printfintfunc
cmp r0, #3
beq malloc
// return -1
retminusone:
mov r0, #0
sub r0, r0, #1
mov r15, r14
// readint function, r0=1, r1=val1, r2=val3
// returns value input in r0
// 0x20200000 is memory mapped I/O for readint
scanfintfunc:
ldr r1, =0x20200000
ldr r0, [r1]
mov r15, r14
// sub function, r0=2, r1=val1, r2=val3
// return val1-val2 in r0
printfintfunc:
ldr r0, =0x20200000
str r1, [r0]
mov r15, r14
// malloc function, r0=3, r1=num_of_bytes
// return address in r0
// mallocs num_of_bytes rounded up to a multiple of 4
// ask for 10, get 12
// Heap memory begins at 0x2004 and grows up
// pStatePtr->memory[0x2000/4] has the next available address
// pStatePtr->memory[0x2000/4] = 0x2004; is the intial value
malloc:
cmp r1, #0       // must ask for a positive number of bytes
ble retminusone  // return -1 for 0 or negative number of bytes
and r2, r1, #3   // examine bottom two bits of number of bytes
cmp r2, #3       // asking for 3 bytes
bne try2
add r1, r1, #1   // add 1 to make a multiple of 4
b mallocmem
try2:
cmp r2, #2       // asking for 2 bytes
bne try1
add r1, r1, #2   // add 2 to make a multiple of 4
bne mallocmem
try1:
cmp r2, #1       // asking for 1 byte
bne mallocmem
add r1, r1, #3   // add 3 to make a multiple of 4
// At this point r1 has a multiple of 4 bytes
mallocmem:
ldr r2, =0x2000  // put heap address in r2
ldr r3, [r2]     // get next free address from heap pointer
mov r0, r3       // ret value, address of malloced memory
add r3, r3, r1   // update next free address, add num of bytes requested
str r3, [r2]     // store next free address in heap pointer
mov r15, r14
