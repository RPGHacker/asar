;`10
;`10

; define
!AAA = 4

; macro
macro Undi()
	db test(!AAA)
endmacro

; function
function test(n) = 4*n

org $8000

main:
	db test(!AAA)
	%Undi()
