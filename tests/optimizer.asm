;`+
;namespace optimize_dp_flag {
;	enum : int {
;		NONE,	//don't optimize
;		RAM,	//bank 7E only (always uses dp base)
;		ALWAYS	//bank 00-3F[|80] and 7E (always uses dp base)
;	};
;}
;
;extern int optimize_dp;
;extern int dp_base;
;
;namespace optimize_address_flag {
;	enum : int {
;		DEFAULT,//simply use optimizeforbank
;		RAM,	//default+bank 7E only RAM address < $2000
;		MIRRORS	//ram+if optimizeforbank is 00-3F[|80] and address < $8000
;	};
;}
;
;extern int optimize_address;

;quick ref
;lda long = AF
;lda word = AD
;lda dp = A5



org $008000

base $000000
struct test_00
	.test: skip $100
	.test_0100: skip 1
endstruct

base $011000
struct test_word
	.up: skip 1
endstruct

base $7E0000
struct test_7E
	.test: skip $1000
	.word: skip 1

endstruct

mirror = $026000
nonmirror = $008000
nonmirror01 = $018000

base off
optimize address default
optimize dp none
;test default asar optimization (bank only)
;`AD 00 00 AF 00 00 7E
lda test_00.test
lda test_7E.test

;test optimizations of dp on with dp base as default 0000 in bank 7E only
;optimize dp ram
optimize dp ram
dpbase $0000
;`AD 00 00 A5 00
lda test_00.test
lda test_7E.test

optimize dp always
;`A5 00
lda test_00.test

dpbase $0100
;`AD 00 00 A5 00
lda test_00.test
lda test_00.test_0100

optimize dp none
optimize address ram
;`AF 00 10 01 AD 00 10
lda test_word.up
lda test_7E.word

optimize address mirrors
;`AD 00 10
lda test_word.up

bank noassume
asdf:
;`AF 1D 80 00
lda asdf
bank $10
;`AF 1D 80 00
lda asdf
bank auto
;`AD 1D 80
lda asdf

;`AD 00 80 AF 00 80 01
lda nonmirror
lda nonmirror01

assert pc() <= $009000

bank80label = $808080
segment bank=$80
;`AD 80 80
lda bank80label
banksegmentlabel:

segment bank=$80
;`AD 32 80
lda banksegmentlabel
;`AF 00 80 81
lda bank1segmentlabel

segment bank=$81
bank1segmentlabel:
;`08000 EA
nop

;`80000 53 54 41 52 26 00 D9 FF
freecode cleaned
optimize dp none
optimize address default
freec:
;`  AD 08 80  AF 00 00 00  AF 00 60 02
lda freec
lda test_00.test
lda mirror

optimize address ram
;`  AD 00 00  AF 00 10 01  AF 00 60 02
lda test_7E.test
lda test_word.up
lda mirror

optimize address mirrors
;`  AD 00 00  AD 00 10  AD 00 60  AF 00 80 00
lda test_7E.test
lda test_word.up
lda mirror
lda nonmirror

;`  AF 37 80 90
lda freed

;`53 54 41 52 16 00 E9 FF
freedata cleaned
freed:
optimize address default
;`  AF 08 80 90  AD 37 80  AF 00 00 00
lda freec
lda freed
lda test_00.test

optimize address ram
;`  AF 00 00 7E
lda test_7E.test

optimize address mirrors
;`  AF 00 00 7E  AF 00 60 02
lda test_7E.test
lda mirror

segment
pinnedseg:
;`EA
nop

segment pin=pinnedseg
;`AD 4E 80
lda pinnedseg
;`AF 5A 80 90
lda unpinnedseg
;`AF 00 80 81
lda bank1segmentlabel

segment
unpinnedseg:
;`EA
nop

;`FFFFF 00
