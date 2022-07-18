;`01 01 01 FF FF 05 01 01 FF FF 01 02 03 04 05 06 07 FF FF 02 03 04 05 06 07 01 01 02 03
;`warnWfeature_deprecated
;`warnWfeature_deprecated
lorom
org $008000

!a = 0
macro asd(...)
	db sizeof(...), <...[0]>, <!a> ; Intentionally uses deprecated syntax, to make sure it still works
endmacro

macro sorry(...)
	!a = 0
	while !a < sizeof(...)
		db <...[!a]>
		!a #= !a+1
	endwhile
endmacro

macro sorry2(asd, ...)
	!a = 0
	db <...[0]>
	while !a < sizeof(...)-1
		db <...[!a+1]>
		!a #= !a+1
	endwhile
	db <asd>
endmacro

macro optional(required, ...)
	db <required>
	if sizeof(...) > 0
		db <...[0]>
	endif
endmacro

%asd(1)
db $FF, $FF

%asd(1,2,3,4,5)
db $FF, $FF

%sorry(1,2,3,4,5,6,7)
db $FF, $FF
%sorry2(1,2,3,4,5,6,7)

%optional(1)
%optional(2, 3)
