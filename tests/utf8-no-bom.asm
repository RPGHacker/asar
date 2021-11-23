;`00 01 02 02 03 04 05 06 07 05 03 08
;`10
;`11

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


table "data/table.tbl"

db "Ä"


table "data/table-rtl.tbl",rtl

db "Ü"