arch cx4
org $008000
odd_label = $008001
other_page = $008400

; pass 1 errors
;`errEbad_addr_mode
nop #1
;`errEbad_addr_mode
rts $12
;`errEbad_addr_mode
skipzs r0
;`errEbad_addr_mode
ld b, #$12
;`errEbad_addr_mode
ld a, $12
;`errEbad_addr_mode
bra
;`errEbad_addr_mode
inc a
;`errEbad_addr_mode
cmp a << 2, a
;`errEbad_addr_mode
clear a
;`errEbad_addr_mode
ld a, mbr
;`errEbad_addr_mode
st r0, mar
;`errEcx4_invalid_ram_byte
rdram ramb+3, a
; the old syntax
;`errEbad_addr_mode
rdram 0, a
;`errEbad_addr_mode
rdram 0, #$12
;`errEbad_addr_mode
wrram a, 0
;`errEbad_addr_mode
wrram #$12, 0
; RDRAM/WRRAM need a RAMB byte and a DPR offset
;`errEcx4_ramb_without_byte
rdram ramb, a
;`errEcx4_ramb_without_byte
wrram dpr+$12, ramb
;`errEcx4_dpr_without_offset
rdram ramb+0, dpr
;`errEcx4_dpr_without_offset
wrram dpr, ramb+0
; the DPR offset and RAMB byte aren't immediates
;`errEbad_addr_mode
rdram ramb+0, dpr+#$12
;`errEbad_addr_mode
rdram ramb+#0, a
; the DPR offset can't be negative
;`errEcx4_dpr_negative_offset
rdram ramb+0, dpr-1
;`errEcx4_dpr_negative_offset
wrram dpr - $10, ramb+0
; LD loads all of RAMB
;`errEcx4_ld_ramb_byte
ld a, ramb+0
;`errEcx4_ld_ramb_byte
ld mdr, ramb+1
;`errEbad_addr_mode
rdrom #$123
; constants need the #, and can't be stored to or swapped
;`errEbad_addr_mode
and a, $00FF00
;`errEbad_addr_mode
st #$00FF00, a
;`errEbad_addr_mode
swap a, #$000000
; the bus operands are BUSROM and BUSRAM. RDROM and RDRAM are only instructions
;`errEbad_addr_mode
ld mdr, rdram
;`errEbad_addr_mode
st rdrom, mdr

; pass 2 errors
;`errEbad_access_width
ld a, #$100
;`errEbad_access_width
bra $100
;`errEcx4_invalid_shift_count
shr a, #32
;`errEbad_access_width
ld ph, #$80
; not constants, so 8-bit immediates that don't fit
;`errEbad_access_width
and a, #$00FF01
;`errEbad_access_width
and a, #$FF00
;`errEbad_access_width
and a, #$00FF00+0
;`errEbad_access_width
ld p, #$00FF00
;`errEcx4_rdrom_out_of_range
rdrom $400
;`errEcx4_rdrom_out_of_range
rdrom -1
;`errEcx4_branch_misaligned
bra odd_label
;`errEcx4_branch_other_page
bra other_page
; the DPR offset is 8 bits
;`errEbad_access_width
rdram ramb+0, dpr+$100
;`errEbad_access_width
wrram dpr+-1, ramb+0
