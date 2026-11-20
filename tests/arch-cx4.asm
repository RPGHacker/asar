arch cx4
org $008000
cx4_ram_offset = $34
; These encoding tests use A ($00) as the operand, and run past the end of a
; 512-byte page. arch-cx4-warnings.asm tests the warnings.
warnings disable Wcx4_a_operand_reads_zero
warnings disable Wcx4_write_ignored
warnings disable Wcx4_bus_write_ignores_value
warnings disable Wcx4_last_word_of_page

;`00 00
NOP

;`00 08
BRA $00
;`00 0A
BRA P:$00

;`00 0C
BEQ $00
;`00 0E
BEQ P:$00

;`00 10
BCS $00
;`00 12
BCS P:$00

;`00 14
BMI $00
;`00 16
BMI P:$00

;`00 18
BVS $00
;`00 1A
BVS P:$00

;`00 1C
WAIT

;`00 24
SKIPVC
;`01 24
SKIPVS
;`00 25
SKIPCC
;`01 25
SKIPCS
;`00 26
SKIPZC
;`01 26
SKIPZS
;`00 27
SKIPNC
;`01 27
SKIPNS

;`00 28
JSR $00
;`00 2A
JSR P:$00

;`00 2C
JEQ $00
;`00 2E
JEQ P:$00

;`00 30
JCS $00
;`00 32
JCS P:$00

;`00 34
JMI $00
;`00 36
JMI P:$00

;`00 38
JVS $00
;`00 3A
JVS P:$00

;`00 3C
RTS

;`00 40
INC MAR

;`00 48
CMPR A, A
;`00 48
CMPR A, A << 0
;`00 49
CMPR A, A << 1
;`00 4A
CMPR A, A << 8
;`00 4B
CMPR A, A << 16

;`00 4C
CMPR #$00, A
;`00 4C
CMPR #$00, A << 0
;`00 4D
CMPR #$00, A << 1
;`00 4E
CMPR #$00, A << 8
;`00 4F
CMPR #$00, A << 16

;`00 50
CMP A, A
;`00 50
CMP A << 0, A
;`00 51
CMP A << 1, A
;`00 52
CMP A << 8, A
;`00 53
CMP A << 16, A

;`00 54
CMP A, #$00
;`00 54
CMP A << 0, #$00
;`00 55
CMP A << 1, #$00
;`00 56
CMP A << 8, #$00
;`00 57
CMP A << 16, #$00

;`00 59
SXB
;`00 5A
SXW

;`00 60
LD A, A
;`00 61
LD MDR, A
;`00 62
LD MAR, A
; LD P only uses the low 4 bits, so registers other than R0-R15 warn
;`60 63
LD P, R0
;`6F 63
LD P, R15

; bus access registers
;`2E 61
LD MDR, BUSROM
;`2F 61
LD MDR, BUSRAM

; MDR as the source register
;`03 60
LD A, MDR
; RAMB, all 24 bits
;`0C 60
LD A, RAMB

;`00 64
LD A, #$00
;`00 65
LD MDR, #$00
;`00 66
LD MAR, #$00
;`00 67
LD P, #$00

;`00 68
RDRAM RAMB+0, A
;`00 69
RDRAM RAMB+1, A
;`00 6A
RDRAM RAMB+2, A

;`00 6C
RDRAM RAMB+0, DPR+$00
;`00 6D
RDRAM RAMB+1, DPR+$00
;`00 6E
RDRAM RAMB+2, DPR+$00
;`FF 6C
RDRAM RAMB+0, DPR+$FF
; any case, spaces around the '+', and expressions after it
;`20 6D
rdram Ramb + 1, dpr + $10*2
;`34 6E
RDRAM RAMB+(1+1), DPR+cx4_ram_offset

