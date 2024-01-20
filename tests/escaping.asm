;`01 02 03 04 00 01 02 03 04 05 06 01
;`01 02 03 04 10 01 02 03 04 11 12
;`01 02 03 04 01 02 03 04
;`10 01 02 03 04 11 12 01 02 03 04
;`01 02 03 04 10 01 02 03 04 11 12
;`20 01 02 20 20 20 03 04 20 10 01 02 03 04 11 12
;P>The values of !a and !b are 10 and 20.
;P>The values of !a and !b are 10 and 20.
macro do_stuff_1(stuff_1, stuff_2)
	<stuff_1>
	<stuff_2>
endmacro

macro do_stuff_2(    stuff_1    , stuff_2     )
	db <stuff_1>
	db <stuff_2>
endmacro

'a' = $01
'b' = $02
'c' = $03
'd' = $04
'e' = $05
'f' = $06
' ' = $20

org $008000
	%do_stuff_1("db ""abcd"",$00,""abcdef""", "db 'a'")
	%do_stuff_2("""abcd""", "$10,""abcd"",$11,$12")
	%do_stuff_2("""abcd""", """abcd""")
	%do_stuff_2("$10,""abcd"",$11,$12", """abcd""")
	%do_stuff_2("""abcd""",
		"$10,""abcd"",$11,$12")	
	%do_stuff_2(    """ ab   cd """    ,     ; A lot of intentional spaces here.
		"$10,""abcd"",$11,$12"   )
		
!a = 10
!b = 20

print "The values of \!a and \!b are !a and !b."

macro do_print()
	print "The values of \!a and \!b are !a and !b."
endmacro

%do_print()
