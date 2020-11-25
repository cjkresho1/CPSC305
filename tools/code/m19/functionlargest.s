// functionlargest.s 
        mov r0, r13         // r0 has address of ia
        mov r1, #10         // r1 has size of ia
        bl largest
        str r0, [r13, #0x28]  // c = largest(a,10);
        mov r1, r0
        svc #2              // printf largest
halt:
        b halt              // endless loop


largest:
        ldr r2, [r0]        // r2 is largest
        mov r4, r0          // put address of a in r4
        mov r3, #0          // i = 0
loop:
        ldr r5, [r4],#4     // put ia[i] in r5, post increment r4
        cmp r5, r2          // cmp ia[i] to largest
        blt skip
        mov r2, r5          // update largest
skip:
        add r3, r3, #1      // i++
        cmp r3, r1          // i < size
        blt loop            // continue looping until i >= size
        mov r0, r2          // place largest in r0
        mov r15, r14        // return to caller
