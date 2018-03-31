; Dummy ROM to use for Asar's test suite

org $008000
; 7 bytes from SMW, used in tests/read.asm
db $78,$9C,$00,$42,$9C,$0C,$42

org $00FFC0
; to make expecttitle not fail
db "SUPER MARIOWORLD     "
db $20 ; lorom

org $00FFD7
db $0A ; mark the rom as 512 kb

org $0FFFFF
db $00 ; to pad the ROM to 512 kb