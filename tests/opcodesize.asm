;`65 00 6D 00 00 6F 00 00 00 75 00 7D 00 00 7F 00 00 00 69 00 69 00 00
;`25 00 2D 00 00 2F 00 00 00 35 00 3D 00 00 3F 00 00 00 29 00 29 00 00
;`06 00 0E 00 00 16 00 1E 00 00
;`24 00 2C 00 00 34 00 3C 00 00 89 00 89 00 00
;`C5 00 CD 00 00 CF 00 00 00 D5 00 DD 00 00 DF 00 00 00 C9 00 C9 00 00
;`E4 00 EC 00 00 E0 00 E0 00 00
;`C4 00 CC 00 00 C0 00 C0 00 00
;`C6 00 CE 00 00 D6 00 DE 00 00
;`45 00 4D 00 00 4F 00 00 00 55 00 5D 00 00 5F 00 00 00 49 00 49 00 00
;`E6 00 EE 00 00 F6 00 FE 00 00
;`A5 00 AD 00 00 AF 00 00 00 B5 00 BD 00 00 BF 00 00 00 A9 00 A9 00 00
;`A6 00 AE 00 00 B6 00 BE 00 00 A2 00 A2 00 00
;`A4 00 AC 00 00 B4 00 BC 00 00 A0 00 A0 00 00
;`46 00 4E 00 00 56 00 5E 00 00
;`05 00 0D 00 00 0F 00 00 00 15 00 1D 00 00 1F 00 00 00 09 00 09 00 00
;`26 00 2E 00 00 36 00 3E 00 00
;`66 00 6E 00 00 76 00 7E 00 00
;`E5 00 ED 00 00 EF 00 00 00 F5 00 FD 00 00 FF 00 00 00 E9 00 E9 00 00
;`85 00 8D 00 00 8F 00 00 00 95 00 9D 00 00 9F 00 00 00
;`86 00 8E 00 00
;`84 00 8C 00 00
;`64 00 9C 00 00 74 00 9E 00 00
;`14 00 1C 00 00
;`04 00 0C 00 00
;`84 00 85 00 00 94 00 95 00 00
;`24 00 25 00 00 34 00 35 00 00
;`0B 00 0C 00 00
;`64 00 65 00 00 74 00 75 00 00
;`3E 00 1E 00 00
;`7E 00 5E 00 00
;`8B 00 8C 00 00
;`44 00 45 00 00 54 00 55 00 00
;`AB 00 AC 00 00
;`4B 00 4C 00 00
;`E4 00 E5 00 00 F4 00 F5 00 00
;`F8 00 E9 00 00
;`EB 00 EC 00 00
;`C4 00 C5 00 00 D4 00 D5 00 00
;`D8 00 C9 00 00
;`CB 00 CC 00 00
;`04 00 05 00 00 14 00 15 00 00
;`2B 00 2C 00 00
;`6B 00 6C 00 00
;`A4 00 A5 00 00 B4 00 B5 00 00
;`84 00 85 00 00 94 00 95 00 00
;`24 00 25 00 00 34 00 35 00 00
;`0B 00 0C 00 00
;`64 00 65 00 00 74 00 75 00 00
;`3E 00 1E 00 00
;`7E 00 5E 00 00
;`8B 00 8C 00 00
;`44 00 45 00 00 54 00 55 00 00
;`AB 00 AC 00 00
;`4B 00 4C 00 00
;`E4 00 E5 00 00 F4 00 F5 00 00
;`F8 00 E9 00 00
;`EB 00 EC 00 00
;`C4 00 C5 00 00 D4 00 D5 00 00
;`D8 00 C9 00 00
;`CB 00 CC 00 00
;`04 00 05 00 00 14 00 15 00 00
;`2B 00 2C 00 00
;`6B 00 6C 00 00
;`A4 00 A5 00 00 B4 00 B5 00 00

arch 65816
org $008000

adc.b $00
adc.w $00
adc.l $00
adc.b $00,X
adc.w $00,X
adc.l $00,X
adc.b #$00
adc.w #$00

and.b $00
and.w $00
and.l $00
and.b $00,X
and.w $00,X
and.l $00,X
and.b #$00
and.w #$00

asl.b $00
asl.w $00
asl.b $00,X
asl.w $00,X

bit.b $00
bit.w $00
bit.b $00,X
bit.w $00,X
bit.b #$00
bit.w #$00

cmp.b $00
cmp.w $00
cmp.l $00
cmp.b $00,X
cmp.w $00,X
cmp.l $00,X
cmp.b #$00
cmp.w #$00

cpx.b $00
cpx.w $00
cpx.b #$00
cpx.w #$00

