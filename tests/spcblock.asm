;`A9 AA
;`03 00 00 60
;`8F 44 33
;`06 00 00 50
;`5F 03 50
;`8F 22 11
;`00 00 00 50
;`A9 BB

org $008000

lda #$AA

spcblock $6000
	mov $33,#$44
endspcblock

spcblock $5000
start:
	jmp lab
lab:
	mov $11,#$22
endspcblock execute start

lda #$BB
