;`+
;`56 78
;`BA
;`BA
;`EE EE EE
;`03
;`42
;`007FFE EE EE
;P>Test passed. (1)
;P>Test passed. (2)
;P>Test passed. (3)
;P>Test passed. (4)
;P>Test passed. (5)

org $008000
AStaticLabel = 10

struct TestStruct $000000
	.first: skip 1
	.second: skip 1
	.count: skip 1
endstruct

struct NewStruct extends TestStruct
	.new: skip 1
endstruct

if AStaticLabel == 10
	print "Test passed. (1)"
endif

if TestStruct.count == 2
	print "Test passed. (2)"
endif

if TestStruct[0].count == 2
	print "Test passed. (3)"
endif

if TestStruct.NewStruct.new == 3
	print "Test passed. (4)"
endif

if TestStruct.NewStruct[0].new == 3
	print "Test passed. (5)"
endif

; RPG Hacker: Not that this made much sense, but, you know... we can now.
IncbinStart = $008000
IncbinEnd = $008002

incbin "data/64kb.bin":IncbinStart..IncbinEnd

FunStuff = $BA
'a' = FunStuff

OtherFunStuff = FunStuff

db 'a'
db OtherFunStuff

FillByte = $EE
FillCount = 3

fillbyte FillByte
fill FillCount

!adefine #= FillCount

db !adefine

ArgID = 2

macro in_macro(...)
	db <...[ArgID]>
endmacro

%in_macro($40, $41, $42)


org $00FFFE

padbyte FillByte
pad $018000

