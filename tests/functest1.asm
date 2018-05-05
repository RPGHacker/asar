;`08
;`08

; define
!AAA = 4

; macro
macro Undi()
	db test(!AAA)
endmacro

; function
function test(n) = 2*n

org $8000

main:
	db test(!AAA)
	%Undi()


