;`ea
;`00 80 80
;`05 80 80
;`05 80 80
;`0007f 00
org $807f : db 0
segment bank=0
lbl1:
nop
lbl2 = lbl1+5
dl lbl1, lbl2
segment bank=0
lbl3 = lbl2
dl lbl3
