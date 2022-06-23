# file: main.s
.global myStart
.global myCounter
.section myCode
#.equ tim_cfg, 0xFF10
myStart: 
ldr r6, $0xFEFE 
ldr r0, $0x0001 
str r0, 0xFF10
wait:
ldr r0, myCounter
ldr r1, $5
cmp r0, r1
jne wait
halt
.section myData
myCounter:
.word 0
.end