;`000000
;`00 00 00 00 80 00 00 00 20 00 00 00 00 00 20 FF FF 3F 00 80 80 00 80 81 00 80 C0 FF FF FF
;`000100
;`00 80 00 00 80 01 00 00 00 00 80 00 00 80 00 00 00 00 00 80 00 FF FF 3F 00 00 C0 00 80 C0 00 80 C1 FF FF FF
;`000200
;`00 00 40 00 80 40 00 00 60 00 00 00 00 00 20 FF FF 3F 00 80 80 00 80 C0 FF FF FF 00 80 00 00 80 01 00 80 40
;`000300
;`00 80 40 00 80 41 00 00 40 00 80 40 00 80 00 00 00 00 00 80 00 FF FF 3F 00 00 C0 00 80 C0 FF FF FF 00 00 40 00 80 40 00 80 41
;`008000
;`FF FF FF
;`00 01 02 03 04 05 06 07 00 01 02 03
;`warnWmapper_already_set
;`warnWmapper_already_set
;`warnWmapper_already_set
;`warnWmapper_already_set
asar 1.50

lorom
	org $008000

	dl snestopc($008000)
	dl snestopc($018000)
	dl snestopc($408000)
	dl snestopc($808000)
	dl snestopc($C08000)
	dl snestopc($FFFFFF)

	dl pctosnes($000000)
	dl pctosnes($008000)
	dl pctosnes($200000)
	dl pctosnes($3FFFFF)
	

hirom
	org $400100
	
	dl snestopc($008000)
	dl snestopc($018000)
	dl snestopc($400000)
	dl snestopc($408000)
	dl snestopc($808000)
	dl snestopc($C00000)
	dl snestopc($C08000)
	dl snestopc($FFFFFF)

	dl pctosnes($000000)
	dl pctosnes($008000)
	dl pctosnes($018000)
	dl pctosnes($3FFFFF)
	

exlorom
	org $808200
	
	dl snestopc($008000)
	dl snestopc($018000)
	dl snestopc($408000)
	dl snestopc($808000)
	dl snestopc($C08000)
	dl snestopc($FFFFFF)

	dl pctosnes($000000)
	dl pctosnes($200000)
	dl pctosnes($3FFFFF)
	dl pctosnes($400000)
	dl pctosnes($408000)
	dl pctosnes($600000)
	

exhirom
	org $C00300
	
	dl snestopc($008000)
	dl snestopc($018000)
	dl snestopc($400000)
	dl snestopc($408000)
	dl snestopc($808000)
	dl snestopc($C00000)
	dl snestopc($C08000)
	dl snestopc($FFFFFF)

	dl pctosnes($000000)
	dl pctosnes($008000)
	dl pctosnes($3FFFFF)
	dl pctosnes($400000)
	dl pctosnes($408000)
	dl pctosnes($418000)
	
lorom
	org $018000
	
	dl $FFFFFF
	
	incsrc "data/pushtable1.asm"
	db "ABCD"
	
	pushtable
	
	incsrc "data/pushtable2.asm"
	db "ABCD"
	
	pulltable
	
	db "ABCD"
	
