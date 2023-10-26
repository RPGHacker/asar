;`00 FF 01 FE 02 03
;`69
;`10 EF 11 EE 12 13
;`20 EF 21 EE
;`35 26
;`69 4A
;`42
;`88 99 AA BB CC
;`72 61
;`34 45 56
;`03 FF 02 03 04 03 04 05
;`00 01 02 03 04 05 06 07 08 09
;`10 11 12 13 14 15 16 17 18 19
;`20 21 22 23 24 25 26 27 28 29
;`40 41 42 43 44 45 46 47 48 49
;`50 51 52 53 54 55 56 57 58 59
;P>include/macro_fuckery_include.asm
;P>top
;P>top
;P>bottom
;P>bottom
;P>do_useless_shit
;P>arg_1
;P>do_more_useless_shit
;P>arg_2
;;`48 58

org $008000

; This attempts to reproduce certain useage cases
; of macros from vwf_dialogues that would commonly
; break after some random changes to asar.
macro new_macro(...)
	!temp_i #= 0
	while !temp_i < sizeof(...)
		db <...[!temp_i]>
			
		!test_<...[!temp_i]>_test = !temp_i
		
		if !test_<...[!temp_i]>_test == 0
			db $FF
		elseif !test_<...[!temp_i]>_test == 1
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
		!temp_i #= 0
		while !temp_i < sizeof(...)
			<...[!temp_i]>
			%new_macro(<...[!temp_i]>)
			!new_macro(<...[!temp_i]>)
			
			!test_<...[!temp_i]>_test = 1
		
			if !test_<...[!temp_i]>_test == 0
				db $00
			elseif !test_<...[!temp_i]>_test == 1
				db $01
			endif
		
			!temp_i #= !temp_i+2
		endwhile
		undef "temp_i"
	endmacro

	%new_macro_2(1, 2, 3)
endif


macro incsrc_macro(filename, the_fun_part)
	incsrc "<filename>"
endmacro

%incsrc_macro("include/macro_fuckery_include.asm", $69)

%recursive_macro(20, 21)


macro actual_recursive(suffix, multiplier, shadowed)
	macro actual_recursive_inner_<suffix>(number, shadowed)
		print "<^suffix>"
		db (<number>*<^multiplier>)+<shadowed>+<^shadowed>
	endmacro
endmacro

%actual_recursive("top", $10, $04)
%actual_recursive("bottom", $20, $08)

%actual_recursive_inner_top(3, 1)
%actual_recursive_inner_top(2, 2)
%actual_recursive_inner_bottom(3, 1)
%actual_recursive_inner_bottom(2, 2)


macro define_macro(name, arg, what)
	macro <name>(<arg>)
		<^what> <<^arg>>
	endmacro
endmacro

%define_macro("write_number", "num", "db")

%write_number($42)


; RPG Hacker: Yes, I know this is a stupid macro.
; But there is no reason why it shouldn't work.
macro define_variadic_macros(...)
	!temp_i #= 0

	while !temp_i < sizeof(...)
		macro <...[!temp_i]>(<...[!temp_i+1]>, ...)
			print "<^...[!^temp_i]>"
			print "<^...[!^temp_i+1]>"

			db <<^...[!^temp_i+1]>>

			!temp_j #= 0

			while !temp_j < sizeof(...)
				db <...[!temp_j]>
				!temp_j #= !temp_j+1
			endwhile

			undef "temp_j"
		endmacro

		!temp_i #= !temp_i+2
	endwhile

	undef "temp_i"
endmacro

%define_variadic_macros("do_useless_shit", "arg_1", "do_more_useless_shit", "arg_2")

%do_useless_shit($88, $99)
%do_more_useless_shit($AA, $BB, $CC)


macro define_resolution(first, second)
	!temp = "db <first>"
	!temp += ",<second>"

	!temp
	undef "temp"
endmacro

%define_resolution($72, $61)


macro insanely_define_macro(...)
	!temp_i #= 1
	!temp_arg_list = ""
	!temp_arg_usage = "db "

	while !temp_i < sizeof(...)
		!temp_arg := "<...[!temp_i]>"
		!temp_arg_list += !temp_arg
		!temp_arg_usage += "<"
		!temp_arg_usage += "!temp_arg"
		!temp_arg_usage += ">"
		if !temp_i < sizeof(...)-1
			!temp_arg_list += ", "
			!temp_arg_usage += ","
		endif
		!temp_arg_list := !temp_arg_list
		!temp_arg_usage := !temp_arg_usage
		undef "temp_arg"

		!temp_i #= !temp_i+1
	endwhile

	macro <...[0]>(!temp_arg_list)
		!^temp_arg_usage
	endmacro

	undef "temp_i"
	undef "temp_arg_list"
	undef "temp_arg_usage"
endmacro

%insanely_define_macro("insane_macro", "arg_1", "arg_2", "arg_3")
%insane_macro($34, $45, $56)


!define_01 = $01
!define_02 = $02

macro threefold_one(shadowed)
	db <shadowed>
	macro threefold_two(not_shadowed)
		!define_01 = $FF
		!define_02 = $FF
		db !define_01
		db !^define_02
		db <^shadowed>
		db <not_shadowed>
		macro threefold_three(shadowed)
			db <^^shadowed>
			db <^not_shadowed>
			db <shadowed>
		endmacro
	endmacro
endmacro

%threefold_one($03)
%threefold_two($04)
%threefold_three($05)


!temp_i #= 0

macro outer_while()
	while !temp_i < 10
		db !temp_i

		macro inner_while_!temp_i()
			!temp_j #= 0

			while !temp_j < 10
				db !temp_j+!temp_i
				!temp_j #= !temp_j+1
			endwhile

			undef "temp_j"
			!temp_i #= !temp_i+$10
		endmacro

		!temp_i #= !temp_i+1
	endwhile

	!temp_i #= $10
endmacro

%outer_while()
%inner_while_1()
%inner_while_2()
!temp_i #= $40
%inner_while_3()
%inner_while_4()

undef "temp_i"


; RPG Hacker: Comparisons without spaces aren't supported
; pre-Asar 2.0, so there is no point even trying to get
; this test to work before then.
if 0
macro will_it_break(arg)
	macro thats_the_question(arg)
		if <arg><<^arg>
			db $48
		else
			db $58
		endif
	endmacro
endmacro

%will_it_break($10)
%thats_the_question($05)
%thats_the_question($15)
endif
