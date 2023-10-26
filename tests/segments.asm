;`+
;`80000
;`af 07 80 90 ad 07 80 34
;`53 54 41 52 00 00 ff ff 56
;`a0000 78
;`a7fff 9a 01
;`a8009 02
;`fffff 00

segment
lda forwardref

segment
lda forwardref
forwardref:
db $34

freedata cleaned
db $56

segment bank=$14
; whole bank-ful of data
db $78
fillbyte $00
fill 32766
db $9a

segment bank=$15
db $01
fill 8
segment bank=$15
db $02
