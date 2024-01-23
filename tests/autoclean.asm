;`+ #2
;`00000 5C 08 80 90 bf 11 80 90 cf 1a 80 90 23 80 90
;`07FD7 0A
;`80000 53 54 41 52 00 00 FF FF EA
;`      53 54 41 52 00 00 FF FF 12
;`      53 54 41 52 00 00 FF FF 34
;`      53 54 41 52 00 00 FF FF 56
;`FFFFF 00

org $008000
autoclean JML Mymain
autoclean lda l2,x
autoclean cmp l3
autoclean dl l4

freecode
Mymain:
NOP

freedata
l2: db $12
freedata
l3: db $34
freedata
l4: db $56
