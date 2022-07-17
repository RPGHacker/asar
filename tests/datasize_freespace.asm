;`+
;`A9 03 00 A9 F4 7F 22 08 80 90
;`80000 53 54 41 52 02 00 FD FF 00 00 02 
;`FFFFF 00
;`warnWdatasize_last_label
;`warnWdatasize_exceeds_size

org $008000
main:

lda #datasize(my_table)		;3
lda #datasize(other_label)	;0xFFFFFF
autoclean jsl my_table
freecode
my_table:
	db $00, $00, $02
other_label:
