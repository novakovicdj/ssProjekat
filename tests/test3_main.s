# test za poziv moje interrupt metode
# file: main.s
.global myStart
.section myCode
#.equ tim_cfg, 0xFF10
myStart:
ldr r0, $128
ldr r3, $4
ldr r1, myNum
ldr r4, myNum
ldr r2, $0
wait:
int r3
shr r0, r4
cmp r0, r1
add r2, r4
jgt wait
halt
.section myData
myNum:
.word 1
.end