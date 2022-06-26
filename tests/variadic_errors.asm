;`errE5182
;`errE5097
;`errE5179
;`errE5178
;`errE5181
;`errE5181
;`errE5180
;`errE5181
;`errE5180



lorom
org $008000

!a = 0
macro asd(..., dfg)
	db sizeof(...), <0>, <!a>
endmacro

macro sorry(...)
	db <-1>
endmacro

macro sorry2(asd, ...)
	db <10>
endmacro

macro normal()
	db sizeof(...)
endmacro

macro sorry3(asd, ...)
	db 0
endmacro

%asd(1, 2)
db $FF, $FF
db sizeof(...)
%normal()

%sorry(1,2,3,4,5,6,7)
db $FF, $FF
%sorry2(1,2,3,4,5,6,7)
%sorry2()
%sorry2(0)
%sorry3()
