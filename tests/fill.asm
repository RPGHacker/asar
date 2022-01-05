;`12 12 12 12
;`34 12 34 12 34 12 34 12
;`56 34 12 56 34 12 56 34 12 56 34 12
;`78 56 34 12 78 56 34 12 78 56 34 12 78 56 34 12
;`00 00 00 00 00 00 00 00 00 00 00
org $008000
fillbyte $12 : fill 4
fillword $1234 : fill 8
filllong $123456 : fill 12
filldword $12345678 : fill 16
skip align 16 offset 2
db $00
