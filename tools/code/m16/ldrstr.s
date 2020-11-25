// emu $ r 4 400
// emu $ r 6 500
// emu $ m 3fc abadbead
// emu $ m 400 aabbccdd
// emu $ m 404 feedabee
// emu $ m 408 dad
ldr r8, [r4]
ldr r9, [r4, #4]
ldr r9, [r4, #8]
ldr r10, [r4, #-4]!
ldr r11, [r4], #4
ldr r12, [r4, #-4]
mov r3, #0xff
str r3, [r6]
str r3, [r6, #4]
str r3, [r6, #8]
str r3, [r6, #-4]!
add r3, r3, #1
str r3, [r6], #4
str r3, [r6], #4
str r3, [r6, #-4]
