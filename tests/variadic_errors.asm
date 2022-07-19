;`errEvararg_must_be_last
;`errEinvalid_macro_param_name
;`warnWfeature_deprecated
;`errEunclosed_vararg
;`errEinvalid_vararg
;`errEinvalid_vararg
;`errEvararg_out_of_bounds
;`errEvararg_out_of_bounds
;`errEmacro_wrong_min_params
;`errEvararg_out_of_bounds
;`errEmacro_wrong_min_params
;`errEvararg_sizeof_nomacro
;`errEmacro_not_varadic



lorom
org $008000

!a = 0
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

%sorry(1,2,3,4,5,6,7)
db $FF, $FF
%sorry2(1,2,3,4,5,6,7)
%sorry2()
%sorry2(0)
%sorry3()


macro deprecated(...)
	db <0>
endmacro

macro unclosed(...)
	db <...[0>
endmacro

macro invalid(named, ...)
	db <...[named]>
endmacro

macro invalid_2(named)
	db <...[named]>
endmacro

%deprecated($01)
%unclosed($01)
%invalid($01, $01)
%invalid_2($01)
