# ovaj test je za osnovne funkcije i call neke funkcije
# file: main.s
.global myStart
.global myCounter
.section myCode
myStart:
ldr r0, $5
ldr r1, r0 
ldr r0, %myCounter
mul r0, r1
ldr r2, $0xFF30
str r0, [r2]
div r0, r1
call myFunc
halt
myFunc: 
ldr r3, myCounter
ldr r4, [r2]
or r3, r4
ldr r5, $2
shr r1, r5
ret
.section myData
myCounter:
.word 10
.end