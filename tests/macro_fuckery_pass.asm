;`00 FF 01 FE 02 03
;`10 EF 11 EE 12 13
org $008000

; This attempts to reproduce certain useage cases
; of macros from vwf_dialogues that would commonly
; break after some random changes to asar.
macro new_macro(...)
	!temp_i = 0
	while !temp_i < sizeof(...)
		db <!temp_i>
			
		!test_<!temp_i>_test = <!temp_i>
		
		if !test_<!temp_i>_test == 0
			db $FF
		elseif !test_<!temp_i>_test == 1
			db $FE
		endif
		
		!temp_i #= !temp_i+1
	endwhile
	undef "temp_i"
endmacro

%new_macro(0, 1, 2, 3)


!new_macro = %new_macro
if 0
	macro new_macro_2(...)
		!temp_i = 0
		while !temp_i < sizeof(...)
			<!temp_i>
			%new_macro(<!temp_i>)
			!new_macro(<!temp_i>)
			
			!test_<!temp_i>_test = 1
		
			if !test_<!temp_i>_test == 0
				db $00
			elseif !test_<!temp_i>_test == 1
				db $01
			endif
		
			!temp_i #= !temp_i+2
		endwhile
		undef "temp_i"
	endmacro

	%new_macro_2(1, 2, 3)
endif


macro incsrc_macro(filename)
	incsrc "<filename>"
endmacro

%incsrc_macro("include/macro_fuckery_include.asm")
