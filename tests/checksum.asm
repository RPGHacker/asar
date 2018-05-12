;`01
;`07FDC
;`00 FE FF 01
;`0FFFF
;`00

;;`FE FF 01 00

checksum on
norom
org $0000
db $01
org $FFFF ; length padding
db $00
lorom