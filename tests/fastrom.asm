;`+
;`01
;`80 FE
;`4C 01 80
;`5C 01 80 00
;`
;`07FD7 0A
;`80000
;`53 54 41 52 0D 00 F2 FF
;`50 52 4F 54 03
;`15 80 90
;`53 54 4F 50 00
;`EA
;`FFFFF 00
;`warnW1030

fastrom ; this is a null operation. it gives too much trouble.

org $8000
db $01
warnpc $8001

Test:
BRA Test
JMP Test
JML Test

freecode cleaned
prot A
A:
NOP
