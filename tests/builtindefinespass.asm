;`01 03 06 07

org $008000

if stringsequal("!assembler", "asar")
	db $01
else
	db $02
endif

if stringsequalnocase("!assembler", "ASAR")
	db $03
else
	db $04
endif

if stringsequal("!assembler", "xkas")
	db $05
else
	db $06
endif

if !assembler_ver >= 10602
	db $07
else
	db $08
endif
