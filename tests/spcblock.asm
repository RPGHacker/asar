;`03 00 00 50
;`5F 03 50
;`00 00 00 50
org $008000
arch spc700
spcblock $5000
startpos start
start:
jmp lab
lab:
endspcblock
