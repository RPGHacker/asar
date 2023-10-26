db <the_fun_part>

macro recursive_macro(...)
	!temp_i #= 0
	while !temp_i < sizeof(...)
		db $<...[!temp_i]>
			
		!test_<...[!temp_i]>_test = !temp_i
		
		if !test_<...[!temp_i]>_test == 0
			db $EF
		elseif !test_<...[!temp_i]>_test == 1
			db $EE
		endif
		
		!temp_i #= !temp_i+1
	endwhile
	undef "temp_i"
endmacro

%recursive_macro(10, 11, 12, 13)


!recursive_macro = %recursive_macro
if 0
	macro recursive_macro_2(...)
		!temp_i #= 0
		while !temp_i < sizeof(...)
			db $<...[!temp_i]>
			%recursive_macro(<...[!temp_i]>)
			!recursive_macro(<...[!temp_i]>)
			
			!test_<...[!temp_i]>_test = 1
		
			if !test_<...[!temp_i]>_test == 0
				db $10
			elseif !test_<...[!temp_i]>_test == 1
				db $11
			endif
		
			!temp_i #= !temp_i+2
		endwhile
		undef "temp_i"
	endmacro

	%recursive_macro_2(11, 12, 13)
endif


macro complete_fuckery()
	print "<^filename>"
endmacro

%complete_fuckery()