;`00 70
RDROM A

;`00 74
RDROM $000
;`00 75
RDROM $100
;`00 76
RDROM $200
;`00 77
RDROM $300
;`FF 77
RDROM $3FF
;`23 75
RDROM $100+$23

;`00 7C
LD PL, #$00
;`00 7D
LD PH, #$00
;`7F 7D
LD PH, #$7F

;`00 80
ADD A, A
;`00 80
ADD A << 0, A
;`00 81
ADD A << 1, A
;`00 82
ADD A << 8, A
;`00 83
ADD A << 16, A

;`00 84
ADD A, #$00
;`00 84
ADD A << 0, #$00
;`00 85
ADD A << 1, #$00
;`00 86
ADD A << 8, #$00
;`00 87
ADD A << 16, #$00

;`00 88
SUBR A, A
;`00 88
SUBR A, A << 0
;`00 89
SUBR A, A << 1
;`00 8A
SUBR A, A << 8
;`00 8B
SUBR A, A << 16

;`00 8C
SUBR #$00, A
;`00 8C
SUBR #$00, A << 0
;`00 8D
SUBR #$00, A << 1
;`00 8E
SUBR #$00, A << 8
;`00 8F
SUBR #$00, A << 16

;`00 90
SUB A, A
;`00 90
SUB A << 0, A
;`00 91
SUB A << 1, A
;`00 92
SUB A << 8, A
;`00 93
SUB A << 16, A

;`00 94
SUB A, #$00
;`00 94
SUB A << 0, #$00
;`00 95
SUB A << 1, #$00
;`00 96
SUB A << 8, #$00
;`00 97
SUB A << 16, #$00

;`00 98
SMUL A, A

;`00 9C
SMUL A, #$00

;`00 A0
XNOR A, A
;`00 A0
XNOR A << 0, A
;`00 A1
XNOR A << 1, A
;`00 A2
XNOR A << 8, A
;`00 A3
XNOR A << 16, A

;`00 A4
XNOR A, #$00
;`00 A4
XNOR A << 0, #$00
;`00 A5
XNOR A << 1, #$00
;`00 A6
XNOR A << 8, #$00
;`00 A7
XNOR A << 16, #$00

;`00 A8
XOR A, A
;`00 A8
XOR A << 0, A
;`00 A9
XOR A << 1, A
;`00 AA
XOR A << 8, A
;`00 AB
XOR A << 16, A

;`00 AC
XOR A, #$00
;`00 AC
XOR A << 0, #$00
;`00 AD
XOR A << 1, #$00
;`00 AE
XOR A << 8, #$00
;`00 AF
XOR A << 16, #$00

;`00 B0
AND A, A
;`00 B0
AND A << 0, A
;`00 B1
AND A << 1, A
;`00 B2
AND A << 8, A
;`00 B3
AND A << 16, A

;`00 B4
AND A, #$00
;`00 B4
AND A << 0, #$00
;`00 B5
AND A << 1, #$00
;`00 B6
AND A << 8, #$00
;`00 B7
AND A << 16, #$00

;`00 B8
OR A, A
;`00 B8
OR A << 0, A
;`00 B9
OR A << 1, A
;`00 BA
OR A << 8, A
;`00 BB
OR A << 16, A

;`00 BC
OR A, #$00
;`00 BC
OR A << 0, #$00
;`00 BD
OR A << 1, #$00
;`00 BE
OR A << 8, #$00
;`00 BF
OR A << 16, #$00

;`00 C0
SHR A, A

;`00 C4
SHR A, #$00
;`18 C4
SHR A, #24

;`00 C8
ASR A, A

;`00 CC
ASR A, #$00
;`18 CC
ASR A, #24

;`00 D0
ROR A, A

;`00 D4
ROR A, #$00
;`18 D4
ROR A, #24

;`00 D8
SHL A, A

;`00 DC
SHL A, #$00
;`18 DC
SHL A, #24

;`00 E0
ST A, A
;`00 E1
ST A, MDR
;`2E E1
ST BUSROM, MDR
;`2F E0
ST BUSRAM, A
;`03 E0
ST MDR, A

;`00 E8
WRRAM A, RAMB+0
;`00 E9
WRRAM A, RAMB+1
;`00 EA
WRRAM A, RAMB+2

