;`A9 AA
;`06 00 00 50
;`5F 03 50
;`8F 22 11
;`00 00 00 50
;`A9 BB

org $008000

lda #$AA

spcblock $5000
	startpos start
start:
	jmp lab
lab:
	mov $11,#$22
endspcblock

lda #$BB
