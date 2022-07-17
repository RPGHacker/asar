;`89
;`AB
;`01
;`02 03
;`04 05 06
;`07 08 09 0A
;`
;`01
;`02 03
;`FF FF FF FF
;`
;`01
;`01
;`00
;`01
;`00
;`
;`01 00 00
;`02 80 80
;`
;`0A
;`0A
;`05
;`05
;`05
;`00
;`0A
;`
;`02
;`00
;`
;`BB
;`AA
;`AA
;`AA
;`AA
;`
;`01
;`00
;`01
;`00
;`00
;`01
;`
;`00
;`01
;`00
;`01
;`01
;`00
;`00
;`00
;`01
;`01
;`00
;`01
;`
;`00
;`00
;`01
;`00
;`01
;`01
;`01
;`01
;`00
;`01
;`00
;`00
;`00
;`01
;`00
;`
;`01
;`
;`02
;`
;`05
;`04
;`03
;`02
;`01
;`
;`03
;`02
;`01
;`
;`07
;`06
;`05
;`04
;`03
;`02
;`01
;`
;`00 00
;`00 01
;`00 02
;`00 03
;`00 04
;`00 05
;`00 06
;`00 07
;`00 08
;`00 09
;`01 00
;`01 01
;`01 02
;`01 03
;`01 04
;`01 05
;`01 06
;`01 07
;`01 08
;`01 09
;`02 00
;`02 01
;`02 02
;`02 03
;`02 04
;`02 05
;`02 06
;`02 07
;`02 08
;`02 09
;`03 00
;`03 01
;`03 02
;`03 03
;`03 04
;`03 05
;`03 06
;`03 07
;`03 08
;`03 09
;`04 00
;`04 01
;`04 02
;`04 03
;`04 04
;`04 05
;`04 06
;`04 07
;`04 08
;`04 09
;`05 00
;`05 01
;`05 02
;`05 03
;`05 04
;`05 05
;`05 06
;`05 07
;`05 08
;`05 09
;`06 00
;`06 01
;`06 02
;`06 03
;`06 04
;`06 05
;`06 06
;`06 07
;`06 08
;`06 09
;`07 00
;`07 01
;`07 02
;`07 03
;`07 04
;`07 05
;`07 06
;`07 07
;`07 08
;`07 09
;`08 00
;`08 01
;`08 02
;`08 03
;`08 04
;`08 05
;`08 06
;`08 07
;`08 08
;`08 09
;`09 00
;`09 01
;`09 02
;`09 03
;`09 04
;`09 05
;`09 06
;`09 07
;`09 08
;`09 09
;`
;`AC
;`65
;`
;`0B
;`
;`0F
;`
;`0A
;`0B
;`warnWfeature_deprecated
;`
;P>
;P>;This is not a comment, so getconnectedlines() should ignore it and not remove it.
;P>
;P>Let's have some random decimal number with fractional part: 10.34017
;P>Now the same number, but with a different precision: 10.3401734476
;P>Testing a few more numbers: 0, 0.1, -0.1, 1, -1
;P>
asar 1.90

org $008000

db readfile1("data/64kb.bin", 0)
db readfile1("data/64kb.bin", 1)
db readfile1("data/filename with spaces.bin", 0)
dw readfile2("data/filename with spaces.bin", 1)
dl readfile3("data/filename with spaces.bin", 3)
dd readfile4("data/filename with spaces.bin", 6)

db readfile1("data/filename with spaces.bin", 0, 1000)
dw readfile2("data/filename with spaces.bin", 1, -1000)
dd readfile4("data/filename with spaces.bin", 14, $FFFFFFFF)

db canreadfile1("data/filename with spaces.bin", 0)
db canreadfile2("data/filename with spaces.bin", 1)
db canreadfile4("data/filename with spaces.bin", 14)
db canreadfile("data/filename with spaces.bin", 0, 16)
db canreadfile("data/filename with spaces.bin", 0, 17)

dl snestopc($008001)
dl pctosnes($000002)

db max(5, 10)
db max(10, 5)
db min(5, 10)
db min(10, 5)
db clamp(5, 0, 10)
db clamp(-1, 0, 10)
db clamp(11, 0, 10)

db safediv(32, 16, 0)
db safediv(32, 0, 0)

db select(0, $AA, $BB)
db select(1, $AA, $BB)
db select(0.1, $AA, $BB)
db select(-1, $AA, $BB)
db select(-0.1, $AA, $BB)

db not(0)
db not(1)
db equal(1, 1)
db equal(0, 1)
db notequal(1, 1)
db notequal(0, 1)

db less(0, 0)
db less(0, 1)
db less(1, 0)
db lessequal(0, 0)
db lessequal(0, 1)
db lessequal(1, 0)
db greater(0, 0)
db greater(0, 1)
db greater(1, 0)
db greaterequal(0, 0)
db greaterequal(0, 1)
db greaterequal(1, 0)

db and(0, 0)
db and(0, 1)
db and(1, 1)
db or(0, 0)
db or(0, 1)
db or(1, 1)
db nand(0, 0)
db nand(0, 1)
db nand(1, 1)
db nor(0, 0)
db nor(0, 1)
db nor(1, 1)
db xor(0, 0)
db xor(0, 1)
db xor(1, 1)

!testlabel = 1
!anothertestlabel = 2

if !testlabel == 0
	db $00
elseif !testlabel == 1 && !anothertestlabel == 2
	db $01
elseif !testlabel == 2 && !anothertestlabel == 1
	db $02
elseif !testlabel == 3 && !anothertestlabel == 0
	db $03
else
	db $FF
	
	while !testlabel > 0
		db $0A
	endif
endif

!mathlabel #= 0.01+0.01
!mathlabel #= !mathlabel*100
db !mathlabel

!whiletestvar = 5

while !whiletestvar > 0
	db !whiletestvar
	!whiletestvar #= !whiletestvar-1
endif

macro whilemacro1(numloops)
	!loopdyloop = <numloops>
	while !loopdyloop > 0
		db !loopdyloop
		!loopdyloop #= !loopdyloop-1
	endif
endmacro

macro whilemacro2()
	%whilemacro1(7)
endmacro

%whilemacro1(3)
%whilemacro2()


!cond1 = 0

while !cond1 < 10
	!cond2 = 0
	
	while !cond2 < 10
		db !cond1
		db !cond2
		!cond2 #= !cond2+1
	endif
	
	!cond1 #= !cond1+1
endif


db round(1.719247114, 2)*100
db round(100.6414710, 0)


function multilinefunction(x) = x+\		;;;;; This is a comment, getconnectedlines() should remove it
                               10
							  
db multilinefunction(1)


macro yetanothermacro()
	db \
	$0F
endmacro

%yetanothermacro()


db $0A		; This is a comment as well, so getconnectedlines() should remove it and ignore this backslah right here -> \
db $0B


print ""
print ";This is not a comment, ",\
    "so getconnectedlines() should ignore it and not remove it."
 
print ""
 
print "Let's have some random decimal number with fractional part: ",double(10.340173447601384024017510834015701571048)
print "Now the same number, but with a different precision: ",double(10.340173447601384024017510834015701571048, 10)
print "Testing a few more numbers: ",double(0.0),", ",double(0.1),", ",double(-0.1),", ",double(1.0),", ",double(-1.0)

print ""
