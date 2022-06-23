# ovaj test je za ascii neki ispis
# file: main.s
.global myStart
.section myCode
# .equ tim_cfg, 0xFF10
myStart:
ldr r0, $endStr
ldr r1, $startStr
sub r0, r1
ldr r1, $0
ldr r4, $1
ldr r2, $0
ldr r5, $0
wait:
ldr r3, [r5 + startStr]
str r3, 0xFF00
add r5, r4
wait1: cmp r2, r4
jne wait1
ldr r2, $0
add r1, r4 
cmp r0, r1
jne wait
halt
.section myData
startStr:
.ascii "Ovo je\tneki tekst \nza moj ispis"
endStr:
.end