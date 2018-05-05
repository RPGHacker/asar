;`+ #2
;`07606 22 20 80 90
;`07FD7 0A
;`80000
;`53 54 41 52 18 00 E7 FF
;`50 52 4F 54 06 29 80 90 32 80 90
;`50 52 4F 54 03 3C 80 90
;`53 54 4F 50 00
;`EA
;`53 54 41 52 00 00 FF FF
;`EA
;`53 54 41 52 01 00 FE FF
;`EA EA
;`53 54 41 52 02 00 FD FF
;`EA EA EA
;`FFFFF 00
org $00F606
autoclean JSL root

freecode
prot a,b
prot c
root:
NOP

freedata
a:
NOP

freedata
b:
NOP #2

freedata
c:
NOP #3