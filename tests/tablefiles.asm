;`78           56           34           12         
;`78 56        56 34        34 12        12 00      
;`78 56 34     56 34 12     34 12 00     12 00 00   
;`78 56 34 12  56 34 12 00  34 12 00 00  12 00 00 00
;`
;`10 20 30 10
;`10 20 30 10
;`11 21 31 11
;`42 42
;`43 43

org $008000

incsrc "data/table.asm"
db "ABCD"
dw "ABCD"
dl "ABCD"
dd "ABCD"

't' = $10
'e' = $20
's' = $30

db "test"
db 't','e','s','t'
db 't'+1,'e'+1,'s'+1,'t'+1

''' = $42 ; Comment after actual line
; ''' = $44 ; This line is a comment and should be ignored
db "'"
db '''

';' = $43 ; Comment after actual line
; ';' = $45 ; This line is a comment and should be ignored
db ";"
db ';'