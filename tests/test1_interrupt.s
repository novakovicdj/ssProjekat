# file: interrupts.s
.section ivt
.word isr_reset
.skip 2 # isr_error
.word isr_timer
.word isr_terminal
.skip 8
.extern myStart
.section isr
# prekidna rutina za reset
isr_reset:
jmp myStart
# prekidna rutina za tajmer
isr_timer:
iret
# prekidna rutina za terminal
isr_terminal:
push r0
push r1
ldr r0, 0xFF02
str r0, 0xFF00
pop r1
pop r0
iret
.end