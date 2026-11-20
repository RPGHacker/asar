arch cx4
org $008000

; A reads as 0 when it's the register operand
;`warnWcx4_a_operand_reads_zero
;`00 50
cmp a, a
;`warnWcx4_a_operand_reads_zero
;`00 49
cmpr a, a << 1
;`warnWcx4_a_operand_reads_zero
;`00 98
smul a, a
;`warnWcx4_a_operand_reads_zero
;`00 C0
shr a, a
;`warnWcx4_a_operand_reads_zero
;`00 61
ld mdr, a

; writes to A and IR0-IR15 are ignored
;`warnWcx4_write_ignored
;`00 E1
st a, mdr
;`warnWcx4_write_ignored
;`55 E0
st ir5, a

; a bus write sends MDR, not the stored value
;`warnWcx4_bus_write_ignores_value
;`2E E0
st busrom, a
;`warnWcx4_bus_write_ignores_value
;`2F E0
st busram, a

; SWAP and LD P only use the low 4 bits of the register code. A doesn't
; read as 0 here, it acts as R0.
;`warnWcx4_register_low_bits
;`03 F0
swap a, mdr
;`warnWcx4_register_low_bits
;`2E F0
swap a, busrom
;`warnWcx4_register_low_bits
;`00 F0
swap a, a
;`warnWcx4_register_low_bits
;`03 63
ld p, mdr
;`warnWcx4_register_low_bits
;`00 63
ld p, a

; reading BUSROM or BUSRAM returns 0 and starts a bus read into MDR
;`warnWcx4_bus_read_reads_zero
;`2E 60
ld a, busrom
;`warnWcx4_bus_read_reads_zero
;`2F 80
add a, busram
;`warnWcx4_bus_read_reads_zero
;`2E 98
smul a, busrom

; these use A directly, store MDR, use R0-R15, or are how a bus read is
; started, so they don't warn
;`2E 61
ld mdr, busrom
;`2F E1
st busram, mdr
;`61 E0
st r1, a
;`00 68
rdram ramb+0, a
;`00 E8
wrram a, ramb+0
;`60 F0
swap a, r0
;`60 63
ld p, r0
;`00 70
rdrom a
;`00 54
cmp a, #$00

warnings disable Wcx4_a_operand_reads_zero
;`00 80
add a, a

; the last word of a 512-byte page. STOP is the only instruction that's fine
; there.
org $0081FE
;`warnWcx4_last_word_of_page
;`0001FE 00 00
nop
org $0083FE
;`0003FE 00 FC
stop

; branches there have their own warning, since they run with PB already set
; to P. RTS isn't a branch, so it gets the general one.
org $0085FE
;`warnWcx4_branch_last_word_of_page
;`0005FE 00 08
bra $00
org $0087FE
;`warnWcx4_branch_last_word_of_page
;`0007FE 12 0E
beq p:$12
org $0089FE
;`warnWcx4_branch_last_word_of_page
;`0009FE 34 28
jsr $34
org $008BFE
;`warnWcx4_branch_last_word_of_page
;`000BFE 00 2E
jeq p:$00
org $008DFE
;`warnWcx4_last_word_of_page
;`000DFE 00 3C
rts

; a register shift count only uses the low 5 bits, and counts above 24 act as
; 0, so a shift by any IR constant shifts by 0. SMUL isn't a shift.
org $008E00
;`warnWcx4_constant_shift_count
;`000E00 5E C0
shr a, ir14
;`warnWcx4_constant_shift_count
;`51 D8
shl a, ir1
;`warnWcx4_constant_shift_count
;`50 C8
asr a, ir0
;`warnWcx4_constant_shift_count
;`5E D0
ror a, #$000100
;`5E 98
smul a, ir14
;`60 C0
shr a, r0

; the same goes for an immediate shift count of 25-31. 24 is fine.
;`warnWcx4_shift_count_above_24
;`19 C4
shr a, #25
;`warnWcx4_shift_count_above_24
;`1F DC
shl a, #31
;`18 CC
asr a, #24

; bsnes only keeps bits 0-7 of a value stored to MACH
;`warnWcx4_mach_write_truncated
;`01 E0
st mach, a
;`warnWcx4_mach_write_truncated
;`01 E1
st mach, mdr
;`02 E0
st macl, a

; a SKIP at the second-to-last word skips the last word, which switches to
; page P. One word earlier is fine.
org $008FFA
;`warnWcx4_skip_second_to_last_word_of_page
;`000FFA 00 26 01 26 00 FC
skipzc
skipzs
stop
