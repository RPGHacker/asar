;`01 01 01 FF FF 05 01 01 FF FF 01 02 03 04 05 06 07 FF FF 02 03 04 05 06 07 01
lorom
org $008000

!a = 0
macro asd(...)
	db sizeof(...), <0>, <!a>
endmacro

macro sorry(...)
	!a = 0
	while !a < sizeof(...)
		db <!a>
		!a #= !a+1
	endif
endmacro

macro sorry2(asd, ...)
	!a = 0
	while !a < sizeof(...)
		db <!a>
		!a #= !a+1
	endif
	db <asd>
endmacro

%asd(1)
db $FF, $FF

%asd(1,2,3,4,5)
db $FF, $FF

%sorry(1,2,3,4,5,6,7)
db $FF, $FF
%sorry2(1,2,3,4,5,6,7)
