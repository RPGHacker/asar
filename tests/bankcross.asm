;@007FFF
;@00
;@008000
;@00
;@FF FF 00
;@00 80 81
;@00 00 00
;@FF FF 00
;@00 80 81
;@EA EA

warn bankcross off

org $00FFFF

First:
	db $00
	
Second:
	db $00
	
	dl First
	dl Second
	
warn bankcross on

	db $00,$00,$00
	
	dl First
	dl Second
	
	nop #2
