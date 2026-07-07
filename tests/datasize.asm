struct wat $0000
	.first: skip 5
	.last: skip 2
endstruct

org $008000
main:

;`A9 03 00
lda #datasize(my_table)
;`A9 F3 7F
;`warnWdatasize_last_label
;`warnWdatasize_exceeds_size
lda #datasize(other_label)
;`A9 09 00
lda #datasize(main)

my_table:
;`00 00 02
	db $00, $00, $02
other_label:

;`a9 05
lda #datasize(wat.first)
;`a9 02
lda #datasize(wat.last)
