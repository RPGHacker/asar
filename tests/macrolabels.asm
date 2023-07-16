;`00 00
;`00 00
;`02 80 00
;`02 80 00
;`00 80 00
;`04 80 00
;`1B 80 00
;`16 80 00
;`00 90 00
;`00 00
;`00 00
;`00 00
;`1D 80 00
;`1D 80 00
;`04 80 00
;`04 80 00
;`3C 61 3E

org $008000
	Main:
		macro macro_with_labels()
			?MacroMain:
				db $00,$00
			?.MacroSub:
				db $00,$00
			?-:
			#InMacro:
			#.InMacroSub:
			dl ?.MacroSub
			dl ?MacroMain_MacroSub
			dl -
			dl ?-
			dl +
			dl ?+
			?+:
			?MacroAssignment = $009000
			dl ?MacroAssignment
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
		dl InMacro
		dl Main_InMacroSub		; Note that this is not InMacro_InMacroSub

		; not exactly a test for macro labels, but close enough
		db "<a>" ; macro argument outside macro - should be left unexpanded
		
