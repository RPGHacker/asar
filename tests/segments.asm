;`+
;`00007 0a 0b 0c
;`80000
;`af 07 80 90
;`ad 07 80 34
;`03 04 05
;`06 07
;`53 54 41 52 00 00 ff ff 56
;`09 08
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
; whole bankful of data
db $78
fillbyte $00
fill 32766
db $9a

segment bank=$15
db $01
fill 8
segment bank=$15
db $02

segment pin=forwardref
pin2:
db $03,$04,$05

segment pin=pin2
db $06,$07

segment pin=pin4
pin3:
db $08
segment pin=pin3
pin4:
db $09

segment start=$8000
db $0a,$0b,$0c
