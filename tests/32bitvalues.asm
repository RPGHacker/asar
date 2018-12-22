;`FF
;`FF FF FF FF
;`21 43 65 87
;`FF FF FF FF
;`00 00 FF 00

org $8000

db -1
dd $FFFFFFFF
; returns 00 00 00 80
dd $87654321+0
dd (-2)/2
; returns 00 00 80 FF ?????
dd $FF000000/256
