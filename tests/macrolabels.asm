;@00 00
;@00 00
;@02 80 00
;@02 80 00
;@00 80 00
;@04 80 00
;@18 80 00
;@16 80 00
;@00 00
;@00 00
;@00 00
;@1A 80 00
;@1A 80 00

org $008000
	Main:
		macro macro_with_labels()
			?MacroMain:
				db $00,$00
			?.MacroSub:
				db $00,$00
			?-:
			dl ?.MacroSub
			dl ?MacroMain_MacroSub
			dl -
			dl ?-
			dl +
			dl ?+
			?+:
		endmacro
		
	-:
		%macro_with_labels()
		db $00,$00
	+:
		db $00,$00
		
	.Sub:
		db $00,$00
				
		dl .Sub
		dl Main_Sub
		