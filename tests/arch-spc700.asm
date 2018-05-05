;`86         87 12      88 12      95 34 12   94 12      96 34 12   97 12      85 34 12   84 12      99         98 34 12   89 34 12
;`26         27 12      28 12      35 34 12   34 12      36 34 12   37 12      25 34 12   24 12      39         38 34 12   29 34 12
;`46         47 12      48 12      55 34 12   54 12      56 34 12   57 12      45 34 12   44 12      59         58 34 12   49 34 12
;`06         07 12      08 12      15 34 12   14 12      16 34 12   17 12      05 34 12   04 12      19         18 34 12   09 34 12
;`A6         A7 12      A8 12      B5 34 12   B4 12      B6 34 12   B7 12      A5 34 12   A4 12      B9         B8 34 12   A9 34 12
;`1C         1B 12      0C 34 12   0B 12
;`5C         5B 12      4C 34 12   4B 12
;`3C         3B 12      2C 34 12   2B 12
;`7C         7B 12      6C 34 12   6B 12
;`13 12 3F   33 12 3C   53 12 39   73 12 36   93 12 33   B3 12 30   D3 12 2D   F3 12 2A
;`03 12 27   23 12 24   43 12 21   63 12 1E   83 12 1B   A3 12 18   C3 12 15   E3 12 12
;`10 10      2F 0E      30 0C      50 0A      70 08      90 06      B0 04      D0 02      F0 00
;`02 12      22 12      42 12      62 12      82 12      A2 12      C2 12      E2 12
;`12 12      32 12      52 12      72 12      92 12      B2 12      D2 12      F2 12
;`66         67 12      68 12      75 34 12   74 12      76 34 12   77 12      65 34 12   64 12
;`C8 12      1E 34 12   3E 12      AD 12      5E 34 12   7E 12      79         78 34 12   69 34 12
;`DE 12 B4   2E 12 B1   FE AF      6E 12 AC   DF         BE         EA 34 32   9F         AA 34 32   CA 34 32
;`1A 12      3A 12      5A 12      7A 12      9A 12      BA 12      DA 12      CF         9E
;`9C         1D         DC         9B 12      8C 34 12   8B 12
;`BC         3D         FC         BB 12      AC 34 12   AB 12
;`5D         7D         9D         BD         DD         FD         AF         BF         C6         E6         8D 12      CD 12      E8 12
;`C7 12      D7 12      E7 12      F7 12      D5 34 12   D4 12      D6 34 12   D9 12      DB 12      F9 12      FB 12      F5 34 12   F4 12      F6 34 12
;`C5 34 12   C4 12      C9 34 12   D8 12      CC 34 12   CB 12      E5 34 12   E4 12      E9 34 12   F8 12      EC 34 12   EB 12      8F 34 12   FA 34 12
;`0A 34 32   2A 34 32   4A 34 32   6A 34 32   8A 34 32
;`01         11         21         31         41         51         61         71
;`81         91         A1         B1         C1         D1         E1         F1
;`0E 34 12   4E 34 12
;`3F 34 12   4F 12      1F 34 12   5F 34 12
;`0D         2D         4D         6D
;`8E         AE         CE         EE
;`00         0F         6F         7F         20         40         60         80         A0         C0         E0         ED         EF         FF

org 32768
arch spc700
ADC A,(X)                              ; 86
ADC A,($12+X)                          ; 87 12
ADC A,#$12                             ; 88 12
ADC A,$1234+X                          ; 95 34 12
ADC A,$12+X                            ; 94 12
ADC A,$1234+Y                          ; 96 34 12
ADC A,($12)+Y                          ; 97 12
ADC A,$1234                            ; 85 34 12
ADC A,$12                              ; 84 12
ADC (X),(Y)                            ; 99
ADC $12,#$34                           ; 98 34 12
ADC $12,$34                            ; 89 34 12

AND A,(X)                              ; 26
AND A,($12+X)                          ; 27 12
AND A,#$12                             ; 28 12
AND A,$1234+X                          ; 35 34 12
AND A,$12+X                            ; 34 12
AND A,$1234+Y                          ; 36 34 12
AND A,($12)+Y                          ; 37 12
AND A,$1234                            ; 25 34 12
AND A,$12                              ; 24 12
AND (X),(Y)                            ; 39
AND $12,#$34                           ; 38 34 12
AND $12,$34                            ; 29 34 12

EOR A,(X)                              ; 46
EOR A,($12+X)                          ; 47 12
EOR A,#$12                             ; 48 12
EOR A,$1234+X                          ; 55 34 12
EOR A,$12+X                            ; 54 12
EOR A,$1234+Y                          ; 56 34 12
EOR A,($12)+Y                          ; 57 12
EOR A,$1234                            ; 45 34 12
EOR A,$12                              ; 44 12
EOR (X),(Y)                            ; 59
EOR $12,#$34                           ; 58 34 12
EOR $12,$34                            ; 49 34 12

OR A,(X)                               ; 06
OR A,($12+X)                           ; 07 12
OR A,#$12                              ; 08 12
OR A,$1234+X                           ; 15 34 12
OR A,$12+X                             ; 14 12
OR A,$1234+Y                           ; 16 34 12
OR A,($12)+Y                           ; 17 12
OR A,$1234                             ; 05 34 12
OR A,$12                               ; 04 12
OR (X),(Y)                             ; 19
OR $12,#$34                            ; 18 34 12
OR $12,$34                             ; 09 34 12

SBC A,(X)                              ; A6
SBC A,($12+X)                          ; A7 12
SBC A,#$12                             ; A8 12
SBC A,$1234+X                          ; B5 34 12
SBC A,$12+X                            ; B4 12
SBC A,$1234+Y                          ; B6 34 12
SBC A,($12)+Y                          ; B7 12
SBC A,$1234                            ; A5 34 12
SBC A,$12                              ; A4 12
SBC (X),(Y)                            ; B9
SBC $12,#$34                           ; B8 34 12
SBC $12,$34                            ; A9 34 12

ASL A                                  ; 1C
ASL $12+X                              ; 1B 12
ASL $1234                              ; 0C 34 12
ASL $12                                ; 0B 12

LSR A                                  ; 5C
LSR $12+X                              ; 5B 12
LSR $1234                              ; 4C 34 12
LSR $12                                ; 4B 12

ROL A                                  ; 3C
ROL $12+X                              ; 3B 12
ROL $1234                              ; 2C 34 12
ROL $12                                ; 2B 12

ROR A                                  ; 7C
ROR $12+X                              ; 7B 12
ROR $1234                              ; 6C 34 12
ROR $12                                ; 6B 12

BBC0 $12,Mylabel                       ; 13 12 FF
BBC1 $12,Mylabel                       ; 33 12 FF
BBC2 $12,Mylabel                       ; 53 12 FF
BBC3 $12,Mylabel                       ; 73 12 FF
BBC4 $12,Mylabel                       ; 93 12 FF
BBC5 $12,Mylabel                       ; B3 12 FF
BBC6 $12,Mylabel                       ; D3 12 FF
BBC7 $12,Mylabel                       ; F3 12 FF

BBS0 $12,Mylabel                       ; 03 12 FF
BBS1 $12,Mylabel                       ; 23 12 FF
BBS2 $12,Mylabel                       ; 43 12 FF
BBS3 $12,Mylabel                       ; 63 12 FF
BBS4 $12,Mylabel                       ; 83 12 FF
BBS5 $12,Mylabel                       ; A3 12 FF
BBS6 $12,Mylabel                       ; C3 12 FF
BBS7 $12,Mylabel                       ; E3 12 FF

BPL Mylabel                            ; 10 FF
BRA Mylabel                            ; 2F FF
BMI Mylabel                            ; 30 FF
BVC Mylabel                            ; 50 FF
BVS Mylabel                            ; 70 FF
BCC Mylabel                            ; 90 FF
BCS Mylabel                            ; B0 FF
BNE Mylabel                            ; D0 FF
BEQ Mylabel                            ; F0 FF

Mylabel:

SET0 $12                               ; 02 12
SET1 $12                               ; 22 12
SET2 $12                               ; 42 12
SET3 $12                               ; 62 12
SET4 $12                               ; 82 12
SET5 $12                               ; A2 12
SET6 $12                               ; C2 12
SET7 $12                               ; E2 12

CLR0 $12                               ; 12 12
CLR1 $12                               ; 32 12
CLR2 $12                               ; 52 12
CLR3 $12                               ; 72 12
CLR4 $12                               ; 92 12
CLR5 $12                               ; B2 12
CLR6 $12                               ; D2 12
CLR7 $12                               ; F2 12

CMP A,(X)                              ; 66
CMP A,($12+X)                          ; 67 12
CMP A,#$12                             ; 68 12
CMP A,$1234+X                          ; 75 34 12
CMP A,$12+X                            ; 74 12
CMP A,$1234+Y                          ; 76 34 12
CMP A,($12)+Y                          ; 77 12
CMP A,$1234                            ; 65 34 12
CMP A,$12                              ; 64 12
CMP X,#$12                             ; C8 12
CMP X,$1234                            ; 1E 34 12
CMP X,$12                              ; 3E 12
CMP Y,#$12                             ; AD 12
CMP Y,$1234                            ; 5E 34 12
CMP Y,$12                              ; 7E 12
CMP (X),(Y)                            ; 79
CMP $12,#$34                           ; 78 34 12
CMP $12,$34                            ; 69 34 12

CBNE $12+x,Mylabel                     ; DE 12 FF
CBNE $12,Mylabel                       ; 2E 12 FF
DBNZ Y,Mylabel                         ; FE FF
DBNZ $12,Mylabel                       ; 6E 12 FF
DAA A                                  ; DF
DAS A                                  ; BE
NOT1 $1234                             ; EA 34 32
XCN A                                  ; 9F
MOV1 C,$1234                           ; AA 34 32
MOV1 $1234,C                           ; CA 34 32

DECW $12                               ; 1A 12
INCW $12                               ; 3A 12
CMPW YA,$12                            ; 5A 12
ADDW YA,$12                            ; 7A 12
SUBW YA,$12                            ; 9A 12
MOVW YA,$12                            ; BA 12
MOVW $12,YA                            ; DA 12
MUL YA                                 ; CF
DIV YA,X                               ; 9E

DEC A                                  ; 9C
DEC X                                  ; 1D
DEC Y                                  ; DC
DEC $12+X                              ; 9B 12
DEC $1234                              ; 8C 34 12
DEC $12                                ; 8B 12

INC A                                  ; BC
INC X                                  ; 3D
INC Y                                  ; FC
INC $12+X                              ; BB 12
INC $1234                              ; AC 34 12
INC $12                                ; AB 12

MOV X,A                                ; 5D
MOV A,X                                ; 7D
MOV X,SP                               ; 9D
MOV SP,X                               ; BD
MOV A,Y                                ; DD
MOV Y,A                                ; FD
MOV (X+),A                             ; AF
MOV A,(X+)                             ; BF
MOV (X),A                              ; C6
MOV A,(X)                              ; E6
MOV Y,#$12                             ; 8D 12
MOV X,#$12                             ; CD 12
MOV A,#$12                             ; E8 12
MOV ($12+X),A                          ; C7 12
MOV ($12)+Y,A                          ; D7 12
MOV A,($12+X)                          ; E7 12
MOV A,($12)+Y                          ; F7 12
MOV $1234+X,A                          ; D5 34 12
MOV $12+X,A                            ; D4 12
MOV $1234+Y,A                          ; D6 34 12
MOV $12+Y,X                            ; D9 12
MOV $12+X,Y                            ; DB 12
MOV X,$12+Y                            ; F9 12
MOV Y,$12+X                            ; FB 12
MOV A,$1234+X                          ; F5 34 12
MOV A,$12+X                            ; F4 12
MOV A,$1234+Y                          ; F6 34 12
MOV $1234,A                            ; C5 34 12
MOV $12,A                              ; C4 12
MOV $1234,X                            ; C9 34 12
MOV $12,X                              ; D8 12
MOV $1234,Y                            ; CC 34 12
MOV $12,Y                              ; CB 12
MOV A,$1234                            ; E5 34 12
MOV A,$12                              ; E4 12
MOV X,$1234                            ; E9 34 12
MOV X,$12                              ; F8 12
MOV Y,$1234                            ; EC 34 12
MOV Y,$12                              ; EB 12
MOV $12,#$34                           ; 8F 34 12
MOV $12,$34                            ; FA 34 12

OR1 C,$1234                            ; 0A 34 32
OR1 C,!$1234                           ; 2A 34 32
AND1 C,$1234                           ; 4A 34 32
AND1 C,!$1234                          ; 6A 34 32
EOR1 C,$1234                           ; 8A 34 32

TCALL 0                                ; 01
TCALL 1                                ; 11
TCALL 2                                ; 21
TCALL 3                                ; 31
TCALL 4                                ; 41
TCALL 5                                ; 51
TCALL 6                                ; 61
TCALL 7                                ; 71
TCALL 8                                ; 81
TCALL 9                                ; 91
TCALL 10                               ; A1
TCALL 11                               ; B1
TCALL 12                               ; C1
TCALL 13                               ; D1
TCALL 14                               ; E1
TCALL 15                               ; F1
TSET $1234,a                           ; 0E 34 12
TCLR $1234,a                           ; 4E 34 12

CALL $1234                             ; 3F 34 12
PCALL $12                              ; 4F 12
JMP ($1234+X)                          ; 1F 34 12
JMP $1234                              ; 5F 34 12

PUSH P                                 ; 0D
PUSH A                                 ; 2D
PUSH X                                 ; 4D
PUSH Y                                 ; 6D

POP P                                  ; 8E
POP A                                  ; AE
POP X                                  ; CE
POP Y                                  ; EE

NOP                                    ; 00
BRK                                    ; 0F
RET                                    ; 6F
RETI                                   ; 7F
CLRP                                   ; 20
SETP                                   ; 40
CLRC                                   ; 60
SETC                                   ; 80
EI                                     ; A0
DI                                     ; C0
CLRV                                   ; E0
NOTC                                   ; ED
SLEEP                                  ; EF
STOP                                   ; FF