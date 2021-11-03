;`007FFF
;`00
;`008000
;`00
;`FF FF 00
;`00 80 01
;`00 00 00
;`FF FF 00
;`00 80 01
;`EA EA
;`00FFFF
;`00
;`010000
;`00
;`FF FF 00
;`00 00 01
;`00 00 00
;`FF FF 00
;`00 00 01
;`EA EA
;`000015
;`00 00 00
;`00 00 00
;`15 00 00
;`18 00 00
;`020000
;`00 00 00
;`00 00 00
;`15 00 00
;`18 00 00
;`02FFF8
;`00 01 02 03
;`057FFC
;`30 20 10 00
;`FF FE
;`037FF8
;`42 42 42 42 42 42 42 42

;disabling mapper change warning as a bonus test instead
;of requiring the warning like other tests
warnings disable Wmapper_already_set
warnings disable Wfeature_deprecated
check bankcross off

org $00FFFF

First:
	db $00
	
Second:
	db $00
	
	dl First
	dl Second
	
check bankcross on

	db $00,$00,$00
	
	dl First
	dl Second
	
	nop #2

	
	
norom

check bankcross off

org $00FFFF

Third:
	db $00
	
Fourth:
	db $00
	
	dl Third
	dl Fourth
	
check bankcross on

	db $00,$00,$00
	
	dl Third
	dl Fourth
	
	nop #2
	
	
	
check bankcross off

org $000015

Fifth:
	dl $000000
Sixth:
	dl $000000
	dl Fifth
	dl Sixth
	
check bankcross on



org $020000

check bankcross off

base $000015

Seventh:
	dl $000000
	
Eigth:
	dl $000000
	dl Seventh
	dl Eigth
	
base off

check bankcross on



lorom
org $05FFF8
db $00
db $01
db $02
db $03

norom
check bankcross off

org $057FFC
base $00002

db $30
db $20
db $10
db $00 ;x7FFF
db $FF ;x8000
db $FE

lorom
base off
check bankcross on

org $06FFF8
padbyte $42 : pad $078000