;`00 EC
WRRAM DPR+$00, RAMB+0
;`00 ED
WRRAM DPR+$00, RAMB+1
;`00 EE
WRRAM DPR+$00, RAMB+2
;`FF EC
WRRAM DPR+$FF, RAMB+0
;`20 ED
wrram dpr + $10*2, Ramb + 1
;`34 EE
WRRAM DPR+cx4_ram_offset, RAMB+(1+1)

; full register code, like LD P
;`60 F0
SWAP A, R0
;`6F F0
SWAP A, R15

;`00 F8
CLEAR

;`00 FC
STOP

; Constants, by name or as # and their 6-digit value
;`50 60
LD A, IR0
;`5F 60
LD A, IR15
;`50 60
LD A, #$000000
;`51 60
LD A, #$FFFFFF
;`52 60
LD A, #$00FF00
;`53 60
LD A, #$FF0000
;`54 60
LD A, #$00FFFF
;`55 60
LD A, #$FFFF00
;`56 60
LD A, #$800000
;`57 60
LD A, #$7FFFFF
;`58 60
LD A, #$008000
;`59 60
LD A, #$007FFF
;`5A 60
LD A, #$FF7FFF
;`5B 60
LD A, #$FFFF7F
;`5C 60
LD A, #$010000
;`5D 60
LD A, #$FEFFFF
;`5E 60
LD A, #$000100
;`5F 60
LD A, #$00FEFF
;`52 B2
and a << 8, #$00ff00
;`51 49
CMPR #$FFFFFF, A << 1
;`56 98
SMUL A, #$800000
;`warnWcx4_constant_shift_count
;`5E C0
SHR A, #$000100
;`57 61
LD MDR, #$7FFFFF
!mask = $FF0000
;`53 B8
OR A, #!mask
;`50 50
CMP A, #$000000
; anything else after # is an 8-bit immediate
;`00 54
CMP A, #$00
;`12 64
LD A, #$000012
;`00 67
LD P, #$000000
;`00 7C
LD PL, #$000000

; Branch and jump targets. A plain number is the word index itself. A target
; with a label is a byte address, and its word index is (address >> 1) & $FF.
org $008200
;`000200

;`12 08
BRA $12
;`FF 0E
BEQ P:$FF

; $008204, word index $02
back:
;`02 08
BRA back
;`02 0A
BRA P:back
;`02 0C
BEQ back
;`02 0E
BEQ P:back
;`02 10
BCS back
;`02 12
BCS P:back
;`02 14
BMI back
;`02 16
BMI P:back
;`02 18
BVS back
;`02 1A
BVS P:back
;`02 28
JSR back
;`02 2A
JSR P:back
;`02 2C
JEQ back
;`02 2E
JEQ P:back
;`02 30
JCS back
;`02 32
JCS P:back
;`02 34
JMI back
;`02 36
JMI P:back
;`02 38
JVS back
;`02 3A
JVS P:back

; forward is at $008236, word index $1B
;`1B 08
BRA forward
;`1B 2A
jsr p:forward

; back+4 is $008208, word index $04
;`04 08
BRA back+4

; + and - are at $008234, word index $1A
;`1A 08
BRA +
+
;`1A 08
- BRA -

forward:
;`00 00
NOP

; far_target is in another page, at $008456, word index $2B
;`2B 0A
BRA P:far_target
;`2B 2A
JSR P:far_target

; base changes label addresses only, e.g. to count pages from the start of
; the Cx4 program. other_page is $000806: page 4, word index $03.
base $000200
;`00 08
- BRA -
;`FF 08
BRA $FF
;`03 0A
BRA P:other_page
base $000806
;`00 00
other_page: NOP
base off

org $008456
;`000456
;`00 00
far_target: NOP
