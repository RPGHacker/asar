;`40
;`00 00 00 00
;`00 00
org $008000
db $80/(Label2-Label1)
dd 0
Label1:
dw 0
Label2: