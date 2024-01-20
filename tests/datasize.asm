;`A9 03 00 A9 F3 7F A9 09 00 00 00 02
;`warnWdatasize_last_label
;`warnWdatasize_exceeds_size
org $008000
main:

lda #datasize(my_table)		;3
lda #datasize(other_label)	;0xFFFFFF
lda #datasize(main)		;9


my_table:
	db $00, $00, $02
other_label:
