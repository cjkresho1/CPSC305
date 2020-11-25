// functionnostack.s 
// emu $ r d 400
// emu $ m 400 10
// emu $ m 404 20
// emu $ m 408 30
// emu $ m 40c 0
          ldr r0, [r13,#0]      // r0 is a
          ldr r1, [r13,#4]      // r1 is b
          ldr r2, [r13,#8]      // r2 is c
          bl add3               // call add3
          str r0, [r13,#0xc]    // update d


add3:
          add r0, r0, r1
          add r0, r0, r2
          mov r15, r14 // return from add3