cpy.b $00
cpy.w $00
cpy.b #$00
cpy.w #$00

dec.b $00
dec.w $00
dec.b $00,X
dec.w $00,X

eor.b $00
eor.w $00
eor.l $00
eor.b $00,X
eor.w $00,X
eor.l $00,X
eor.b #$00
eor.w #$00

inc.b $00
inc.w $00
inc.b $00,X
inc.w $00,X

lda.b $00
lda.w $00
lda.l $00
lda.b $00,X
lda.w $00,X
lda.l $00,X
lda.b #$00
lda.w #$00

ldx.b $00
ldx.w $00
ldx.b $00,Y
ldx.w $00,Y
ldx.b #$00
ldx.w #$00

ldy.b $00
ldy.w $00
ldy.b $00,X
ldy.w $00,X
ldy.b #$00
ldy.w #$00

lsr.b $00
lsr.w $00
lsr.b $00,X
lsr.w $00,X

ora.b $00
ora.w $00
ora.l $00
ora.b $00,X
ora.w $00,X
ora.l $00,X
ora.b #$00
ora.w #$00

rol.b $00
rol.w $00
rol.b $00,X
rol.w $00,X

ror.b $00
ror.w $00
ror.b $00,X
ror.w $00,X

sbc.b $00
sbc.w $00
sbc.l $00
sbc.b $00,X
sbc.w $00,X
sbc.l $00,X
sbc.b #$00
sbc.w #$00

sta.b $00
sta.w $00
sta.l $00
sta.b $00,X
sta.w $00,X
sta.l $00,X

stx.b $00
stx.w $00

sty.b $00
sty.w $00

stz.b $00
stz.w $00
stz.b $00,X
stz.w $00,X

trb.b $00
trb.w $00

tsb.b $00
tsb.w $00

arch spc700
adc.b A, $0000
adc.w A, $0000
adc.b A, $0000+X
adc.w A, $0000+X

and.b A, $0000
and.w A, $0000
and.b A, $0000+X
and.w A, $0000+X

asl.b $0000
asl.w $0000

cmp.b A, $0000
cmp.w A, $0000
cmp.b A, $0000+X
cmp.w A, $0000+X

cmp.b X, $0000
cmp.w X, $0000

cmp.b Y, $0000
cmp.w Y, $0000

dec.b $0000
dec.w $0000

eor.b A, $0000
eor.w A, $0000
eor.b A, $0000+X
eor.w A, $0000+X

inc.b $0000
inc.w $0000

lsr.b $0000
lsr.w $0000

mov.b A, $0000
mov.w A, $0000
mov.b A, $0000+X
mov.w A, $0000+X

mov.b X, $0000
mov.w X, $0000

mov.b Y, $0000
mov.w Y, $0000

mov.b $0000, A
mov.w $0000, A
mov.b $0000+X, A
mov.w $0000+X, A

mov.b $0000, X
mov.w $0000, X

mov.b $0000, Y
mov.w $0000, Y

or.b A, $0000
or.w A, $0000
or.b A, $0000+X
or.w A, $0000+X

rol.b $0000
rol.w $0000

ror.b $0000
ror.w $0000

sbc.b A, $0000
sbc.w A, $0000
sbc.b A, $0000+X
sbc.w A, $0000+X


adc.b A, $00
adc.w A, $00
adc.b A, $00+X
adc.w A, $00+X

and.b A, $00
and.w A, $00
and.b A, $00+X
and.w A, $00+X

asl.b $00
asl.w $00

cmp.b A, $00
cmp.w A, $00
cmp.b A, $00+X
cmp.w A, $00+X

cmp.b X, $00
cmp.w X, $00

cmp.b Y, $00
cmp.w Y, $00

dec.b $00
dec.w $00

eor.b A, $00
eor.w A, $00
eor.b A, $00+X
eor.w A, $00+X

inc.b $00
inc.w $00

lsr.b $00
lsr.w $00

mov.b A, $00
mov.w A, $00
mov.b A, $00+X
mov.w A, $00+X

mov.b X, $00
mov.w X, $00

mov.b Y, $00
mov.w Y, $00

mov.b $00, A
mov.w $00, A
mov.b $00+X, A
mov.w $00+X, A

mov.b $00, X
mov.w $00, X

mov.b $00, Y
mov.w $00, Y

or.b A, $00
or.w A, $00
or.b A, $00+X
or.w A, $00+X

rol.b $00
rol.w $00

ror.b $00
ror.w $00

sbc.b A, $00
sbc.w A, $00
sbc.b A, $00+X
sbc.w A, $00+X
