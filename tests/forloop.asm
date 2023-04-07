;`00 01 02 03 04
;`01 01 01 01 01
;`42
;`00 03 00 04 00 05 01 03 01 04 01 05 02 03 02 04 02 05
;`0a 0b
;`11 11 11
;`69 00 69 01 6a 00 6a 01
;`00 01 02 00 01 02 00 01 02

org $8000

!i = $42

for i = 0..5
    db !i
endfor

for i = 0..5 : db 1 : endfor

for i = 3..2
  dw $dead
endfor

db !i

for i = 0..3
	for j = 3..6
		db !i,!j
	endfor
endfor

if 1
	for i = 10..12
		db !i
	endfor
endif

if 0
	for i = 12..14
		db !i
	endfor
endif

if 1
	for i = 0..3 : db $11 : endfor
endif

for i = $69..$6b
	!j = 0
	while !j < 2
		db !i,!j
		!j #= !j+1
	endwhile
endfor

for i = 0..3
	for i = 0..3
		db !i
	endfor
endfor
