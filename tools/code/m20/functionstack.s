// eOS sets r13 to 0x4000
main:
    // int ia[] = {1,2,3,4,80,6,7,8,9,10};
    sub r13, r13, #48 // space for ia[10], largest, and r14
    mov r2, #1
    str r2, [r13, #0] // ia[0] = 1
    mov r2, #2
    str r2, [r13, #4]   // ia[1] = 2
    mov r2, #3
    str r2, [r13, #8]   // ia[2] = 3
    mov r2, #4
    str r2, [r13, #12]  // ia[3] = 4
    mov r2, #80
    str r2, [r13, #16]  // ia[4] = 80
    mov r2, #6
    str r2, [r13, #20]  // ia[5] = 6
    mov r2, #7
    str r2, [r13, #24]  // ia[6] = 7
    mov r2, #8
    str r2, [r13, #28]  // ia[7] = 8
    mov r2, #9
    str r2, [r13, #32]  // ia[8] = 9
    mov r2, #10
    str r2, [r13, #36]  // ia[9] = 10
    // int largest = 0;
    mov r2, #0
    str r2, [r13, #40]  // largest = 0
    str r14, [r13, #44] // save LR on stack
    svc #1
    str r0, [r13, #16]  // ia[4] = user's input
    mov r0, r13         // put address of ia in r0
    mov r1, #10         // put size of ia in r1
    bl largest
    str r0, [r13, #0x28] // int largest = largest(ia,10);
    mov r1, r0
    svc #2              // printf largest
halt:
    b halt              // endless loop

// r0 has address of int array
// r1 has num of elements in array
largest:
    sub r13, r13, #20   // space for largest, i, r4, r5, and lr
    str r4, [r13, #8]   // save r4 on stack
    str r5, [r13, #12]  // save r5 on stack
    str r14, [r13, #16] // save lr on stack
    ldr r2, [r0]
    str r2, [r13, #0]   // int largest = a[0]
    mov r4, r0          // put address of a in r4
    mov r3, #0
    str r3, [r13, #4]   // for (int i = 0;
loop:
    ldr r5, [r4],#4     // put ia[i] in r5, post increment r4
    cmp r5, r2          // if ia[i] < largest 
    blt skip
    mov r2, r5          // update largest
    str r2, [r13, #0]   // largest = a[i]
skip:
    add r3, r3, #1
    str r3, [r13, #4]   // for ( ; ; i++)
    cmp r3, r1          // for( ; i < size; )
    blt loop            // continue looping until i >= size
    mov r0, r2          // place largest in r0
    ldr r4, [r13, #8]   // restore r4
    ldr r5, [r13, #12]  // restore r5
    ldr r14, [r13, #16] // restore lr
    mov r15, r14        // return to caller
    
