;`+ #2
;`00000 5C 08 80 90
;`07FD7 0A
;`80000 53 54 41 52 00 00 FF FF EA
;`FFFFF 00
;`errEstatic_freespace_growing
org $8000
autoclean jml lbl

freedata static
lbl:
nop
if canread1($108000)
	; if in the 2nd iteration
	nop
endif
