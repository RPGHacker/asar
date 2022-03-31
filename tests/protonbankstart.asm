;`+
;`07FD7 0A
;`80000 53 54 41 52 2F 75 D0 8A
;`87FF8 53 54 41 52 94 13 6B EC
;`88000 50 52 4F 54 03 08 80 90 53 54 4F 50
;`FFFFF 00
freedata cleaned
A:
!i = 0
while !i < 30000
	db 0
	!i #= !i+1
endwhile

freedata cleaned
prot A
!j = 0
while !j < 5000
	db 0
	!j #= !j+1
endwhile
