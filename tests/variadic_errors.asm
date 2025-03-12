lorom
org $008000

!a = 0
;`errEvararg_must_be_last
macro asd(..., dfg)
	db sizeof(...), <0>, <!a>
endmacro

macro sorry(...)
	db <...[-1]>
endmacro

macro sorry2(asd, ...)
	db <...[10]>
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

;`errEvararg_out_of_bounds
%sorry(1,2,3,4,5,6,7)
db $FF, $FF
;`errEvararg_out_of_bounds
%sorry2(1,2,3,4,5,6,7)
;`errEmacro_wrong_min_params
%sorry2()
;`errEvararg_out_of_bounds
%sorry2(0)
;`errEmacro_wrong_min_params
%sorry3()


macro deprecated(...)
	db <0>
endmacro

macro unclosed(...)
	db <...[0>
;]>; unfuck my syntax highlight
endmacro

macro invalid(named, ...)
	db <...[named]>
endmacro

macro invalid_2(named)
	db <...[named]>
endmacro

%deprecated($01)
;`errEunclosed_vararg
%unclosed($01)
;`errEno_labels_here
%invalid($01, $01)
;`errEmacro_not_varadic
%invalid_2($01)

; thrown in pass 2:

;`errEmacro_not_varadic
;`errEvararg_sizeof_nomacro
;`errEmacro_not_varadic
;`errEinvalid_number
