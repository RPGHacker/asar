;`00 01 02 02 03 04 05 06 07 05 03 08
;`10
;`00 F8 02 00
;`00 00 0F 00
;`00 00 10 00
;`6F
;P>12 0 -1
;P>14

'D' = $00
'o' = $01
'p' = $02
'e' = $03
'l' = $04
'g' = $05
'ä' = $06
'n' = $07
'r' = $08

org $008000
db "Doppelgänger"


incsrc "data/table.tbl"

print bytes," ",freespaceuse, " ", bin(-1)

reset bytes

db "Ä"
dd "丽"
dd "󰀀"
dd "􀀀"

cleartable
db 'o'
print bytes
