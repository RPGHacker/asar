;`+
;`08000 53 54 41 52 01 00 fe ff 01 02
;`0fff8 53 54 41 52 fd ff 02 00
;`1fffd 03
;`2fff8 53 54 41 52 ff ff 00 00
;`3ffff 04
;`58000 53 54 41 52 00 00 ff ff 05
;`71234 06
warnings disable Wmapper_already_set
norom ; hack: this disables checksum generation
hirom

freecode cleaned
db $01,$02

freedata cleaned
skip $fffd
db $03

freedata cleaned
skip $ffff
db $04

freespace cleaned,bank=$05
db $05

segment bank=$47,start=$471234
db $06
